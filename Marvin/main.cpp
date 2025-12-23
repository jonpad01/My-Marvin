#include "platform/Platform.h"
//
#include <ddraw.h>
#include <detours.h>
#include <thread>
#include <atomic>
#include <winsock2.h> // include before windows.h
#include <ws2tcpip.h>
#include <windows.h>
#include <mmsystem.h>

#pragma comment(lib, "winmm.lib")
#pragma comment(lib, "ws2_32.lib")

#include <chrono>
#include <fstream>
#include <iostream>
#include <filesystem>

#include "GameProxy.h"
#include "Bot.h"
#include "Debug.h"
#include "Time.h"
#include "KeyController.h"
#include "MemoryMap.h"
#include "ProfileParser.h"

/* Marvin Basic Operation - Running Multiple Bots
*
*  When running more than one bot, bad things can happen.
* 
* The login process must be handled by a launcher that sequentially launches each bot one at a time.
* 
* If a bot is disconnected, it should exit.  The launcher will see this and relaunch it.  It should not 
* try to reconnect itself, as it has no idea if other bots are currently trying to reconnect.
* 
* If new maps are added and the arena is recycled, all bots will try to open and write to the .lvl file
* at the same time, causing error popups.
* 
* The fuse-launcher no longer needs any information used by profile.dat, Marvin will write its own data
* to memory when signaled my the launcher.  It reads from a config file that must be filled out by the user.
*
*/


/* Marvin Detours
* 
* Marvin uses detours to override winapi functions as they are called.  If no functions were called, you can't override them!  
* You have the option to add code to be executed and change what the function returns, but you can't control when the caller calls them.
* The caller is also expecting a result when the function returns, so you need to give it something that makes sense.  here is a basic list of what can be done.
* 
* You can insert code to execute just about anything you want.  In this case all the code is used to simulate a player playing the game.
* 
* You can manipulate the return value to produce a result you want.  If the function is a call to generate a messagebox, you can return 0 to make it fail, and
* the message box will never be created.  If you dont want to change the result, you can return a call to the real function, and it will execute normally.
* 
* You can call the function yourself and observe the result that it would produce.  If you call it before returning, then you have executed the body of the function.
* At this point you should only return a result when finished and avoid executing the same function multiple times.
* 
* You can lookup the arguments that were used by the caller, and you can also manipulate the arguments.  Most winapi function parameters are pointers that the caller reads and uses 
* depending on wether the function succeded or failed.  You can manipulate the data they point to after executing the function.  For example, it is possible to execute the real function, 
* observe the result, then manipulate the arguments afterwards to change the data that the caller will read.
* 
* Functions such as peakmessage (game loop) and getmessage (menu loop) run in a loop and can not be interupted.  They are expected to return quickly so doing anything that waits a long time 
* will cause the game to freeze.  A lot of the pathfinder stuff is pre calculated when the bot starts to reduce cpu usage, and it takes enough time to make the
* game graphics visibly stutter for a second.  If you put in something like Sleep(1000) the game will lock and never recover.  An easy mistake is to make a call to a winapi
* function that waits for something.
* 
* 
*/


#define UM_SETTEXT WM_USER + 0x69

// 104 can also be a License Agreement box that only displays on first time setup
// 107 can also be a Downloading File box, which probably pops up for downloading zone news
enum class MenuDialogBoxId { 
  None = 0,
  AddZone = 104,
  ZoneNews = 105,
  TraceZone = 107,
  AdvancedOptions = 113,
  SelectProfile = 118,
  EditMacro = 120,
  DevTeam = 121,
  EditKeybinds = 122,
  FirstTimeSetup = 134,
  ProfileEmptyPassword = 142,
  NotCentralDatabase = 144,
  ProxySettings = 151,
  OptionsDisplay = 154,
  OptionsChat = 156,
  OptionsWindow = 159,
  TipOfTheDay = 160,
  BannerEditor = 161,
  ChatFindText = 175,
  AddCustomZone = 176,
  PlayerInformation = 183, // new player setup menu
  OptionsSound = 185,
  OptionsRadar = 186,
  OptionsGraphics = 187,
  OptionsMenu = 188,
  OptionsOther = 189
};

enum class SharedMemoryState { OpeningMap, Listening, WritingToMemory, WritingToLauncher, Finished, Failed };

struct OverrideGuard {
  static std::atomic<int> depth;

  OverrideGuard() { ++depth; }

  ~OverrideGuard() { --depth; }

  static void Wait() {
    while (depth > 0) {
      std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
  }
};

std::atomic<int> OverrideGuard::depth = 0;

MenuDialogBoxId dialog_id = MenuDialogBoxId::None;
const char* kMutexName = "mtxsscont";
const std::string kEnabledText = "Continuum (enabled) - ";
const std::string kDisabledText = "Continuum (disabled) - ";

using time_clock = std::chrono::high_resolution_clock;
using time_point = time_clock::time_point;
using seconds = std::chrono::duration<float>;

std::string my_name;
uint32_t profile_index = 0;

static time_point g_LastUpdateTime;
static time_point g_JoinGameStartPoint;
static time_point g_SharedMemoryStartPoint;
static float g_UpdatdeDelay = 1.0f / 60.0f;
static float g_JoinDelay = 2.0f;
static float g_SharedMemoryWaitTime = 3.0f;

static float highest_dt = 0.0f;

std::unique_ptr<marvin::Bot> bot = nullptr;
std::shared_ptr<marvin::ContinuumGameProxy> game = nullptr;
static HHOOK g_hHook = NULL;

static bool enabled = true;
static bool initialize_debug = true;
static bool set_title = true;
static bool set_join_timer = true;
static bool quit_game = false;
static bool minimize__game_window = true;
static bool joined = false;

static SharedMemoryState sm_state = SharedMemoryState::OpeningMap;

static MemoryMap memory_map;
static HANDLE hPipe = NULL;


// This function needs to be called whenever anything changes in Continuum's memory.
// Continuum keeps track of its memory by calculating a checksum over it. Any changes to the memory outside of the
// normal game update would be caught. So we bypass this by manually calculating the crc at the end of the bot update
// and replacing the expected crc in memory.
void UpdateCRC() {
  typedef u32(__fastcall * CRCFunction)(void* This, void* thiscall_garbage);
  CRCFunction func_ptr = (CRCFunction)0x43BB80;

  u32 game_addr = (*(u32*)0x4c1afc) + 0x127ec;
  u32 result = func_ptr((void*)game_addr, 0);

  *(u32*)(game_addr + 0x6d4) = result;
}

void Initialize() {

  if (!game) game = std::make_shared<marvin::ContinuumGameProxy>();

  if (my_name.empty() && game) {
    my_name = game->GetName();
  }

  if (!marvin::log.is_open() && !my_name.empty()) {
    marvin::log.open(my_name + ".log");
    marvin::log << "Log file opened - " << my_name << std::endl;
  }
}

bool LockedInSpec() {

  if (!game) return false;
  if (game->GetZone() != marvin::Zone::ExtremeGames) return false;

  for (const marvin::ChatMessage& msg : game->GetCurrentChat()) {
    if (msg.type != marvin::ChatType::Arena) continue;

    std::string eg_msg = "[ " + my_name + " ]";
    std::string eg_packet_loss_msg = "Packet loss too high for you to enter the game.";
    bool eg_lag_locked = msg.message.compare(0, 4 + my_name.size(), eg_msg) == 0;
    if (eg_lag_locked || msg.message == eg_packet_loss_msg) return true;
  }

  return false;
}


// multicont functionallity
static HANDLE(WINAPI* RealCreateMutexA)(LPSECURITY_ATTRIBUTES lpMutexAttributes, BOOL bInitialOwner, LPCSTR lpName) = CreateMutexA;
// multicont functionallity
static HANDLE(WINAPI* RealOpenMutexA)(DWORD dwDesiredAccess, BOOL bInheritHandle, LPCSTR lpName) = OpenMutexA;

static int (WINAPI* RealConnect)(SOCKET, const sockaddr*, int) = connect;

static DWORD(WINAPI* RealGetTickCount)() = GetTickCount;

UINT_PTR (WINAPI* RealSetTimer)(HWND hWnd, UINT_PTR nIDEvent, UINT uElapse, TIMERPROC lpTimerFunc) = SetTimer;

static VOID(WINAPI* RealSleep)(DWORD) = Sleep;

DWORD(WINAPI* RealWaitForMultipleObjects)(DWORD nCount, const HANDLE* lpHandles, BOOL bWaitAll, DWORD dwMilliseconds) = WaitForMultipleObjects;

static DWORD(WINAPI* RealWaitForSingleObject)(HANDLE, DWORD) = WaitForSingleObject;

static DWORD(WINAPI* RealMsgWaitForMultipleObjects)(DWORD, const HANDLE*, BOOL, DWORD, DWORD) = MsgWaitForMultipleObjects;

static SHORT(WINAPI* RealGetAsyncKeyState)(int vKey) = GetAsyncKeyState;

LRESULT(WINAPI* RealDispatchMessageA)(const MSG* lpMsg) = DispatchMessageA;

static BOOL(WINAPI* RealPeekMessageA)(LPMSG lpMsg, HWND hWnd, UINT wMsgFilterMin, UINT wMsgFilterMax,
                                      UINT wRemoveMsg) = PeekMessageA;
static BOOL(WINAPI* RealGetMessageA)(LPMSG lpMsg, HWND hWnd, UINT wMsgFilterMin, UINT wMsgFilterMax) = GetMessageA;

static int(WINAPI* RealDialogBoxParamA)(HINSTANCE hInstance, LPCTSTR lpTemplate, HWND hWndParent, DLGPROC lpDialogFunc,
                                   LPARAM dwInitParam) = DialogBoxParamA;

static BOOL(WINAPI* RealEndDialog)(HWND hDlg, INT_PTR nResult) = EndDialog;

static int(WINAPI* RealMessageBoxA)(HWND hWnd, LPCSTR lpText, LPCSTR lpCaption, UINT uType) = MessageBoxA;

static HRESULT(STDMETHODCALLTYPE* RealBlt)(LPDIRECTDRAWSURFACE, LPRECT, LPDIRECTDRAWSURFACE, LPRECT, DWORD, LPDDBLTFX);


HANDLE WINAPI OverrideCreateMutexA(LPSECURITY_ATTRIBUTES lpMutexAttributes, BOOL bInitialOwner, LPCSTR lpName) {
  OverrideGuard guard;

  if (lpName && strcmp(lpName, kMutexName) == 0) {
    return NULL;
  }

  return RealCreateMutexA(lpMutexAttributes, bInitialOwner, lpName);
}

HANDLE WINAPI OverrideOpenMutexA(DWORD dwDesiredAccess, BOOL bInheritHandle, LPCSTR lpName) {
  OverrideGuard guard;

  if (lpName && strcmp(lpName, kMutexName) == 0) {
    return NULL;
  }

  return RealOpenMutexA(dwDesiredAccess, bInheritHandle, lpName);
}

UINT_PTR WINAPI OverrideSetTimer(HWND hWnd, UINT_PTR nIDEvent, UINT uElapse, TIMERPROC lpTimerFunc) {
  
  static DWORD jitter = [] {
    DWORD pid = GetCurrentProcessId();
    return pid % 15; // 0–14 ms
    }();

  // Only jitter non-zero timers
  if (uElapse >= 30) {
    uElapse += jitter;
  }

  return RealSetTimer(hWnd, nIDEvent, uElapse, lpTimerFunc);
}


static bool override_tick_init = false;
static DWORD last_logged = 0;

DWORD WINAPI OverrideGetTickCount()
{
  static DWORD offset = 0;


  if (!override_tick_init) {
    static DWORD offset = (GetCurrentProcessId() * 1103515245u) % 300; // 0-299 ms
    override_tick_init = true;
  }

  return RealGetTickCount() + offset;

  DWORD t = RealGetTickCount();

  
  if (t - last_logged >= 10000)
  {
    last_logged = t;
    // Replace with your logger
    marvin::log << "GetTickCount crossed 5s boundary: " << t << std::endl;
  }

  return t;
}

VOID WINAPI OverrideSleep(DWORD ms)
{ 
  static DWORD offset = [] {
    return GetCurrentProcessId() % 10; // 0–9 ms
    }();

  if (ms <= 5) {
    ms += offset;
  }

  RealSleep(ms);
}

DWORD WINAPI OverrideMsgWaitForMultipleObjects(DWORD nCount, const HANDLE* lpHandles, BOOL bWaitAll, DWORD dwMilliseconds, DWORD dwWakeMask) {
  static DWORD offset = [] {
    return (GetCurrentProcessId() % 10) + 1; // 1–10 ms
    }();

  //marvin::log << "MWFMO: " << dwMilliseconds << std::endl;

  // Only jitter short waits
  if (dwMilliseconds <= 10 && dwMilliseconds != INFINITE) {
    dwMilliseconds += offset;
  }

  return RealMsgWaitForMultipleObjects(
    nCount, lpHandles, bWaitAll, dwMilliseconds, dwWakeMask
  );
}

int WINAPI OverrideConnect(SOCKET s, const sockaddr* name, int namelen)
{
  int result = RealConnect(s, name, namelen);

  if (result == 0) {
    // Connected successfully → safe to configure socket
    int bufSize = 256 * 1024;
    setsockopt(s, SOL_SOCKET, SO_SNDBUF,
      reinterpret_cast<char*>(&bufSize), sizeof(bufSize));

    BOOL flag = TRUE;
    setsockopt(s, IPPROTO_TCP, TCP_NODELAY,
      reinterpret_cast<char*>(&flag), sizeof(flag));
  }

  return result;
}

HRESULT STDMETHODCALLTYPE OverrideBlt(LPDIRECTDRAWSURFACE surface, LPRECT dest_rect, LPDIRECTDRAWSURFACE next_surface,
                                      LPRECT src_rect, DWORD flags, LPDDBLTFX fx) {
  OverrideGuard guard;
  Initialize();

  if (!game || !game->IsInGame()) {
    return RealBlt(surface, dest_rect, next_surface, src_rect, flags, fx);
  } 

  u32 graphics_addr = *(u32*)(0x4C1AFC) + 0x30;
  LPDIRECTDRAWSURFACE primary_surface = (LPDIRECTDRAWSURFACE)*(u32*)(graphics_addr + 0x40);
  LPDIRECTDRAWSURFACE back_surface = (LPDIRECTDRAWSURFACE)*(u32*)(graphics_addr + 0x44);

  // Check if flipping. I guess there's a full screen blit instead of flip when running without vsync?
  if (surface == primary_surface && next_surface == back_surface && fx == 0) {
    marvin::g_RenderState.Render();
  }

  return RealBlt(surface, dest_rect, next_surface, src_rect, flags, fx);
}



// used to write into new player creation box, captured by OverrideDialogBoxParamA()
// because it uses window handles (hwnd) it may not work in a headless VM
// or when a machine does not have a display device attached
LRESULT CALLBACK PlayerInformationHook(int nCode, WPARAM wParam, LPARAM lParam) {
  if (nCode == HCBT_ACTIVATE)  // dialog finished creating, now safe to modify
  {
    HWND hDlg = (HWND)wParam;

    // Fill edit boxes
    SetDlgItemTextA(hDlg, 1039, "you@example.com");  // email
    SetDlgItemTextA(hDlg, 1022, "John Doe");         // real name
    SetDlgItemTextA(hDlg, 1044, "Portland");         // city
    SetDlgItemTextA(hDlg, 1045, "OR/USA");           // state/country
    SetDlgItemTextA(hDlg, 1040, "34");               // age

    // Checkboxes
    SendDlgItemMessageA(hDlg, 1046, BM_SETCHECK, BST_CHECKED, 0);    // Home
    SendDlgItemMessageA(hDlg, 1047, BM_SETCHECK, BST_UNCHECKED, 0);  // Work
    SendDlgItemMessageA(hDlg, 1048, BM_SETCHECK, BST_CHECKED, 0);    // School

    // Radio buttons
    SendDlgItemMessageA(hDlg, 1042, BM_SETCHECK, BST_CHECKED, 0);    // Male
    SendDlgItemMessageA(hDlg, 1043, BM_SETCHECK, BST_UNCHECKED, 0);  // Female

    // Remove hook as soon as we’re done
    UnhookWindowsHookEx(g_hHook);
    g_hHook = NULL;
  }

  return CallNextHookEx(g_hHook, nCode, wParam, lParam);
}

// this function is a call to create a dialog box, it waits indefinetly for EndDialog() to close the box.
// Its return value is whatever is given when calling EndDialog(), so i have no idea what value the caller is expecting
// It uses a template stored elsewhere to create the dialog box
// the best way to write text into these boxes is with a hook, or to write the values directly to memory if the address is known
int WINAPI OverrideDialogBoxParamA(HINSTANCE hInstance, LPCTSTR lpTemplate, HWND hWndParent, DLGPROC lpDialogFunc, LPARAM dwInitParam) {
  OverrideGuard guard;

   dialog_id = (MenuDialogBoxId)(unsigned int)lpTemplate;

   if (dialog_id == MenuDialogBoxId::NotCentralDatabase) {
      return IDYES;  //was previously set to 1
   }

   // can automate new player generation here by using a hook to fill the player information
   if (dialog_id == MenuDialogBoxId::PlayerInformation) {
     g_hHook = SetWindowsHookExA(WH_CBT, PlayerInformationHook, NULL, GetCurrentThreadId());
     return IDOK;  // does this work?
   }

  return RealDialogBoxParamA(hInstance, lpTemplate, hWndParent, lpDialogFunc, dwInitParam);
}

// can possibly use this to sniff out what values are passed to DialogBoxParamA
BOOL WINAPI OverrideEndDialog(HWND hDlg, INT_PTR nResult) {
  OverrideGuard guard;
  return RealEndDialog(hDlg, nResult);
}

// captures message boxes that continuum produces.
// the game uses a couple message boxes, which are easy to manipulate
//  return 0 to tell the caller it failed
// i believe this function waits indefinetly until returned, which directly controls
// what button is pressed on the message box
int WINAPI OverrideMessageBoxA(HWND hWnd, LPCSTR lpText, LPCSTR lpCaption, UINT uType) {
  OverrideGuard guard;

  std::string text = lpText;
  std::string caption = lpCaption;

  //marvin::log.Write("Message box found with title: " + caption + " and message text: " + lpText);



  if (caption == "Information") {  

    const std::string fail_to_connect_msg =
        "Failed to connect to server. Check to make sure the server is online (it should appear yellow or green in the "
        "zone list) and that you are currently connected to the internet. If problem persists, seek help at the Tech "
        "Support Forums found at www.subspacehq.com";

    const std::string unknown_name =
        "Unknown name. Would you like to create a new user and enter the game using this name?";

    const std::string too_many_times = "You have tried to login too many times. Please try again in few minute";

    if (text == fail_to_connect_msg) {  // verified
      set_join_timer = true;
      joined = false;
      return IDOK;
    }
    if (text == too_many_times) {  // verified
      set_join_timer = true;
      joined = false;
      g_JoinDelay = 60.0f;  // hopefully this never happens
      return IDOK;
    }
    if (text == unknown_name) {  // verified
      return IDYES;  //
    }
  }



  // possible error boxes ive seen so far:
  // Direct Draw error - usually happens after the full screen expansion graphic during login
  // this error can be related to using different resolutions, and other things im sure
  // once this error pops up the only solution was to exit all continumm processes and restart
  // Unable to create zones/****.lvl - happened during arena recycles where multiple bots download a new map
  if (caption == "Error") {

    const std::string map_file_download_failure_partial = "Unable to create zones";

    PostQuitMessage(0);  // errors mean the game has crashed, so try to quit
    quit_game = true;
    return IDOK;
  }

  // return 0 to supress all other message boxes, though im allowing them to popup so i can try to document them.
  return RealMessageBoxA(hWnd, lpText, lpCaption, uType);
}

SHORT WINAPI OverrideGetAsyncKeyState(int vKey) {
  OverrideGuard guard;
  Initialize();

 HWND g_hWnd = game->GetGameWindowHandle();

#if DEBUG_USER_CONTROL
  if (1) {
#else
  if (!enabled) {
#endif

    if (GetFocus() == g_hWnd) {
      return RealGetAsyncKeyState(vKey);
    }

    return 0;
  } else if (bot && bot->GetKeys().IsPressed(vKey)) {
    return (SHORT)0x8000;
  }

  return 0;
}


// dispatchmessage gets messages from both the game menu and the game window
// with a reliable method to detect which state the game is in, this override can
// probably override and run code for both 
// probably not much reason to switch to this though
LRESULT WINAPI OverrideDispatchMessageA(const MSG* lpMsg) {
  OverrideGuard guard;
  LRESULT result = RealDispatchMessageA(lpMsg);
  return result;
}


// MENU
// the dsound injection method will start here
// will not update if the game is running the game window
// might update if the game is also running the chat window
// this override method is blocked until a message arrives.
BOOL WINAPI OverrideGetMessageA(LPMSG lpMsg, HWND hWnd, UINT wMsgFilterMin, UINT wMsgFilterMax) {
  OverrideGuard guard;
  // only want to manipulate keypresses if the result is true, so call it before returning
  // its important that this is only called once, only return the result from this point.
  bool result = RealGetMessageA(lpMsg, hWnd, wMsgFilterMin, wMsgFilterMax);
  if (!result || !lpMsg) return result;  // if false it means the game menu is closing

  if (quit_game) {
    PostQuitMessage(0);
    return result;
  }

  // calling this here causes the game to lookup and store whatever name is currently in the player profile
  // this will get changed later if started by the fuse-loader
  if (!game) {
    game = std::make_shared<marvin::ContinuumGameProxy>();
    return result;
  }


  if (!marvin::log.is_open() && !my_name.empty()) {
    marvin::log.open(my_name + ".log");
    marvin::log << "Log file opened -  " << my_name << std::endl;
  }

  // ------------ Shared memory used with launcher -------------------------------

  // if the game was not loaded using the launcher, then this should fail 
  if (sm_state == SharedMemoryState::OpeningMap) {
    if (memory_map.OpenMap()) {
      sm_state = SharedMemoryState::Listening;
      g_SharedMemoryStartPoint = time_clock::now();
    } else {
      sm_state = SharedMemoryState::Failed;
      my_name = game->GetName();
    }
    return result;
  }

  if (sm_state == SharedMemoryState::Listening) {
    seconds elapsed = time_clock::now() - g_SharedMemoryStartPoint;
    if (memory_map.HasData()) {
      profile_index = memory_map.ReadU32();

      sm_state = SharedMemoryState::WritingToMemory;
    } else if (elapsed.count() > g_SharedMemoryWaitTime) {
      sm_state = SharedMemoryState::Failed;
      my_name = game->GetName();
    }
    return result;
  }

  if (sm_state == SharedMemoryState::WritingToMemory) {
    marvin::ProfileData data;

    bool result = marvin::ProfileParser::GetProfileData(profile_index, data);

    if (!result) {
      sm_state = SharedMemoryState::Failed;
      my_name = game->GetName();
    } else if (game->WriteToPlayerProfile(data) &&
               game->SetMenuSelectedZone(data.zone_name)) {
      my_name = data.name;
      game->SetName(my_name);
      sm_state = SharedMemoryState::WritingToLauncher;  // this state is checked in peekmessage after the game loads
    }
    return result;
  }

  // -------------------------------------------------------------------------


  

  // game is on the menu
  // calling certain functions in the game pointer here will crash because the game is not loaded
  if (game && game->IsOnMenu()) {  
    set_title = true;
    bot = nullptr;  // allow bot set to null even if not enabled

    if (!enabled) return result;

    if (set_join_timer) {
      g_JoinGameStartPoint = time_clock::now();
      set_join_timer = false;
    }

    seconds elapsed = time_clock::now() - g_JoinGameStartPoint;

    if (elapsed.count() > g_JoinDelay && !joined) {
      // press enter and join the game
      // WM_TIMER seems to be the most reliable message to hijack, WM_PAINT only gets sent
      // if the mouse is moved.
      if (lpMsg->message == WM_TIMER || lpMsg->message == WM_PAINT || lpMsg->message == WM_NULL) {
        lpMsg->message = WM_KEYDOWN;
        lpMsg->wParam = VK_RETURN;
        lpMsg->lParam = 0;
        set_join_timer = true;
        joined = true;

        return result;
      }
    }
  }

  return result;
}

//GAME
// This is used to hook into the main update loop in Continuum so the bot can be updated.
// the marvin injection method will start here
// this updates even if there is no message, but does not update when the game is closed to the menu
BOOL WINAPI OverridePeekMessageA(LPMSG lpMsg, HWND hWnd, UINT wMsgFilterMin, UINT wMsgFilterMax, UINT wRemoveMsg) {
  OverrideGuard guard;
  // windows is expecting peekmessage to process quickly, so make it the first thing to execute.
  // though whatever called it will still wait for the override to return, i think
  // when this returns false it means there is no message to procces
  // that is probably the best time to execute bot code
  std::chrono::steady_clock::time_point afterBot;
  auto start = std::chrono::high_resolution_clock::now();
  BOOL result = RealPeekMessageA(lpMsg, hWnd, wMsgFilterMin, wMsgFilterMax, wRemoveMsg);
  if (result || !lpMsg) return result;
  auto afterPeek = std::chrono::high_resolution_clock::now();

  Initialize();

  // bot is set to null when getmsg updates which means game is on the menu
 if (!bot) {
     // game needs to be rebuilt when reentering game 
     // so dont call anything for the game pointer before this
    game = std::make_shared<marvin::ContinuumGameProxy>();
    auto game2(game);
    bot = std::make_unique<marvin::Bot>(std::move(game2));
 }

  if (game->GameIsClosing()) {  // guard peekmsg when game is closing
    return result;
  }

  marvin::ConnectState state = game->GetConnectState();
  u8* map_memory = (u8*)*(u32*)((*(u32*)0x4c1afc) + 0x127ec + 0x1d6d0);  // map loaded

  // stop and wait for game and map to load
  if (state != marvin::ConnectState::Playing || !map_memory) {
    if (state == marvin::ConnectState::Disconnected) {  //bot got disconnected
      PostQuitMessage(0);
      quit_game = true;
    }
    return result;
  }

  // the launcher only needs to know that marvin has finished logging in
  // TODO: if the map is being downloaded could send a signal here to make the launcher wait longer.
  if (sm_state == SharedMemoryState::WritingToLauncher) {
    memory_map.WriteU32(134);  
    sm_state = SharedMemoryState::Finished;
  }


  HWND g_hWnd = game->GetGameWindowHandle();
  
  if (minimize__game_window) {
    ShowWindow(g_hWnd, SW_MINIMIZE);
    minimize__game_window = false;
    return result;
  }

 

  // wait until after bot is in the game
  if (set_title && g_hWnd) {
    SetWindowText(g_hWnd, (kEnabledText + my_name).c_str());
    set_title = false;
    return result;
  }

  #if DEBUG_RENDER
  if (initialize_debug) {

    u32 graphics_addr = *(u32*)(0x4C1AFC) + 0x30;
    LPDIRECTDRAWSURFACE surface = (LPDIRECTDRAWSURFACE) * (u32*)(graphics_addr + 0x44);
    void** vtable = (*(void***)surface);
    RealBlt = (HRESULT(STDMETHODCALLTYPE*)(LPDIRECTDRAWSURFACE surface, LPRECT, LPDIRECTDRAWSURFACE, LPRECT, DWORD,
                                           LPDDBLTFX))vtable[5];

    DetourRestoreAfterWith();

    DetourTransactionBegin();
    DetourUpdateThread(GetCurrentThread());
    DetourAttach(&(PVOID&)RealBlt, OverrideBlt);
    DetourTransactionCommit();
    
    initialize_debug = false;
    return result;
  }

  #endif
  
  time_point now = time_clock::now();
  seconds dt = now - g_LastUpdateTime;

    // Check for key presses to enable/disable the bot.
    if (g_hWnd && GetFocus() == g_hWnd) {
      if (RealGetAsyncKeyState(VK_F10)) {
        enabled = false;
      SetWindowText(g_hWnd, (kDisabledText + my_name).c_str());
      } else if (RealGetAsyncKeyState(VK_F9)) {
        enabled = true;
      SetWindowText(g_hWnd, (kEnabledText + my_name).c_str());
      }
    }
    
    
    auto beforeBot = std::chrono::high_resolution_clock::now();

    if (!enabled) return result;
    bool locked = LockedInSpec();

    if (locked) {
      PostQuitMessage(0);
      quit_game = true;
    }
    else if (bot && dt.count() > (float)(1.0f / bot->GetUpdateInterval())) {
#if DEBUG_RENDER
      marvin::g_RenderState.renderable_texts.clear();
      marvin::g_RenderState.renderable_lines.clear();
#endif
      if (bot) bot->Update(dt.count());
     
      auto end = std::chrono::high_resolution_clock::now();
     // marvin::log << "junk: " << std::chrono::duration<float, std::milli>(beforeBot - afterPeek).count() <<
     //   " bot: " << std::chrono::duration<float, std::milli>(end - beforeBot).count() << std::endl;
      UpdateCRC();
      g_LastUpdateTime = now;
    }
    
  return result;
}



/*
* Both loaders call this.
* 
* The injector method calls this when the game is logged in
* 
* The dsound method calls this when the game is in the menu or even before that, so a call to get the module base will return 0.
*/ 
extern "C" __declspec(dllexport) void InitializeMarvin() {
  
  DetourRestoreAfterWith();

  DetourTransactionBegin();
  DetourUpdateThread(GetCurrentThread());
  DetourAttach(&(PVOID&)RealGetAsyncKeyState, OverrideGetAsyncKeyState);
  DetourAttach(&(PVOID&)RealDispatchMessageA, OverrideDispatchMessageA);
  DetourAttach(&(PVOID&)RealPeekMessageA, OverridePeekMessageA);
  DetourAttach(&(PVOID&)RealGetMessageA, OverrideGetMessageA);
  DetourAttach(&(PVOID&)RealDialogBoxParamA, OverrideDialogBoxParamA);
  DetourAttach(&(PVOID&)RealEndDialog, OverrideEndDialog);
  DetourAttach(&(PVOID&)RealMessageBoxA, OverrideMessageBoxA);
  DetourAttach(&(PVOID&)RealCreateMutexA, OverrideCreateMutexA);
  DetourAttach(&(PVOID&)RealOpenMutexA, OverrideOpenMutexA);
  DetourAttach(&(PVOID&)RealConnect, OverrideConnect);
  DetourAttach(&(PVOID&)RealSetTimer, OverrideSetTimer);
  DetourAttach(&(PVOID&)RealSleep, OverrideSleep);
  DetourAttach(&(PVOID&)RealGetTickCount, OverrideGetTickCount);
  DetourAttach(&(PVOID&)RealMsgWaitForMultipleObjects, OverrideMsgWaitForMultipleObjects);
  

  DetourTransactionCommit();

  

}

extern "C" __declspec(dllexport) void CleanupMarvin() {
 //HWND g_hWnd = game->GetGameWindowHandle();
 // marvin::log << "CLEANUP MARVIN.\n";
  DetourTransactionBegin();
  DetourUpdateThread(GetCurrentThread());
  DetourDetach(&(PVOID&)RealGetAsyncKeyState, OverrideGetAsyncKeyState);
  DetourDetach(&(PVOID&)RealDispatchMessageA, OverrideDispatchMessageA);
  DetourDetach(&(PVOID&)RealPeekMessageA, OverridePeekMessageA);
  DetourDetach(&(PVOID&)RealGetMessageA, OverrideGetMessageA);
  DetourDetach(&(PVOID&)RealDialogBoxParamA, OverrideDialogBoxParamA);
  DetourDetach(&(PVOID&)RealEndDialog, OverrideEndDialog);
  DetourDetach(&(PVOID&)RealMessageBoxA, OverrideMessageBoxA);
  DetourDetach(&(PVOID&)RealConnect, OverrideConnect);
  DetourDetach(&(PVOID&)RealCreateMutexA, OverrideCreateMutexA);
  DetourDetach(&(PVOID&)RealOpenMutexA, OverrideOpenMutexA);
  DetourDetach(&(PVOID&)RealSetTimer, OverrideSetTimer);
  DetourDetach(&(PVOID&)RealSleep, OverrideSleep);
  DetourDetach(&(PVOID&)RealGetTickCount, OverrideGetTickCount);
  DetourDetach(&(PVOID&)RealMsgWaitForMultipleObjects, OverrideMsgWaitForMultipleObjects);
#if DEBUG_RENDER
  if (!initialize_debug) {
      DetourDetach(&(PVOID&)RealBlt, OverrideBlt);
  }
#endif
  DetourTransactionCommit();

  

  //SetWindowText(g_hWnd, "Continuum");
  OverrideGuard::Wait();
  bot = nullptr;
 // marvin::log << "CLEANUP MARVIN FINISHED.\n";
}


// not safe to call detour attach or detour detach because they use locks
BOOL WINAPI DllMain(HINSTANCE hInst, DWORD dwReason, LPVOID reserved) {
  switch (dwReason) {
      case DLL_PROCESS_ATTACH: {
        timeBeginPeriod(1);
        DWORD pid = GetCurrentProcessId();

        // 0–9 ms offset
        DWORD offsetMs = pid % 10;

        if (offsetMs > 0) {
          Sleep(offsetMs);
        }
        // Dont call initialize marvin here
      } break;
      case DLL_PROCESS_DETACH: {
        timeEndPeriod(1);
        // Dont call cleanup marvin here 
        // don't need to call detourdetach when the program exits because it is safely handled.
      } break;
  }

  return TRUE;
}

