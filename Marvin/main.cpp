#include "platform/Platform.h"
//
#include <ddraw.h>
#include <detours.h>

#include <chrono>
#include <fstream>
#include <iostream>

#include "platform/ContinuumGameProxy.h"
//#include "GameProxy.h"
#include "Bot.h"
#include "Debug.h"
#include "zones/ExtremeGames.h"
#include "zones/GalaxySports.h"
#include "zones/Hockey.h"
#include "zones/PowerBall.h"
//#include "Map.h"
//#include "path/Pathfinder.h"
#include "KeyController.h"
//#include "behavior/BehaviorEngine.h"

const char* kEnabledText = "Continuum (enabled)";
const char* kDisabledText = "Continuum (disabled)";

using time_clock = std::chrono::high_resolution_clock;
using time_point = time_clock::time_point;
using seconds = std::chrono::duration<float>;

std::shared_ptr<marvin::ContinuumGameProxy> game;

std::unique_ptr<marvin::ExtremeGames> eg;
std::unique_ptr<marvin::GalaxySports> gs;
std::unique_ptr<marvin::Hockey> hz;
std::unique_ptr<marvin::PowerBall> pb;

std::unique_ptr<marvin::Bot> bot;

static bool g_Enabled = true;
static bool g_Reload = false;
HWND g_hWnd = 0;
static time_point g_LastUpdateTime;

HWND GetMainWindow();
void CreateBot();

static SHORT(WINAPI* RealGetAsyncKeyState)(int vKey) = GetAsyncKeyState;

static BOOL(WINAPI* RealPeekMessageA)(LPMSG lpMsg, HWND hWnd, UINT wMsgFilterMin, UINT wMsgFilterMax,
                                      UINT wRemoveMsg) = PeekMessageA;
static BOOL(WINAPI* RealGetMessageA)(LPMSG lpMsg, HWND hWnd, UINT wMsgFilterMin, UINT wMsgFilterMax) = GetMessageA;

static HRESULT(STDMETHODCALLTYPE* RealBlt)(LPDIRECTDRAWSURFACE, LPRECT, LPDIRECTDRAWSURFACE, LPRECT, DWORD, LPDDBLTFX);

HRESULT STDMETHODCALLTYPE OverrideBlt(LPDIRECTDRAWSURFACE surface, LPRECT dest_rect, LPDIRECTDRAWSURFACE next_surface,
                                      LPRECT src_rect, DWORD flags, LPDDBLTFX fx) {
  u32 graphics_addr = *(u32*)(0x4C1AFC) + 0x30;
  LPDIRECTDRAWSURFACE primary_surface = (LPDIRECTDRAWSURFACE) * (u32*)(graphics_addr + 0x40);
  LPDIRECTDRAWSURFACE back_surface = (LPDIRECTDRAWSURFACE) * (u32*)(graphics_addr + 0x44);

  // Check if flipping. I guess there's a full screen blit instead of flip when running without vsync?
  if (surface == primary_surface && next_surface == back_surface && fx == 0) {
    marvin::g_RenderState.Render();
  }

  return RealBlt(surface, dest_rect, next_surface, src_rect, flags, fx);
}

BOOL CALLBACK MyEnumWindowsProc(HWND hwnd, LPARAM lParam) {
  DWORD pid = 0;

  GetWindowThreadProcessId(hwnd, &pid);

  
  if (pid == lParam) {

    marvin::debug_log << "PID Found. " + std::to_string(pid) << std::endl;

    char title[1024];
    GetWindowText(hwnd, title, 1023);

    marvin::debug_log << title << std::endl;

    if (strcmp(title, "Continuum") == 0) {
      g_hWnd = hwnd;
      return FALSE;
    }
  }
  marvin::debug_log << "No Match Found." << std::endl;
  return TRUE;
}

HWND GetMainWindow() {
  DWORD pid = GetCurrentProcessId();
  marvin::debug_log << "Got PID." << std::endl;
  EnumWindows(MyEnumWindowsProc, pid);

  return g_hWnd;
}

SHORT WINAPI OverrideGetAsyncKeyState(int vKey) {
#if DEBUG_USER_CONTROL
  if (1) {
#else
  if (!g_Enabled) {
#endif

    if (GetFocus() == g_hWnd) {
      return RealGetAsyncKeyState(vKey);
    }

    return 0;
  } else if (eg && eg->GetKeys().IsPressed(vKey)) {
    return (SHORT)0x8000;
  } else if (gs && gs->GetKeys().IsPressed(vKey)) {
    return (SHORT)0x8000;
  } else if (hz && hz->GetKeys().IsPressed(vKey)) {
    return (SHORT)0x8000;
  } else if (pb && pb->GetKeys().IsPressed(vKey)) {
    return (SHORT)0x8000;
  } else if (bot && bot->GetKeys().IsPressed(vKey)) {
    return (SHORT)0x8000;
  }

  return 0;
}

bool GameLoaded() {
  u32 game_addr = *(u32*)0x4C1AFC;

  if (game_addr != 0) {
    // Wait for map to load
    return *(u32*)(game_addr + 0x127ec + 0x6C4) != 0;
  }

  return false;
}

BOOL WINAPI OverrideGetMessageA(LPMSG lpMsg, HWND hWnd, UINT wMsgFilterMin, UINT wMsgFilterMax) {
  if (!GameLoaded()) {
    g_Reload = true;
  }

  return RealGetMessageA(lpMsg, hWnd, wMsgFilterMin, wMsgFilterMax);
}

// This is used to hook into the main update loop in Continuum so the bot can be
// updated.
BOOL WINAPI OverridePeekMessageA(LPMSG lpMsg, HWND hWnd, UINT wMsgFilterMin, UINT wMsgFilterMax, UINT wRemoveMsg) {
  // Check for key presses to enable/disable the bot.
  if (GetFocus() == g_hWnd) {
    if (RealGetAsyncKeyState(VK_F10)) {
      g_Enabled = false;
      SetWindowText(g_hWnd, kDisabledText);
    } else if (RealGetAsyncKeyState(VK_F9)) {
      g_Enabled = true;
      SetWindowText(g_hWnd, kEnabledText);
    }
  }

  if (g_Reload) {
    g_Enabled = false;

    if (GameLoaded()) {
      CreateBot();
      g_Enabled = true;
      g_Reload = false;
      g_hWnd = GetMainWindow();
      SetWindowText(g_hWnd, kEnabledText);
    }
  }

  time_point now = time_clock::now();
  seconds dt = now - g_LastUpdateTime;

  if (g_Enabled) {
    for (marvin::ChatMessage chat : game->GetChat()) {
      std::string name = game->GetPlayer().name;

      std::string eg_msg = "[ " + name + " ]";

      if (chat.type == 0) {
        if (chat.message.compare(0, 9, "WARNING: ") == 0 ||
            (chat.message.compare(0, 4 + name.size(), eg_msg) == 0 && game->GetZone() == marvin::Zone::ExtremeGames)) {
          PostQuitMessage(0);
          return RealPeekMessageA(lpMsg, hWnd, wMsgFilterMin, wMsgFilterMax, wRemoveMsg);
        }
      }
    }

    if (dt.count() > (float)(1.0f / 60.0f)) {
#if DEBUG_RENDER
      marvin::g_RenderState.renderable_texts.clear();
      marvin::g_RenderState.renderable_lines.clear();
#endif

      if (eg) {
        eg->Update(dt.count());
      }
      if (gs) {
        gs->Update(dt.count());
      }
      if (hz) {
        hz->Update(dt.count());
      }
      if (pb) {
        pb->Update(dt.count());
      }
      if (bot) {
        bot->Update(dt.count());
      }
      g_LastUpdateTime = now;
    }
  }
  return RealPeekMessageA(lpMsg, hWnd, wMsgFilterMin, wMsgFilterMax, wRemoveMsg);
}

void CreateBot() {
  // create pointer to game and pass the window handle
  game = std::make_shared<marvin::ContinuumGameProxy>(g_hWnd);
  auto game2(game);

  if (game->GetZone() == marvin::Zone::ExtremeGames) {
    eg = std::make_unique<marvin::ExtremeGames>(std::move(game2));
  } else if (game->GetZone() == marvin::Zone::GalaxySports) {
    gs = std::make_unique<marvin::GalaxySports>(std::move(game2));
  } else if (game->GetZone() == marvin::Zone::Hockey) {
    hz = std::make_unique<marvin::Hockey>(std::move(game2));
  } else if (game->GetZone() == marvin::Zone::PowerBall) {
    pb = std::make_unique<marvin::PowerBall>(std::move(game2));
  } else {
    bot = std::make_unique<marvin::Bot>(std::move(game2));
  }
}

extern "C" __declspec(dllexport) void InitializeMarvin() {

  marvin::debug_log.open("marvin.log", std::ios::out | std::ios::app);

  marvin::debug_log << "Starting Marvin." << std::endl;

  //#if 0
  // prevent windows from throwing error messages and let the game crash out
  if (IsWindows7OrGreater() && !IsWindows8OrGreater()) {
    SetThreadErrorMode(SEM_NOGPFAULTERRORBOX, NULL);
  } else if (IsWindows8OrGreater()) {
    SetErrorMode(SEM_NOGPFAULTERRORBOX);
  }

  //#endif

  g_hWnd = GetMainWindow();

  marvin::debug_log << "Got Main Window." << std::endl;

  
  CreateBot();

  marvin::debug_log << "Bot Created.." << std::endl;

  u32 graphics_addr = *(u32*)(0x4C1AFC) + 0x30;
  LPDIRECTDRAWSURFACE surface = (LPDIRECTDRAWSURFACE) * (u32*)(graphics_addr + 0x44);
  void** vtable = (*(void***)surface);
  RealBlt = (HRESULT(STDMETHODCALLTYPE*)(LPDIRECTDRAWSURFACE surface, LPRECT, LPDIRECTDRAWSURFACE, LPRECT, DWORD,
                                         LPDDBLTFX))vtable[5];

  DetourRestoreAfterWith();

  DetourTransactionBegin();
  DetourUpdateThread(GetCurrentThread());
  DetourAttach(&(PVOID&)RealGetAsyncKeyState, OverrideGetAsyncKeyState);
  DetourAttach(&(PVOID&)RealPeekMessageA, OverridePeekMessageA);
  DetourAttach(&(PVOID&)RealGetMessageA, OverrideGetMessageA);

#if DEBUG_RENDER
  DetourAttach(&(PVOID&)RealBlt, OverrideBlt);
#endif
  DetourTransactionCommit();

  char buf[1024];
  GetWindowText(g_hWnd, buf, 1024);
  if (strcmp(buf, "Continuum") == 0) SetWindowText(g_hWnd, kEnabledText);
  //#endif

  marvin::debug_log << "Marvin started successfully." << std::endl;
}

extern "C" __declspec(dllexport) void CleanupMarvin() {
  DetourTransactionBegin();
  DetourUpdateThread(GetCurrentThread());
  DetourDetach(&(PVOID&)RealGetAsyncKeyState, OverrideGetAsyncKeyState);
  DetourDetach(&(PVOID&)RealPeekMessageA, OverridePeekMessageA);
  DetourDetach(&(PVOID&)RealGetMessageA, OverrideGetMessageA);
#if DEBUG_RENDER
  DetourDetach(&(PVOID&)RealBlt, OverrideBlt);
#endif
  DetourTransactionCommit();

  SetWindowText(g_hWnd, "Continuum");

  marvin::debug_log << "Shutting down Marvin." << std::endl;

  bot = nullptr;

  marvin::debug_log.close();
}

BOOL WINAPI DllMain(HINSTANCE hInst, DWORD dwReason, LPVOID reserved) {
  return TRUE;
}
