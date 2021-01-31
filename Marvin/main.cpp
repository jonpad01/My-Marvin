#include "platform/Platform.h"
#include <detours.h>

#include <chrono>
#include <fstream>
#include <iostream>

#include "Bot.h"
#include "Devastation.h"

#include "Debug.h"
#include "Map.h"
#include "path/Pathfinder.h"
#include "platform/ContinuumGameProxy.h"
#include "platform/ExeProcess.h"
#include "KeyController.h"
#include "behavior/BehaviorEngine.h"

const char* kEnabledText = "Continuum (enabled)";
const char* kDisabledText = "Continuum (disabled)";

using time_clock = std::chrono::high_resolution_clock;
using time_point = time_clock::time_point;
using seconds = std::chrono::duration<float>;

std::unique_ptr<marvin::Bot> bot_;
std::shared_ptr<marvin::ContinuumGameProxy> game_;
std::unique_ptr<marvin::Devastation> devastation_;

static bool g_Enabled = true;
//static HWND g_hWnd;
HWND g_hWnd = 0;
static time_point g_LastUpdateTime = time_clock::now();

static SHORT(WINAPI* RealGetAsyncKeyState)(int vKey) = GetAsyncKeyState;

static BOOL(WINAPI* RealPeekMessageA)(LPMSG lpMsg, HWND hWnd, UINT wMsgFilterMin, UINT wMsgFilterMax, UINT wRemoveMsg) = PeekMessageA;

SHORT WINAPI OverrideGetAsyncKeyState(int vKey) {
#if DEBUG_USER_CONTROL
    if (1) {
#else
  if (!g_Enabled) {
#endif
    //return RealGetAsyncKeyState(vKey);
      if (GetActiveWindow() == g_hWnd) {
          return RealGetAsyncKeyState(vKey);
      }

      return 0;
  }
  if (bot_) {
      if (bot_->GetKeys().IsPressed(vKey)) {
          return (SHORT)0x8000;
      }
  }
  if (devastation_) {
      if (devastation_->GetKeys().IsPressed(vKey)) {
          return (SHORT)0x8000;
      }
  }
  return 0;
}

// This is used to hook into the main update loop in Continuum so the bot can be
// updated.
BOOL WINAPI OverridePeekMessageA(LPMSG lpMsg, HWND hWnd, UINT wMsgFilterMin, UINT wMsgFilterMax, UINT wRemoveMsg) {

  // Check for key presses to enable/disable the bot.
  if (GetActiveWindow() == g_hWnd) {
    if (RealGetAsyncKeyState(VK_F10)) {
      g_Enabled = false;
      SetWindowText(g_hWnd, kDisabledText);
    } else if (RealGetAsyncKeyState(VK_F9)) {
      g_Enabled = true;
      SetWindowText(g_hWnd, kEnabledText);
    }
  }

  time_point now = time_clock::now();
  seconds dt = now - g_LastUpdateTime;

  if (g_Enabled) {

      if (devastation_) { devastation_->Update(dt.count()); }

      if (bot_) { bot_->Update(dt.count()); }
    
  }

  g_LastUpdateTime = now;

#if DEBUG_RENDER
  marvin::WaitForSync();
#endif


  return RealPeekMessageA(lpMsg, hWnd, wMsgFilterMin, wMsgFilterMax, wRemoveMsg);
}

BOOL CALLBACK MyEnumWindowsProc(HWND hwnd, LPARAM lParam) {
    DWORD pid;

    GetWindowThreadProcessId(hwnd, &pid);

    if (pid == lParam) {
        g_hWnd = hwnd;
        return FALSE;
    }

    return TRUE;
}

HWND GetMainWindow() {
    DWORD pid = GetCurrentProcessId();

    EnumWindows(MyEnumWindowsProc, pid);

    return g_hWnd;
}

//marvin::Bot& CreateBot() {
void CreateBot() {
    //create pointer to game and pass the window handle
    game_ = std::make_shared<marvin::ContinuumGameProxy>(g_hWnd);

    std::shared_ptr<marvin::ContinuumGameProxy> game2(game_);
    
    //find the zone and create a pointer to that zones behavior tree, passing the copied bot pointer
    if (game_->GetServerFolder() == "zones\\SSCJ Devastation") {
        std::shared_ptr<marvin::ContinuumGameProxy> game3(game_);
        devastation_ = std::make_unique<marvin::Devastation>(std::move(game_), std::move(game2), std::move(game3));
    }
    else {
        bot_ = std::make_unique<marvin::Bot>(std::move(game_), std::move(game2));
    }
    //return *g_Bot;
}

extern "C" __declspec(dllexport) void InitializeMarvin() {
    //prevent windows from throwing error messages and let the game crash out
    SetErrorMode(SEM_NOGPFAULTERRORBOX);

  g_hWnd = GetMainWindow();

  marvin::debug_log.open("marvin.log", std::ios::out | std::ios::app);

  marvin::debug_log << "Starting Marvin.\n";
#if 0
  try {
    CreateBot();
  } catch (std::exception& e) {
    MessageBox(NULL, e.what(), "A", MB_OK);
  }
#endif
  CreateBot();
  

  DetourRestoreAfterWith();

  DetourTransactionBegin();
  DetourUpdateThread(GetCurrentThread());
  DetourAttach(&(PVOID&)RealGetAsyncKeyState, OverrideGetAsyncKeyState);
  DetourAttach(&(PVOID&)RealPeekMessageA, OverridePeekMessageA);
  DetourTransactionCommit();

  
  char buf[1024];
  GetWindowText(g_hWnd, buf, 1024);
  if (strcmp(buf, "Continuum") == 0) SetWindowText(g_hWnd, kEnabledText);
  
  marvin::debug_log << "Marvin started successfully." << std::endl;
}

extern "C" __declspec(dllexport) void CleanupMarvin() {
  DetourTransactionBegin();
  DetourUpdateThread(GetCurrentThread());
  DetourDetach(&(PVOID&)RealGetAsyncKeyState, OverrideGetAsyncKeyState);
  DetourDetach(&(PVOID&)RealPeekMessageA, OverridePeekMessageA);
  DetourTransactionCommit();

  //SetWindowText(g_hWnd, "Continuum");

  marvin::debug_log << "Shutting down Marvin." << std::endl;

  bot_ = nullptr;

  marvin::debug_log.close();
}

BOOL WINAPI DllMain(HINSTANCE hInst, DWORD dwReason, LPVOID reserved) {
  return TRUE;
}
