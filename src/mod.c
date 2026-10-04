#include "noclip.h"
#include "platform_input.h"

#include <stdarg.h>
#include <stdio.h>

static const FhModHost* H;
static FhMod* M;

void modLog(FhLogLevel level, const char* format, ...) {
  char message[256];
  int prefix;
  va_list args;

  if (!H || !H->log || !M) return;
  prefix = snprintf(message, sizeof(message), "[Noclip] ");
  va_start(args, format);
  vsnprintf(message + prefix, sizeof(message) - (size_t)prefix, format, args);
  va_end(args);
  H->log(M, level, message);
}

FH_MOD_EXPORT int fh_mod_initialize(FhMod* mod, const FhModHost* host) {
  if (!host || host->abiVersion != FH_MOD_ABI_VERSION || host->structSize < sizeof(FhModHost)) return FH_MOD_ERROR;
  if (!host->log || !host->symbolAddress || !host->hookInstall || !host->hookRemove) return FH_MOD_ERROR;
  H = host;
  M = mod;
  noclipReset();
  if (!platformInputInitialize(mod, host)) {
    modLog(FH_LOG_ERROR, "disabled: keyboard input is unavailable");
    return FH_MOD_ERROR;
  }
  if (!noclipHooksInstall(mod, host)) {
    noclipHooksRemove(mod, host);
    platformInputShutdown();
    modLog(FH_LOG_ERROR, "disabled: required host symbols or hooks are unavailable");
    return FH_MOD_ERROR;
  }
  modLog(FH_LOG_INFO, "v1.1.0 loaded (0 / Numpad 0 Toggle Noclip)");
  return FH_MOD_OK;
}

/* 0 and Numpad 0 are one control: it is down while either key is down, and it
   toggles once per press and only while the game window is focused. */
FH_MOD_EXPORT void fh_mod_update(FhMod* mod) {
  int toggleDown;
  (void)mod;

  noclipUpdateSession();
  toggleDown = platformKeyDown(PLATFORM_KEY_0) || platformKeyDown(PLATFORM_KEY_KP_0);
  noclipPoll(toggleDown, noclipGameplayActive() && platformInputActive());
  noclipRecoverFromVoid();
}

FH_MOD_EXPORT void fh_mod_shutdown(FhMod* mod) {
  (void)mod;
  if (H && M) noclipHooksRemove(M, H);
  noclipReset();
  platformInputShutdown();
  H = 0;
  M = 0;
}
