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
static float g_RejoinDelay = 5.0f;
static float g_ServerLockedDelay = 60.0f;
static time_point g_StartTime = time_clock::now();

std::unique_ptr<marvin::Bot> bot = nullptr;
std::shared_ptr<marvin::ContinuumGameProxy> game = nullptr;

static bool enabled = true;
static bool initialize_debug = true;
static bool set_title = true;
static bool set_rejoin_timer = true;
static bool server_locked = false;
static int biller_box_occurrences = 0;

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

int WINAPI OverrideDialogBoxParamA(HINSTANCE hInstance, LPCTSTR lpTemplate, HWND hWndParent, DLGPROC lpDialogFunc,
                                   LPARAM dwInitParam) {
  OverrideGuard guard;

    enum class MenuDialogBoxId {
    AddZone = 104,
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
    BillerPlayerInformation = 183,
    OptionsSound = 185,
    OptionsRadar = 186,
    OptionsGraphics = 187,
    OptionsMenu = 188,
    OptionsOther = 189
  };

    MenuDialogBoxId dialog_id = (MenuDialogBoxId)(unsigned int)lpTemplate;

   if (dialog_id == MenuDialogBoxId::NotCentralDatabase) {
    set_rejoin_timer = true;
    // when zone is rebooted, it takes a couple minutes for the biller to reconnect
    if (biller_box_occurrences < (600 / g_RejoinDelay)) {
      biller_box_occurrences++;
        return 2;
    } else {
        biller_box_occurrences = 0;
        return 1;
    }
   }

  return RealDialogBoxParamA(hInstance, lpTemplate, hWndParent, lpDialogFunc, dwInitParam);
}

//only captures the message box when it appears for continuum
// can't get the biller disconnected message
int WINAPI OverrideMessageBoxA(HWND hWnd, LPCSTR lpText, LPCSTR lpCaption, UINT uType) {
  OverrideGuard guard;
  bool suppress = false;

  std::string text = lpText;
  std::string caption = lpCaption;

  const std::string fail_to_connect_msg =
      "Failed to connect to server. Check to make sure the server is online (it should appear yellow or green in the "
      "zone list) and that you are currently connected to the internet. If problem persists, seek help at the Tech "
      "Support Forums found at www.subspacehq.com";

  const std::string unkown_name =
      "Unkown name. Would you like to create a new user and enter the game using this name?";

  const std::string too_many_times = "You have tried to login too many times. Please try again in few minute";

  if (caption == "Information") {
    // return 0 to supress msg box
    if (text == fail_to_connect_msg) {
      set_rejoin_timer = true;
      return 0;
    }
    if (text == too_many_times) {
      set_rejoin_timer = true;
      server_locked = true;
      return 0;
    }
  }

  return RealMessageBoxA(hWnd, lpText, lpCaption, uType);
}

SHORT WINAPI OverrideGetAsyncKeyState(int vKey) {
  OverrideGuard guard;
  Initialize();

  if (!game || game->IsOnMenu()) return 0;

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


// actions in the menu happen here
// the dsound injection method will start here
// will not update if the game is running the game window
// might update if the game is also running the chat window
BOOL WINAPI OverrideGetMessageA(LPMSG lpMsg, HWND hWnd, UINT wMsgFilterMin, UINT wMsgFilterMax) { 
    OverrideGuard guard;
    Initialize();

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

    if (server_locked) {
      if (elapsed.count() > g_ServerLockedDelay) {
        server_locked = false;
      } else {
        return RealGetMessageA(lpMsg, hWnd, wMsgFilterMin, wMsgFilterMax);
      }
    }

    float delay = g_RejoinDelay + (float)g_Time.UniqueTimerByName(my_name, marvin::kBotNames, 1);

    if (elapsed.count() > delay) {
      bool result = RealGetMessageA(lpMsg, hWnd, wMsgFilterMin, wMsgFilterMax);
      
      // press enter and rejoin the game, must reselect the profile before joining
      // breaks the dsound method because this is how it starts, it wont know its name yet
      // or the name it gets will just be whatever is selected when the game is launched
      // monkeys idea, you could probably also just have 1 profile and set the name before you join  
      if (result && lpMsg->message == WM_TIMER && game->SetMenuProfileIndex()) {
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

// This is used to hook into the main update loop in Continuum so the bot can be updated.
// the marvin injection method will start here
// will not run/update if the game is in the menu
BOOL WINAPI OverridePeekMessageA(LPMSG lpMsg, HWND hWnd, UINT wMsgFilterMin, UINT wMsgFilterMax, UINT wRemoveMsg) {
  OverrideGuard guard;
  BOOL result = 0;
  Initialize();

 if (!bot) {
    game = std::make_shared<marvin::ContinuumGameProxy>();
    auto game2(game);
    bot = std::make_unique<marvin::Bot>(std::move(game2));
    // peakmsg will still run a few times when the game exits to menu, and i think reads bad game memory
    // getmsg will set bot to nullptr as long as its running so it can guard this from continuing
    return RealPeekMessageA(lpMsg, hWnd, wMsgFilterMin, wMsgFilterMax, wRemoveMsg);
  }

  marvin::ConnectState state = game->GetConnectState();
  u8* map_memory = (u8*)*(u32*)((*(u32*)0x4c1afc) + 0x127ec + 0x1d6d0);  // map loaded

  if (state != marvin::ConnectState::Playing || !map_memory) {
    if (state == marvin::ConnectState::Disconnected) {
      // try to space out the bots disconnecting during an internet outage
      // i think this causes problems accessing profile.dat file when switching to the menu
      uint64_t delay = g_Time.UniqueTimerByName(my_name, marvin::kBotNames, 1000); // milliseconds
      if (g_Time.TimedActionDelay("exitgame", delay)) {
        game->ExitGame();
      }
    }
    return RealPeekMessageA(lpMsg, hWnd, wMsgFilterMin, wMsgFilterMax, wRemoveMsg);
  }

  set_rejoin_timer = true;
  HWND g_hWnd = game->GetGameWindowHandle();

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
        game->ExitGame();
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
* The dsound method calls this the game is in the menu or even before that, so a call to get the module base will return 0.
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

