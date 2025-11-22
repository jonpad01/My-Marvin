#include "platform/Platform.h"
//
#include <ddraw.h>
#include <detours.h>
#include <thread>
#include <atomic>

#include <chrono>
#include <fstream>
#include <iostream>
#include <filesystem>

#include "GameProxy.h"
#include "Bot.h"
#include "Debug.h"
#include "Time.h"
#include "KeyController.h"


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

enum class GameState { Unkown, Menu, Connecting, Playing, Disconnected };

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

GameState game_state = GameState::Unkown;
MenuDialogBoxId dialog_id = MenuDialogBoxId::None;
const char* kMutexName = "mtxsscont";
const std::string kEnabledText = "Continuum (enabled) - ";
const std::string kDisabledText = "Continuum (disabled) - ";

using time_clock = std::chrono::high_resolution_clock;
using time_point = time_clock::time_point;
using seconds = std::chrono::duration<float>;
static marvin::Time g_Time;

std::string my_name;

static time_point g_LastUpdateTime;
static time_point g_RejoinTime;
static float g_RejoinDelay = 2.0f;


std::unique_ptr<marvin::Bot> bot = nullptr;
std::shared_ptr<marvin::ContinuumGameProxy> game = nullptr;
static HHOOK g_hHook = NULL;
static marvin::PlayerProfileData profile_data;

static bool enabled = true;
static bool initialize_debug = true;
static bool set_title = true;
static bool set_rejoin_timer = true;
static bool login_complete = false;
static bool set_profile_data = false;
static bool bad_file = false;
static bool quit_game = false;
static bool initial_minimize = false;


bool WriteToComFile(const std::string& cmd) {
  std::ofstream file("MarvinData.txt");

  if (!file.is_open()) {
    return false;
  }

  file << cmd << std::endl;
  file.close();

  return true;
}

// simple format reader, easy to break
marvin::PlayerProfileData ReadProfileData() {
  marvin::PlayerProfileData data;
  std::ifstream file("MarvinData.txt");
  std::string buffer;

  if (!file.is_open()) return data;

  // check the file for correct format
  for (int i = 0; i <= 5; i++) {
    getline(file, buffer);
    if (buffer.find("=") == std::string::npos) return data;
    if (file.eof() || !file.good()) return data;
  }

  // reset
  file.clear();
  file.seekg(0);

  getline(file, buffer);
  data.player_name = buffer.substr(buffer.find("=") + 2);
  getline(file, buffer);
  data.password = buffer.substr(buffer.find("=") + 2);
  getline(file, buffer);
  buffer = buffer.substr(buffer.find("=") + 2);
  data.ship = std::stoi(buffer);
  getline(file, buffer);
  buffer = buffer.substr(buffer.find("=") + 2);
  data.window_mode = std::stoi(buffer);
  getline(file, buffer);
  data.zone_name = buffer.substr(buffer.find("=") + 2);
  getline(file, buffer);
  data.chats = buffer.substr(buffer.find("=") + 2);
  
  file.close();

  return data;
}

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

  if (!marvin::log.IsOpen() && !my_name.empty()) {
    marvin::log.Open(my_name);
    marvin::log.Write("Log file opened\n");
  }
}

bool LockedInSpec() {

  if (!game) return false;

  for (marvin::ChatMessage msg : game->GetCurrentChat()) {
    
    std::string eg_msg = "[ " + my_name + " ]";
    std::string eg_packet_loss_msg = "Packet loss too high for you to enter the game.";
    std::string hs_lag_msg = "You are too lagged to play in this arena.";

    bool eg_lag_locked =
        msg.message.compare(0, 4 + my_name.size(), eg_msg) == 0 && game->GetZone() == marvin::Zone::ExtremeGames;

    bool eg_locked_in_spec =
        eg_lag_locked || msg.message == eg_packet_loss_msg && game->GetZone() == marvin::Zone::ExtremeGames;

    bool hs_locked_in_spec = msg.message == hs_lag_msg && game->GetZone() == marvin::Zone::Hyperspace;

    if (msg.type == marvin::ChatType::Arena) {
      if (eg_locked_in_spec || hs_locked_in_spec) {
        return true;
      }
    }
  }
  return false;
}





// multicont functionallity
static HANDLE(WINAPI* RealCreateMutexA)(LPSECURITY_ATTRIBUTES lpMutexAttributes, BOOL bInitialOwner, LPCSTR lpName) = CreateMutexA;
// multicont functionallity
static HANDLE(WINAPI* RealOpenMutexA)(DWORD dwDesiredAccess, BOOL bInheritHandle, LPCSTR lpName) = OpenMutexA;

static SHORT(WINAPI* RealGetAsyncKeyState)(int vKey) = GetAsyncKeyState;

static BOOL(WINAPI* RealPeekMessageA)(LPMSG lpMsg, HWND hWnd, UINT wMsgFilterMin, UINT wMsgFilterMax,
                                      UINT wRemoveMsg) = PeekMessageA;
static BOOL(WINAPI* RealGetMessageA)(LPMSG lpMsg, HWND hWnd, UINT wMsgFilterMin, UINT wMsgFilterMax) = GetMessageA;

static int(WINAPI* RealDialogBoxParamA)(HINSTANCE hInstance, LPCTSTR lpTemplate, HWND hWndParent, DLGPROC lpDialogFunc,
                                   LPARAM dwInitParam) = DialogBoxParamA;

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



//used to write into new player creation box
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

// cannot fill data here because the box is being called and hasnt been created, need to use a hook or possible
// overide something else that can input key commands
int WINAPI OverrideDialogBoxParamA(HINSTANCE hInstance, LPCTSTR lpTemplate, HWND hWndParent, DLGPROC lpDialogFunc, LPARAM dwInitParam) {
  OverrideGuard guard;

   dialog_id = (MenuDialogBoxId)(unsigned int)lpTemplate;

   if (dialog_id == MenuDialogBoxId::NotCentralDatabase) {
      return IDYES;  //was previously set to 1
   }

   // can automate new player generation here by using a hook to fill the player information
   if (dialog_id == MenuDialogBoxId::PlayerInformation) {
     g_hHook = SetWindowsHookExA(WH_CBT, PlayerInformationHook, NULL, GetCurrentThreadId());
   }

  return RealDialogBoxParamA(hInstance, lpTemplate, hWndParent, lpDialogFunc, dwInitParam);
}

//only captures the message box when it appears for continuum
// the menu only uses a couple message boxes, the rest are dialog boxes
int WINAPI OverrideMessageBoxA(HWND hWnd, LPCSTR lpText, LPCSTR lpCaption, UINT uType) {
  OverrideGuard guard;

  std::string text = lpText;
  std::string caption = lpCaption;

  //marvin::log.Write(text);

  const std::string fail_to_connect_msg =
      "Failed to connect to server. Check to make sure the server is online (it should appear yellow or green in the "
      "zone list) and that you are currently connected to the internet. If problem persists, seek help at the Tech "
      "Support Forums found at www.subspacehq.com";

  const std::string unknown_name =
      "Unknown name. Would you like to create a new user and enter the game using this name?";

  const std::string too_many_times = "You have tried to login too many times. Please try again in few minute";

  if (caption == "Information") {  
    // return 0 to close msg box
    if (text == fail_to_connect_msg) {  // verified
      set_rejoin_timer = true;
      return IDOK;
    }
    if (text == too_many_times) {  // verified
      set_rejoin_timer = true;
      return IDOK;
    }
    if (text == unknown_name) {  // verified
      return IDYES;  //
    }
  }

  return RealMessageBoxA(hWnd, lpText, lpCaption, uType);
}

SHORT WINAPI OverrideGetAsyncKeyState(int vKey) {
  OverrideGuard guard;
  Initialize();

  if (!game || game->IsOnMenu()) return RealGetAsyncKeyState(vKey);

  if (game->GameIsClosing()) {  // guard keys when game is closing
    return 0;
  }

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


// MENU
// the dsound injection method will start here
// will not update if the game is running the game window
// might update if the game is also running the chat window
BOOL WINAPI OverrideGetMessageA(LPMSG lpMsg, HWND hWnd, UINT wMsgFilterMin, UINT wMsgFilterMax) { 
    OverrideGuard guard;

    if (quit_game) {
      PostQuitMessage(0);
      return RealGetMessageA(lpMsg, hWnd, wMsgFilterMin, wMsgFilterMax);
    }
    
    if (!game) {
      game = std::make_shared<marvin::ContinuumGameProxy>();
      return RealGetMessageA(lpMsg, hWnd, wMsgFilterMin, wMsgFilterMax);
    }

    if (!marvin::log.IsOpen() && !my_name.empty()) {
      marvin::log.Open(my_name);
      marvin::log.Write("Log file opened\n");
    }

    if (profile_data.player_name.empty()) {  // initializing data
      profile_data = ReadProfileData();
      if (profile_data.player_name.empty()) {  // no file or bad data
        bad_file = true;
        profile_data.player_name = game->GetName();  // get whatever name is currently on the selected profile
      }
      my_name = profile_data.player_name;
    }

    if (!set_profile_data && !bad_file) {
      if (game->WriteToPlayerProfile(profile_data) && game->SetMenuSelectedZone(profile_data.zone_name)) {
        set_profile_data = true;
      }
      return RealGetMessageA(lpMsg, hWnd, wMsgFilterMin, wMsgFilterMax);
    }

    // game is on the menu
    // calling certain functions in the game pointer here will crash because the game is not loaded
  if (game && game->IsOnMenu()) {  
    set_title = true;
    bot = nullptr;  // allow bot set to null even if not enabled

    if (!enabled) return RealGetMessageA(lpMsg, hWnd, wMsgFilterMin, wMsgFilterMax);

    if (set_rejoin_timer) {
      g_RejoinTime = time_clock::now();
      set_rejoin_timer = false;
    }

    seconds elapsed = time_clock::now() - g_RejoinTime;

    if (elapsed.count() > g_RejoinDelay) {

      // this interupts input when manually selecting menu options
      //bool result = RealGetMessageA(lpMsg, hWnd, wMsgFilterMin, wMsgFilterMax);
      
      // press enter and rejoin the game, must reselect the profile before joining
      // breaks the dsound method because this is how it starts, it wont know its name yet
      // or the name it gets will just be whatever is selected when the game is launched
      // monkeys idea, you could probably also just have 1 profile and set the name before you join  
      //if (result && lpMsg->message == WM_TIMER && game->SetMenuProfileIndex()) {
      if (lpMsg->message == WM_TIMER) {
        lpMsg->message = WM_KEYDOWN;
        lpMsg->wParam = VK_RETURN;
        lpMsg->lParam = 0;
        set_rejoin_timer = true;

        return true;
      }
    }
  }

  return RealGetMessageA(lpMsg, hWnd, wMsgFilterMin, wMsgFilterMax);
}

//GAME
// This is used to hook into the main update loop in Continuum so the bot can be updated.
// the marvin injection method will start here
// will not run/update if the game is in the menu
BOOL WINAPI OverridePeekMessageA(LPMSG lpMsg, HWND hWnd, UINT wMsgFilterMin, UINT wMsgFilterMax, UINT wRemoveMsg) {
  OverrideGuard guard;
  BOOL result = 0;
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
    return RealPeekMessageA(lpMsg, hWnd, wMsgFilterMin, wMsgFilterMax, wRemoveMsg);
  }

  marvin::ConnectState state = game->GetConnectState();
  u8* map_memory = (u8*)*(u32*)((*(u32*)0x4c1afc) + 0x127ec + 0x1d6d0);  // map loaded

  if (!login_complete && state == marvin::ConnectState::Playing &&
      WriteToComFile("OK-" + my_name)) {
    login_complete = true;
  }

  if (state != marvin::ConnectState::Playing || !map_memory) { // gaurd peekmsg when game is loading
    if (state == marvin::ConnectState::Disconnected) {
      PostQuitMessage(0);
      quit_game = true;

#if 0 
      // try to space out the bots disconnecting during an internet outage
      // i think this causes problems accessing profile.dat file when switching to the menu
      uint64_t delay = g_Time.UniqueTimerByName(my_name, marvin::kBotNames, 1000); // milliseconds
      if (g_Time.TimedActionDelay("exitgame", delay)) {
        game->ExitGame();
      }
#endif
    }
    return RealPeekMessageA(lpMsg, hWnd, wMsgFilterMin, wMsgFilterMax, wRemoveMsg);
  }

  set_rejoin_timer = true;
  HWND g_hWnd = game->GetGameWindowHandle();

  if (!initial_minimize) {
    ShowWindow(g_hWnd, SW_MINIMIZE);
    initial_minimize = true;
    return RealPeekMessageA(lpMsg, hWnd, wMsgFilterMin, wMsgFilterMax, wRemoveMsg);
  }

  // wait until after bot is in the game
  if (set_title && g_hWnd) {
    SetWindowText(g_hWnd, (kEnabledText + my_name).c_str());
    set_title = false;
    return RealPeekMessageA(lpMsg, hWnd, wMsgFilterMin, wMsgFilterMax, wRemoveMsg);
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
    return RealPeekMessageA(lpMsg, hWnd, wMsgFilterMin, wMsgFilterMax, wRemoveMsg);
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
    
    if (enabled) {
      if (LockedInSpec()) {
        PostQuitMessage(0);
        quit_game = true;
        //game->ExitGame();
      } else if (bot && dt.count() > (float)(1.0f / bot->GetUpdateInterval())) {
#if DEBUG_RENDER
        marvin::g_RenderState.renderable_texts.clear();
        marvin::g_RenderState.renderable_lines.clear();
#endif
        if (bot) bot->Update(false, dt.count());
        UpdateCRC(); 
        g_LastUpdateTime = now;
      }
    }
    
     result = RealPeekMessageA(lpMsg, hWnd, wMsgFilterMin, wMsgFilterMax, wRemoveMsg);

    if (g_hWnd && result && lpMsg->message == UM_SETTEXT) {
      SendMessage(g_hWnd, WM_SETTEXT, NULL, lpMsg->lParam);
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
  DetourAttach(&(PVOID&)RealPeekMessageA, OverridePeekMessageA);
  DetourAttach(&(PVOID&)RealGetMessageA, OverrideGetMessageA);
  DetourAttach(&(PVOID&)RealDialogBoxParamA, OverrideDialogBoxParamA);
  DetourAttach(&(PVOID&)RealMessageBoxA, OverrideMessageBoxA);
  DetourAttach(&(PVOID&)RealCreateMutexA, OverrideCreateMutexA);
  DetourAttach(&(PVOID&)RealOpenMutexA, OverrideOpenMutexA);

  DetourTransactionCommit();

}

extern "C" __declspec(dllexport) void CleanupMarvin() {
 //HWND g_hWnd = game->GetGameWindowHandle();
 // marvin::log << "CLEANUP MARVIN.\n";
  DetourTransactionBegin();
  DetourUpdateThread(GetCurrentThread());
  DetourDetach(&(PVOID&)RealGetAsyncKeyState, OverrideGetAsyncKeyState);
  DetourDetach(&(PVOID&)RealPeekMessageA, OverridePeekMessageA);
  DetourDetach(&(PVOID&)RealGetMessageA, OverrideGetMessageA);
  DetourDetach(&(PVOID&)RealDialogBoxParamA, OverrideDialogBoxParamA);
  DetourDetach(&(PVOID&)RealMessageBoxA, OverrideMessageBoxA);
  // is this right?
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


BOOL WINAPI DllMain(HINSTANCE hInst, DWORD dwReason, LPVOID reserved) {
  switch (dwReason) {
      case DLL_PROCESS_ATTACH: {
        InitializeMarvin();
      } break;
      case DLL_PROCESS_DETACH: {
        CleanupMarvin();
      } break;
  }

  return TRUE;
}

