#include "DirectSound.h"

#include <dsound.h>
#pragma comment(lib, "dsound.lib")

namespace fuse {

fuse::DirectSound g_DirectSound;

extern "C" {

HRESULT __stdcall FuseDirectSoundCreate(LPGUID lpGuid, LPDIRECTSOUND* ppDS, LPUNKNOWN pUnkOuter) {
  return g_DirectSound.Create(lpGuid, ppDS, pUnkOuter);
}

HRESULT __stdcall FuseDirectSoundEnumerateA(LPDSENUMCALLBACKA lpDSEnumCallback, LPVOID lpContext) {
  return g_DirectSound.EnumerateA(lpDSEnumCallback, lpContext);
}

HRESULT __stdcall FuseDirectSoundEnumerateW(LPDSENUMCALLBACKW lpDSEnumCallback, LPVOID lpContext) {
  return g_DirectSound.EnumerateW(lpDSEnumCallback, lpContext);
}

HRESULT __stdcall FuseDllCanUnloadNow() {
  return g_DirectSound.DllCanUnloadNow();
}

HRESULT __stdcall FuseDllGetClassObject(REFCLSID rclsid, REFIID riid, LPVOID FAR* ppv) {
  return g_DirectSound.DllGetClassObject(rclsid, riid, ppv);
}

HRESULT __stdcall FuseGetDeviceID(LPCGUID pGuidSrc, LPGUID pGuidDest) {
  return g_DirectSound.GetDeviceID(pGuidSrc, pGuidDest);
}

HRESULT __stdcall FuseDirectSoundCaptureCreate(LPCGUID lpcGUID, LPDIRECTSOUNDCAPTURE8* lplpDSC, LPUNKNOWN pUnkOuter) {
  return g_DirectSound.CaptureCreate(lpcGUID, lplpDSC, pUnkOuter);
}

HRESULT __stdcall FuseDirectSoundCaptureEnumerateA(LPDSENUMCALLBACKA lpDSEnumCallback, LPVOID lpContext) {
  return g_DirectSound.CaptureEnumerateA(lpDSEnumCallback, lpContext);
}

HRESULT __stdcall FuseDirectSoundCaptureEnumerateW(LPDSENUMCALLBACKW lpDSEnumCallback, LPVOID lpContext) {
  return g_DirectSound.CaptureEnumerateW(lpDSEnumCallback, lpContext);
}

HRESULT __stdcall FuseDirectSoundFullDuplexCreate(LPCGUID pcGuidCaptureDevice, LPCGUID pcGuidRenderDevice,
                                                  LPCDSCBUFFERDESC pcDSCBufferDesc, LPCDSBUFFERDESC pcDSBufferDesc,
                                                  HWND hWnd, DWORD dwLevel, LPDIRECTSOUNDFULLDUPLEX* ppDSFD,
                                                  LPDIRECTSOUNDCAPTUREBUFFER8* ppDSCBuffer8,
                                                  LPDIRECTSOUNDBUFFER8* ppDSBuffer8, LPUNKNOWN pUnkOuter) {
  return g_DirectSound.FullDuplexCreate(pcGuidCaptureDevice, pcGuidRenderDevice, pcDSCBufferDesc, pcDSBufferDesc, hWnd,
                                        dwLevel, ppDSFD, ppDSCBuffer8, ppDSBuffer8, pUnkOuter);
}

HRESULT __stdcall FuseDirectSoundCreate8(LPCGUID lpcGuidDevice, LPDIRECTSOUND8* ppDS8, LPUNKNOWN pUnkOuter) {
  return g_DirectSound.Create8(lpcGuidDevice, ppDS8, pUnkOuter);
}

HRESULT __stdcall FuseDirectSoundCaptureCreate8(LPCGUID lpcGUID, LPDIRECTSOUNDCAPTURE8* lplpDSC, LPUNKNOWN pUnkOuter) {
  return g_DirectSound.CaptureCreate8(lpcGUID, lplpDSC, pUnkOuter);
}

}  // extern "C"

DirectSound DirectSound::Load(HMODULE dsound) {
  DirectSound result = {};

  result.Create = (DirectSound::CreateProc)GetProcAddress(dsound, "DirectSoundCreate");
  result.EnumerateA = (DirectSound::EnumerateAProc)GetProcAddress(dsound, "DirectSoundEnumerateA");
  result.EnumerateW = (DirectSound::EnumerateWProc)GetProcAddress(dsound, "DirectSoundEnumerateW");
  result.DllCanUnloadNow = (DirectSound::DllCanUnloadNowProc)GetProcAddress(dsound, "DllCanUnloadNow");
  result.DllGetClassObject = (DirectSound::DllGetClassObjectProc)GetProcAddress(dsound, "DllGetClassObject");
  result.CaptureCreate = (DirectSound::CaptureCreateProc)GetProcAddress(dsound, "DirectSoundCaptureCreate");
  result.CaptureEnumerateA = (DirectSound::CaptureEnumerateAProc)GetProcAddress(dsound, "DirectSoundCaptureEnumerateA");
  result.CaptureEnumerateW = (DirectSound::CaptureEnumerateWProc)GetProcAddress(dsound, "DirectSoundCaptureEnumerateW");
  result.GetDeviceID = (DirectSound::GetDeviceIDProc)GetProcAddress(dsound, "GetDeviceID");
  result.FullDuplexCreate = (DirectSound::FullDuplexCreateProc)GetProcAddress(dsound, "DirectSoundFullDuplexCreate");
  result.Create8 = (DirectSound::Create8Proc)GetProcAddress(dsound, "DirectSoundCreate8");
  result.CaptureCreate8 = (DirectSound::CaptureCreate8Proc)GetProcAddress(dsound, "DirectSoundCaptureCreate8");

  return result;
}

}  // namespace fuse
