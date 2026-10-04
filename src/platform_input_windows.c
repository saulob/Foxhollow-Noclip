#include "platform_input.h"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

int platformInputInitialize(FhMod* mod, const FhModHost* host) {
  (void)mod;
  (void)host;
  return 1;
}

void platformInputShutdown(void) {
}

int platformInputActive(void) {
  HWND window = GetForegroundWindow();
  DWORD processId = 0;

  if (window == NULL) return 0;
  GetWindowThreadProcessId(window, &processId);
  return processId == GetCurrentProcessId();
}

/* GetAsyncKeyState reports the physical key, focused or not. '0' is the
   number-row 0 key (Win32 digit keys use their ASCII codes). Windows reports
   numpad 0 as VK_NUMPAD0 only with Num Lock on; with Num Lock off or Shift
   held it is VK_INSERT, which Noclip does not read. */
int platformKeyDown(PlatformKey key) {
  int virtualKey;

  switch (key) {
    case PLATFORM_KEY_0:
      virtualKey = '0';
      break;
    case PLATFORM_KEY_KP_0:
      virtualKey = VK_NUMPAD0;
      break;
    default:
      return 0;
  }
  return (GetAsyncKeyState(virtualKey) & 0x8000) != 0;
}
