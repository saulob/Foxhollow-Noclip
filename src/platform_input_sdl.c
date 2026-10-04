#define _GNU_SOURCE

#include "platform_input.h"
#include "noclip.h"

#include <dlfcn.h>
#include <stdbool.h>
#include <stddef.h>

/* SDL_Scancode values from Foxhollow's SDL3 (SDL_scancode.h). */
#define SDL3_SCANCODE_0 39
#define SDL3_SCANCODE_KP_0 98

typedef const bool* (*SdlGetKeyboardStateFn)(int* numkeys);
typedef void* (*SdlGetKeyboardFocusFn)(void);

static SdlGetKeyboardStateFn sGetKeyboardState;
static SdlGetKeyboardFocusFn sGetKeyboardFocus;

static void* resolve_sdl_symbol(FhMod* mod, const FhModHost* host, const char* name) {
  void* address = dlsym(RTLD_DEFAULT, name);

  if (address == NULL) {
    address = host->symbolAddress(mod, name);
  }
  if (address == NULL) {
    modLog(FH_LOG_ERROR, "could not resolve %s", name);
  }
  return address;
}

int platformInputInitialize(FhMod* mod, const FhModHost* host) {
  const bool* keys;
  int count = 0;

  sGetKeyboardState = (SdlGetKeyboardStateFn)resolve_sdl_symbol(mod, host, "SDL_GetKeyboardState");
  sGetKeyboardFocus = (SdlGetKeyboardFocusFn)resolve_sdl_symbol(mod, host, "SDL_GetKeyboardFocus");
  if (sGetKeyboardState == NULL || sGetKeyboardFocus == NULL) {
    platformInputShutdown();
    return 0;
  }
  /* Numpad 0 is the highest scancode Noclip reads. */
  keys = sGetKeyboardState(&count);
  if (keys == NULL || count <= SDL3_SCANCODE_KP_0) {
    modLog(FH_LOG_ERROR, "SDL keyboard state is unavailable");
    platformInputShutdown();
    return 0;
  }
  return 1;
}

void platformInputShutdown(void) {
  sGetKeyboardState = NULL;
  sGetKeyboardFocus = NULL;
}

int platformInputActive(void) {
  return sGetKeyboardFocus != NULL && sGetKeyboardFocus() != NULL;
}

/* Keys are read by physical position: the number-row 0 and numpad 0 each have
   their own scancode, which ignores Num Lock and Shift. SDL clears this state
   when the window loses focus and restores only modifier keys when it regains
   it, so a key held across a focus change reads as up until it is pressed
   again. */
int platformKeyDown(PlatformKey key) {
  const bool* keys;
  int count = 0;
  int scancode;

  switch (key) {
    case PLATFORM_KEY_0:
      scancode = SDL3_SCANCODE_0;
      break;
    case PLATFORM_KEY_KP_0:
      scancode = SDL3_SCANCODE_KP_0;
      break;
    default:
      return 0;
  }
  if (sGetKeyboardState == NULL) return 0;
  keys = sGetKeyboardState(&count);
  return keys != NULL && scancode < count && keys[scancode];
}
