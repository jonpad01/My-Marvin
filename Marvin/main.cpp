#include "platform/Platform.h"
//
#include <ddraw.h>
#include <detours.h>

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

std::string kEnabledText = "Continuum (enabled) - ";
std::string kDisabledText = "Continuum (disabled) - ";

using time_clock = std::chrono::high_resolution_clock;
using time_point = time_clock::time_point;
using seconds = std::chrono::duration<float>;

static time_point g_LastUpdateTime;
static time_point g_StartTime = time_clock::now();

std::shared_ptr<marvin::ContinuumGameProxy> game;
std::unique_ptr<marvin::Bot> bot;

static bool g_Enabled = true;
static bool g_Reload = false;

HWND g_hWnd = *(HWND*)((*(u32*)0x4C1AFC) + 0x8C);


std::string CreateBot();

// text must not be stack allocated
void SafeSetWindowText(HWND hwnd, const char* text) {
  PostMessage(hwnd, UM_SETTEXT, NULL, (LPARAM)text);
}

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
  } else if (bot && bot->GetKeys().IsPressed(vKey)) {
    return (SHORT)0x8000;
  }

  return 0;
}

bool GameLoaded() {
  u32 game_addr = *(u32*)0x4C1AFC;

  if (game_addr) {
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
  BOOL result = 0;

  time_point now = time_clock::now();
  seconds dt = now - g_LastUpdateTime;
  seconds sec = now - g_StartTime;

    // Check for key presses to enable/disable the bot.
    if (GetFocus() == g_hWnd) {
      if (RealGetAsyncKeyState(VK_F10)) {
        g_Enabled = false;
        SafeSetWindowText(g_hWnd, kDisabledText.c_str());
      } else if (RealGetAsyncKeyState(VK_F9)) {
        g_Enabled = true;
        SafeSetWindowText(g_hWnd, kEnabledText.c_str());
      }
    }

    if (g_Reload) {
      g_Enabled = false;

      if (GameLoaded()) {
        g_Enabled = true;
        g_Reload = false;
        CreateBot();
        g_hWnd = *(HWND*)((*(u32*)0x4C1AFC) + 0x8C);
        SafeSetWindowText(g_hWnd, kEnabledText.c_str());
      }
    }

    
    if (g_Enabled) {
      for (marvin::ChatMessage chat : game->GetChat()) {

        std::string name = game->GetPlayer().name;
        std::string eg_msg = "[ " + name + " ]";

        bool eg_locked_in_spec =
            chat.message.compare(0, 4 + name.size(), eg_msg) == 0 && game->GetZone() == marvin::Zone::ExtremeGames;
        bool disconected = chat.message.compare(0, 9, "WARNING: ") == 0;

        

        if (chat.type == marvin::ChatType::Arena) {
          if (disconected || eg_locked_in_spec) {
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
        if (bot) {
          bot->Update(dt.count());
        }
        g_LastUpdateTime = now;
      }
    }
    
     result = RealPeekMessageA(lpMsg, hWnd, wMsgFilterMin, wMsgFilterMax, wRemoveMsg);

    if (result && lpMsg->message == UM_SETTEXT) {
      SendMessage(g_hWnd, WM_SETTEXT, NULL, lpMsg->lParam);
    }

  return result;
}

std::string CreateBot() {

  // create pointer to game and pass the window handle
  game = std::make_shared<marvin::ContinuumGameProxy>(g_hWnd);
  auto game2(game);
 
  bot = std::make_unique<marvin::Bot>(std::move(game2));
  
  return game->GetName();
}

/* there are limitations on what win32 calls/actions can be made inside of this funcion call (DLLMain) */
extern "C" __declspec(dllexport) void InitializeMarvin() {
  marvin::PerformanceTimer timer; 
  
 std::string name = CreateBot();
 marvin::log.Write("INITIALIZE MARVIN - NEW TIMER", timer.GetElapsedTime());

 kEnabledText = kEnabledText + name; 
 kDisabledText = kDisabledText + name; 

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

  marvin::log.Write("Detours attached.", timer.GetElapsedTime());

     SafeSetWindowText(g_hWnd, kEnabledText.c_str());


  marvin::log.Write("FINISH INITIALIZE MARVIN - TOTAL TIME", timer.TimeSinceConstruction());
}

extern "C" __declspec(dllexport) void CleanupMarvin() {
  marvin::log.Write("CLEANUP MARVIN.");
  DetourTransactionBegin();
  DetourUpdateThread(GetCurrentThread());
  DetourDetach(&(PVOID&)RealGetAsyncKeyState, OverrideGetAsyncKeyState);
  DetourDetach(&(PVOID&)RealPeekMessageA, OverridePeekMessageA);
  DetourDetach(&(PVOID&)RealGetMessageA, OverrideGetMessageA);
#if DEBUG_RENDER
  DetourDetach(&(PVOID&)RealBlt, OverrideBlt);
#endif
  DetourTransactionCommit();

  SafeSetWindowText(g_hWnd, "Continuum");
  bot = nullptr;
  marvin::log.Write("CLEANUP MARVIN FINISHED.");
}

BOOL WINAPI DllMain(HINSTANCE hInst, DWORD dwReason, LPVOID reserved) {
  return TRUE;
}
