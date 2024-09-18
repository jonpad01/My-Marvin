#pragma once

#include <dsound.h>
#pragma comment(lib, "dsound.lib")

namespace fuse {

// A structure of function pointers to the real DirectSound functions.
// Create the struct with DirectSound::Load by passing in the loaded dsound module.
struct DirectSound {
  typedef HRESULT(__stdcall* CreateProc)(LPGUID lpGuid, LPDIRECTSOUND* ppDS, LPUNKNOWN pUnkOuter);  // @1

  typedef HRESULT(__stdcall* EnumerateAProc)(LPDSENUMCALLBACKA lpDSEnumCallback, LPVOID lpContext);  // @2
  typedef HRESULT(__stdcall* EnumerateWProc)(LPDSENUMCALLBACKW lpDSEnumCallback, LPVOID lpContext);  // @3

  typedef HRESULT(__stdcall* DllCanUnloadNowProc)(void);                                             // @4
  typedef HRESULT(__stdcall* DllGetClassObjectProc)(REFCLSID rclsid, REFIID riid, LPVOID FAR* ppv);  // @5

  typedef HRESULT(__stdcall* CaptureCreateProc)(LPCGUID pcGuidDevice, LPDIRECTSOUNDCAPTURE* ppDSC,
                                                LPUNKNOWN pUnkOuter);                                       // @6
  typedef HRESULT(__stdcall* CaptureEnumerateAProc)(LPDSENUMCALLBACKA lpDSEnumCallback, LPVOID lpContext);  // @7
  typedef HRESULT(__stdcall* CaptureEnumerateWProc)(LPDSENUMCALLBACKW lpDSEnumCallback, LPVOID lpContext);  // @8

  typedef HRESULT(__stdcall* GetDeviceIDProc)(LPCGUID pGuidSrc, LPGUID pGuidDest);  // @9
  typedef HRESULT(__stdcall* FullDuplexCreateProc)(LPCGUID pcGuidCaptureDevice, LPCGUID pcGuidRenderDevice,
                                                   LPCDSCBUFFERDESC pcDSCBufferDesc, LPCDSBUFFERDESC pcDSBufferDesc,
                                                   HWND hWnd, DWORD dwLevel, LPDIRECTSOUNDFULLDUPLEX* ppDSFD,
                                                   LPDIRECTSOUNDCAPTUREBUFFER8* ppDSCBuffer8,
                                                   LPDIRECTSOUNDBUFFER8* ppDSBuffer8,
                                                   LPUNKNOWN pUnkOuter);                                      // @10
  typedef HRESULT(__stdcall* Create8Proc)(LPCGUID pcGuidDevice, LPDIRECTSOUND8* ppDSB, LPUNKNOWN pUnkOuter);  // @11
  typedef HRESULT(__stdcall* CaptureCreate8Proc)(LPCGUID lpcGUID, LPDIRECTSOUNDCAPTURE8* lplpDSC,
                                                 LPUNKNOWN pUnkOuter);  // @12

  CreateProc Create;                        // @1
  EnumerateAProc EnumerateA;                // @2
  EnumerateWProc EnumerateW;                // @3
  DllCanUnloadNowProc DllCanUnloadNow;      // @4
  DllGetClassObjectProc DllGetClassObject;  // @5
  CaptureCreateProc CaptureCreate;          // @6
  CaptureEnumerateAProc CaptureEnumerateA;  // @7
  CaptureEnumerateWProc CaptureEnumerateW;  // @8
  GetDeviceIDProc GetDeviceID;              // @9
  FullDuplexCreateProc FullDuplexCreate;    // @10
  Create8Proc Create8;                      // @11
  CaptureCreate8Proc CaptureCreate8;        // @12

  static DirectSound Load(HMODULE dsound);
};

extern DirectSound g_DirectSound;

}  // namespace fuse
