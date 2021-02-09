#include "platform/Platform.h"
#include <detours.h>

#include <chrono>
#include <fstream>
#include <iostream>

#include "Devastation.h"
#include "ExtremeGames.h"
#include "GalaxySports.h"
#include "Hockey.h"
#include "Hyperspace.h"
#include "PowerBall.h"
#include "Bot.h"

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

std::unique_ptr<marvin::ContinuumGameProxy> game;

std::unique_ptr<marvin::Devastation> deva;
std::unique_ptr<marvin::ExtremeGames> eg;
std::unique_ptr<marvin::GalaxySports> gs;
std::unique_ptr<marvin::Hockey> hz;
std::unique_ptr<marvin::Hyperspace> hs;
std::unique_ptr<marvin::PowerBall> pb;

std::unique_ptr<marvin::Bot> bot;


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

        if (GetForegroundWindow() == g_hWnd) {
            return RealGetAsyncKeyState(vKey);
        }

        return 0;
    }
    else if (deva && deva->GetKeys().IsPressed(vKey)) { return (SHORT)0x8000; }
    else if (eg && eg->GetKeys().IsPressed(vKey)) { return (SHORT)0x8000; }
    else if (gs && gs->GetKeys().IsPressed(vKey)) { return (SHORT)0x8000; }
    else if (hz && hz->GetKeys().IsPressed(vKey)) { return (SHORT)0x8000; }
    else if (hs && hs->GetKeys().IsPressed(vKey)) { return (SHORT)0x8000; }
    else if (pb && pb->GetKeys().IsPressed(vKey)) { return (SHORT)0x8000; }
    else if (bot && bot->GetKeys().IsPressed(vKey)) { return (SHORT)0x8000; }

    return 0;
}

// This is used to hook into the main update loop in Continuum so the bot can be
// updated.
BOOL WINAPI OverridePeekMessageA(LPMSG lpMsg, HWND hWnd, UINT wMsgFilterMin, UINT wMsgFilterMax, UINT wRemoveMsg) {


    // Check for key presses to enable/disable the bot.
      //GetForegroundWindow()
      //GetActiveWindow()
      //GetFocus()
    if (GetActiveWindow() == g_hWnd) {
        if (RealGetAsyncKeyState(VK_F10)) {
            g_Enabled = false;
            SetWindowText(g_hWnd, kDisabledText);
        }
        else if (RealGetAsyncKeyState(VK_F9)) {
            g_Enabled = true;
            SetWindowText(g_hWnd, kEnabledText);
        }
    }


    time_point now = time_clock::now();
    seconds dt = now - g_LastUpdateTime;

    if (g_Enabled) {
#if DEBUG_RENDER
        if (dt.count() > (float)(1.0f / 60.0f)) {
            if (deva) { deva->Update(dt.count()); }
            if (eg) { eg->Update(dt.count()); }
            if (gs) { gs->Update(dt.count()); }
            if (hz) { hz->Update(dt.count()); }
            if (hs) { hs->Update(dt.count()); }
            if (pb) { pb->Update(dt.count()); }
            if (bot) { bot->Update(dt.count()); }
            g_LastUpdateTime = now;
            marvin::WaitForSync();
        }
#else
        if (deva) { deva->Update(dt.count()); }
        if (eg) { eg->Update(dt.count()); }
        if (gs) { gs->Update(dt.count()); }
        if (hz) { hz->Update(dt.count()); }
        if (hs) { hs->Update(dt.count()); }
        if (pb) { pb->Update(dt.count()); }
        if (bot) { bot->Update(dt.count()); }
#endif 
    }



#if DEBUG_RENDER
    //marvin::WaitForSync();
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


void CreateBot() {
    //create pointer to game and pass the window handle
    game = std::make_unique<marvin::ContinuumGameProxy>(g_hWnd);
   
    //find the zone and create a pointer to that zones behavior tree
    if (game->GetServerFolder() == "zones\\SSCJ Devastation") {
        deva = std::make_unique<marvin::Devastation>(std::move(game));
    }
    else if (game->GetServerFolder() == "zones\\SSCU Extreme Games") {
        eg = std::make_unique<marvin::ExtremeGames>(std::move(game));
    }
    else if (game->GetServerFolder() == "zones\\SSCJ Galaxy Sports") {
        gs = std::make_unique<marvin::GalaxySports>(std::move(game));
    }
    else if (game->GetServerFolder() == "zones\\SSCE HockeyFootball Zone") {
        hz = std::make_unique<marvin::Hockey>(std::move(game));
    }
    else if (game->GetServerFolder() == "zones\\SSCE Hyperspace") {
        hs = std::make_unique<marvin::Hyperspace>(std::move(game));
    }
    else if (game->GetServerFolder() == "zones\\SSCJ PowerBall") {
        pb = std::make_unique<marvin::PowerBall>(std::move(game));
    }
    else {
        bot = std::make_unique<marvin::Bot>(std::move(game));
    }
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

  bot = nullptr;

  marvin::debug_log.close();
}

BOOL WINAPI DllMain(HINSTANCE hInst, DWORD dwReason, LPVOID reserved) {
  return TRUE;
}
