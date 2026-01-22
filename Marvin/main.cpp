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
#include "platform/Menu.h"

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

enum class ComSequence { OpenMap, Listen, WriteToMenu, SendToLauncher, Finished, Failed };
using enum ComSequence;


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

std::string name;
uint32_t profile_index = 0;

static time_point g_LastUpdateTime;
static time_point g_JoinGameStartPoint = time_clock::now();
static time_point g_SharedMemoryStartPoint = time_clock::now();;
static float g_UpdatdeDelay = 1.0f / 60.0f;
static float g_JoinDelay = 3.0f; // any faster and the server will throw a messagebox
static float g_SharedMemoryWaitTime = 3.0f;


std::unique_ptr<marvin::Bot> bot;
std::unique_ptr<marvin::ContinuumGameProxy> game;
std::unique_ptr<marvin::Menu> menu;

static HHOOK g_hHook = NULL;

static bool enabled = true;
static bool initialize_debug = true;
static bool set_title = true;
static bool set_join_timer = true;
static bool quit_game = false;
static bool minimize_game_window = true;
static bool joined = false;
bool reopen_log = true;
static bool g_GameFocused = false;

static ComSequence g_ComSequence = ComSequence::OpenMap;

static MemoryMap memory_map;


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

bool LockedInSpec() {

  if (!game) return false;
  if (game->GetZone() != marvin::Zone::ExtremeGames) return false;

  for (const marvin::ChatMessage& msg : game->GetCurrentChat()) {
    if (msg.type != marvin::ChatType::Arena) continue;

    std::string eg_msg = "[ " + name + " ]";
    std::string eg_packet_loss_msg = "Packet loss too high for you to enter the game.";
    bool eg_lag_locked = msg.message.compare(0, 4 + name.size(), eg_msg) == 0;
    if (eg_lag_locked || msg.message == eg_packet_loss_msg) return true;
  }

  return false;
}


// multicont functionallity
static HANDLE(WINAPI* RealCreateMutexA)(LPSECURITY_ATTRIBUTES lpMutexAttributes, BOOL bInitialOwner, LPCSTR lpName) = CreateMutexA;
// multicont functionallity
static HANDLE(WINAPI* RealOpenMutexA)(DWORD dwDesiredAccess, BOOL bInheritHandle, LPCSTR lpName) = OpenMutexA;

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


HRESULT STDMETHODCALLTYPE OverrideBlt(LPDIRECTDRAWSURFACE surface, LPRECT dest_rect, LPDIRECTDRAWSURFACE next_surface,
                                      LPRECT src_rect, DWORD flags, LPDDBLTFX fx) {
  OverrideGuard guard;

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
   // confirmed works
   if (dialog_id == MenuDialogBoxId::PlayerInformation) {
     g_hHook = SetWindowsHookExA(WH_CBT, PlayerInformationHook, NULL, GetCurrentThreadId());
     return IDOK; 
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

// returns keypresses to the game even if its not in focus
// must check if the game is in focus and if not return 0
// must allow bot to press keys even when not in focus
// this is called for multiple keys every frame so don't do heavy work here
// use peekmessage to determine if game is in focus or not
SHORT WINAPI OverrideGetAsyncKeyState(int vKey) {
  OverrideGuard guard;
  SHORT user_keypress = RealGetAsyncKeyState(vKey);

  if (bot && bot->GetKeys().IsPressed(vKey) && enabled) {
#if !DEBUG_USER_CONTROL
   return user_keypress |= 0x8000;
#endif
  }

  return g_GameFocused ? user_keypress : 0;
}


// dispatchmessage gets messages from both the game menu and the game window
// with a reliable method to detect which state the game is in, this override can
// probably override and run code for both 
// probably not much reason to switch to this though
LRESULT WINAPI OverrideDispatchMessageA(const MSG* lpMsg) {
  OverrideGuard guard;
  LRESULT result = RealDispatchMessageA(lpMsg);

  if (!menu) menu = std::make_unique<marvin::Menu>();
  if (!menu->IsInitialized() || !menu->IsOnMenu()) return result;

  if (game) { game.reset(); }
  if (bot) { bot.reset(); }
  set_title = true;

  if (quit_game) {
    PostQuitMessage(0);
    return result;
  }

  if (reopen_log && !name.empty()) {
    if (marvin::log.is_open()) {
      marvin::log << "Log reopening as: " << name << ".log" << std::endl;
      marvin::log.close();
    }
    marvin::log.open(name + ".log");
    marvin::log << "Log file opened from menu:  " << name << std::endl;
    reopen_log = false;
  }

  // ------------ Shared memory used with launcher -------------------------------

  // if the game was not loaded using the launcher, then this should fail 
  switch (g_ComSequence) {
  case OpenMap: {
    if (memory_map.OpenMap()) {
      g_ComSequence = Listen;
      g_SharedMemoryStartPoint = time_clock::now();
    }
    else {
      name = menu->GetSelectedProfileName();
      g_ComSequence = Failed;
    }
  } break;
  case Listen: {
    seconds elapsed = time_clock::now() - g_SharedMemoryStartPoint;
    if (memory_map.HasData()) {
      profile_index = memory_map.ReadU32();
      g_ComSequence = WriteToMenu;
    }
    else if (elapsed.count() > g_SharedMemoryWaitTime) {
      name = menu->GetSelectedProfileName();
      g_ComSequence = Failed;
    }
    return result;
  } break;
  case WriteToMenu: {
    marvin::ProfileData data;
    bool result = marvin::ProfileParser::GetProfileData(profile_index, data);

    if (result && menu->WriteToSelectedProfile(data) && menu->SetSelectedZone(data.zone_name)) {
      name = data.name;
      g_ComSequence = SendToLauncher;
    }
    else {
      name = menu->GetSelectedProfileName();
      g_ComSequence = Failed;
    }
  } break;
  }

  // -------------------------------------------------------------------------

  seconds elapsed = time_clock::now() - g_JoinGameStartPoint;
  if (elapsed.count() < g_JoinDelay || joined) return result;

  if (g_ComSequence == SendToLauncher || g_ComSequence == Failed) {
    PostMessage(lpMsg->hwnd, WM_KEYDOWN, VK_RETURN, 0);
    PostMessage(lpMsg->hwnd, WM_KEYUP, VK_RETURN, 0);
    joined = true;
  }

  return result;
}


// MENU
// the dsound injection method will start here
// will not update if the game is running the game window
// might update if the game is also running the chat window
// this override method is blocked until a message arrives.
// TODO: confirm message is for the menu window by comparing HWND
BOOL WINAPI OverrideGetMessageA(LPMSG lpMsg, HWND hWnd, UINT wMsgFilterMin, UINT wMsgFilterMax) {
  OverrideGuard guard;
  // only run if the result is true, so call it before returning
  // its important that this is only called once, only return the result from this point.
  bool result = RealGetMessageA(lpMsg, hWnd, wMsgFilterMin, wMsgFilterMax);
  if (!result || !lpMsg) return result;  // if false it means the window is closing

  return result;
}


//GAME
// This is used to hook into the main update loop in Continuum so the bot can be updated.
// the injector method will start here
// this updates even if there is no message, but does not update when the game is closed to the menu
// this is a safe place to read and write memory becasue continuum will be idle
BOOL WINAPI OverridePeekMessageA(LPMSG lpMsg, HWND hWnd, UINT wMsgFilterMin, UINT wMsgFilterMax, UINT wRemoveMsg) {
  OverrideGuard guard;
  
  // windows is expecting peekmessage to process quickly, so make it the first thing to execute.
  // though whatever called it will still wait for the override to return
  // when this returns false it means there is no message to procces
  // that is probably the best time to execute bot code
  
  BOOL result = RealPeekMessageA(lpMsg, hWnd, wMsgFilterMin, wMsgFilterMax, wRemoveMsg);
  if (result || !lpMsg) return result;
 
  /*  Required safety checks */

  if (!menu) { menu = std::make_unique<marvin::Menu>(); }
  if (!menu->IsInitialized()) return result;

  if (name.empty()) { name = menu->GetSelectedProfileName(); }
  if (!game) { game = std::make_unique<marvin::ContinuumGameProxy>(name); }

  if (!game->IsInitialized()) return result;

  if (game->GetConnectState() == marvin::ConnectState::Disconnected) {
    PostQuitMessage(0);
    quit_game = true;
    return result;
  }

  if (!game->IsInGame() || game->GameIsClosing()) {
    bot.reset();  // for arena recycle
    return result;
  }

  // and finally, we are in the game
  HWND g_hWnd = game->GetGameWindowHandle();
  joined = true;

  // check for game window focus and cache it
  g_GameFocused = g_hWnd == GetForegroundWindow();
  
  if (!bot) { bot = std::make_unique<marvin::Bot>(game.get()); }
 
  if (reopen_log && !name.empty()) {
    if (marvin::log.is_open()) {
      marvin::log << "Log closed and reopening as: " << name << ".log" << std::endl;
      marvin::log.close();
    }
    marvin::log.open(name + ".log");
    marvin::log << "Log file opened from game:  " << name << std::endl;
    reopen_log = false;
  }

  // the launcher only needs to know that marvin has finished logging in
  // TODO: if the map is being downloaded could send a signal here to make the launcher wait longer.
  if (g_ComSequence == ComSequence::SendToLauncher) {
    memory_map.WriteU32(134);  
    g_ComSequence = ComSequence::Finished;
  }

  if (minimize_game_window) {
    ShowWindow(g_hWnd, SW_MINIMIZE);
    minimize_game_window = false;
    return result;
  }

  // wait until after bot is in the game
  if (set_title && g_hWnd) {
    SetWindowText(g_hWnd, (kEnabledText + name).c_str());
    set_title = false;
    return result;
  }
  
#if DEBUG_RENDER
  if (initialize_debug) {
    u32 graphics_addr = *(u32*)(0x4C1AFC) + 0x30;
    LPDIRECTDRAWSURFACE surface = (LPDIRECTDRAWSURFACE) * (u32*)(graphics_addr + 0x44);
    void** vtable = (*(void***)surface);
    RealBlt = 
      (HRESULT(STDMETHODCALLTYPE*)(LPDIRECTDRAWSURFACE surface, LPRECT, LPDIRECTDRAWSURFACE, LPRECT, DWORD, LPDDBLTFX))vtable[5];

    DetourRestoreAfterWith();

    DetourTransactionBegin();
    DetourUpdateThread(GetCurrentThread());
    DetourAttach(&(PVOID&)RealBlt, OverrideBlt);
    DetourTransactionCommit();
    
    initialize_debug = false;
    return result;
  }
#endif
 
    // Check for key presses to enable/disable the bot.
    if (g_GameFocused) {
      if (RealGetAsyncKeyState(VK_F10)) {
        enabled = false;
      SetWindowText(g_hWnd, (kDisabledText + name).c_str());
      } else if (RealGetAsyncKeyState(VK_F9)) {
        enabled = true;
      SetWindowText(g_hWnd, (kEnabledText + name).c_str());
      }
    }
    
    time_point now = time_clock::now();
    seconds dt = now - g_LastUpdateTime;
    
    if (!enabled) return result;
    bool locked = LockedInSpec();
    
    if (locked) {
      PostQuitMessage(0);
      quit_game = true;
      return result;
    }
    
    
    if (dt.count() > 1.0f / 60.0f) {

#if DEBUG_RENDER
      marvin::g_RenderState.renderable_texts.clear();
      marvin::g_RenderState.renderable_lines.clear();
#endif
 
      if (game->Update()) {
        marvin::PerformanceTimer timer;
        bot->Update(dt.count()); 
        marvin::g_RenderState.RenderDebugText("  Total Update Time: %04llu", timer.GetElapsedTime());
      }

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

  LONG commit = DetourTransactionCommit();
  //test_log << "DetourTransactionCommit returned: " << commit << std::endl;
  

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
  DetourDetach(&(PVOID&)RealCreateMutexA, OverrideCreateMutexA);
  DetourDetach(&(PVOID&)RealOpenMutexA, OverrideOpenMutexA);

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
        marvin::log.open("bot.log");
        marvin::log << "Log opened." << std::endl;
      } break;
      case DLL_PROCESS_DETACH: {
        timeEndPeriod(1);
        // Dont call cleanup marvin here 
        // don't need to call detourdetach when the program exits because it is safely handled.
      } break;
  }

  return TRUE;
}

