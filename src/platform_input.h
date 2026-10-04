#ifndef PLATFORM_INPUT_H_
#define PLATFORM_INPUT_H_

#include "foxhollow_mod_api.h"

/* The platform layer only reports whether a key is physically down and
   whether the game has keyboard focus. Noclip decides what a press, a hold
   and a release mean. */
typedef enum PlatformKey {
  PLATFORM_KEY_0,
  PLATFORM_KEY_KP_0
} PlatformKey;

int platformInputInitialize(FhMod* mod, const FhModHost* host);
void platformInputShutdown(void);
int platformInputActive(void);
int platformKeyDown(PlatformKey key);

#endif
