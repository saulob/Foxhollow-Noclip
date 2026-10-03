#include "noclip.h"

#include <string.h>

typedef struct Symbol {
  const char* name;
  void** address;
} Symbol;

typedef void (*HookFn)(void);

typedef struct Hook {
  const char* name;
  HookFn replacement;
  void** original;
  void* target;
} Hook;

NoclipGame game;

static void (*origPlayerAnimate)(GameObject*, PlayerState*, float);
static void (*origPlayerDoEyeAnims)(GameObject*, char*);
static void (*origCurvesUpdateQueryBounds)(GameObject*, void*, float);
static void (*origCurvesAdvanceCollision)(GameObject*, void*, float);
static int (*origPlayerCheckIfClimbingOntoWall)(GameObject*, PlayerState*, PlayerState*, void*, float, uint32_t);
static int (*origTrackGetLineIntersect)(float*, float*, float, int, void*, GameObject*, int8_t, int8_t, int, int8_t);
static void (*origObjHitsApplyPairResponse)(GameObject*, GameObject*, float, float, float, int);
static void (*origObjTransformWorldVectorToLocal)(float, float, float, float*, float*, float*, GameObject*);
static void (*origObjSetPos)(GameObject*, float, float, float);
static void (*origCameraModeStaticUpdate)(void*);

/* playerUpdate is the only caller. The position and velocityY it leaves are
   the ones the main move starts from (staffAnimate,
   playerUpdateSurfaceResponse and playerUpdateVelocityFromMotion write
   neither). */
static void hookPlayerAnimate(GameObject* obj, PlayerState* state, float dt) {
  origPlayerAnimate(obj, state, dt);
  noclipCaptureMove(obj);
}

/* playerUpdate calls this right after the main objMove, with only the facing
   update and a UI query in between. */
static void hookPlayerDoEyeAnims(GameObject* obj, char* state) {
  noclipUpdate(obj);
  origPlayerDoEyeAnims(obj, state);
}

/* The path control update and advance that playerDoHitDetection brackets. */
static void hookCurvesUpdateQueryBounds(GameObject* obj, void* collision, float step) {
  noclipBeginCollision(obj, collision);
  origCurvesUpdateQueryBounds(obj, collision, step);
}

static void hookCurvesAdvanceCollision(GameObject* obj, void* collision, float step) {
  origCurvesAdvanceCollision(obj, collision, step);
  noclipEndCollision(obj, collision);
}

static int hookPlayerCheckIfClimbingOntoWall(GameObject* obj, PlayerState* state, PlayerState* state2, void* out,
                                             float fv, uint32_t probeMask) {
  GameObject* outer = noclipBeginProbes(obj);
  int result = origPlayerCheckIfClimbingOntoWall(obj, state, state2, out, fv, probeMask);

  noclipEndProbes(outer);
  return result;
}

/* Every other caller passes straight through after one pointer test. */
static int hookTrackGetLineIntersect(float* from, float* to, float radius, int mode, void* hit, GameObject* self,
                                     int8_t flags, int8_t mask, int slot, int8_t arg10) {
  if (noclipSkipProbeTrace(self, radius, mode, flags, mask, slot, arg10)) {
    return 0;
  }
  return origTrackGetLineIntersect(from, to, radius, mode, hit, self, flags, mask, slot, arg10);
}

static void hookObjHitsApplyPairResponse(GameObject* objA, GameObject* objB, float x, float y, float z, int flag) {
  noclipPairResponse(objA, objB, x, y, z, flag, origObjHitsApplyPairResponse);
}

static void hookObjTransformWorldVectorToLocal(float x, float y, float z, float* outX, float* outY, float* outZ,
                                               GameObject* obj) {
  noclipPairVector(&x, y, &z, obj);
  origObjTransformWorldVectorToLocal(x, y, z, outX, outY, outZ, obj);
}

static void hookObjSetPos(GameObject* obj, float x, float y, float z) {
  origObjSetPos(obj, x, y, z);
  noclipPlayerMoved(obj);
}

/* Camera_update calls this every frame while the fixed-angle camera is on;
   it is the only reader of the mode's anchor. */
static void hookCameraModeStaticUpdate(void* camera) {
  noclipCheckStaticCamera();
  origCameraModeStaticUpdate(camera);
}

static const Symbol kSymbols[] = {
    {"getGameState", (void**)&game.getGameState},
    {"getCurUiDll", (void**)&game.getCurUiDll},
    {"getSaveGameLoadStatus", (void**)&game.getSaveGameLoadStatus},
    {"Obj_GetPlayerObject", (void**)&game.Obj_GetPlayerObject},
    {"getArwing", (void**)&game.getArwing},
    {"getCurSeqNo", (void**)&game.getCurSeqNo},
    {"getCurMapLayer", (void**)&game.getCurMapLayer},
    {"isInBounds", (void**)&game.isInBounds},
    {"trackGetHeightAboveGround", (void**)&game.trackGetHeightAboveGround},
    {"playerRefreshCollisionState", (void**)&game.playerRefreshCollisionState},
    {"Obj_TransformLocalPointToWorld", (void**)&game.Obj_TransformLocalPointToWorld},
    {"powfBitEstimate", (void**)&game.powfBitEstimate},
    {"objGetAllOfType", (void**)&game.objGetAllOfType},
    {"playerTeleport", (void**)&game.playerTeleport},
    {"objSetPos", (void**)&game.objSetPos},
    {"gCameraModeStaticState", (void**)&game.gCameraModeStaticState},
    {"gArrivedWarpIndex", (void**)&game.gArrivedWarpIndex},
    {"gPendingWarpIndex", (void**)&game.gPendingWarpIndex},
    {"gGameLoopPendingMapId", (void**)&game.gGameLoopPendingMapId},
    {"timeDelta", (void**)&game.timeDelta},
};

static Hook sHooks[] = {
    {"playerAnimate", (HookFn)hookPlayerAnimate, (void**)&origPlayerAnimate, NULL},
    {"playerDoEyeAnims", (HookFn)hookPlayerDoEyeAnims, (void**)&origPlayerDoEyeAnims, NULL},
    {"curves_updateQueryBounds", (HookFn)hookCurvesUpdateQueryBounds, (void**)&origCurvesUpdateQueryBounds, NULL},
    {"curves_advanceCollision", (HookFn)hookCurvesAdvanceCollision, (void**)&origCurvesAdvanceCollision, NULL},
    {"playerCheckIfClimbingOntoWall", (HookFn)hookPlayerCheckIfClimbingOntoWall,
     (void**)&origPlayerCheckIfClimbingOntoWall, NULL},
    {"trackGetLineIntersect", (HookFn)hookTrackGetLineIntersect, (void**)&origTrackGetLineIntersect, NULL},
    {"ObjHits_ApplyPairResponse", (HookFn)hookObjHitsApplyPairResponse, (void**)&origObjHitsApplyPairResponse, NULL},
    {"Obj_TransformWorldVectorToLocal", (HookFn)hookObjTransformWorldVectorToLocal,
     (void**)&origObjTransformWorldVectorToLocal, NULL},
    {"objSetPos", (HookFn)hookObjSetPos, (void**)&origObjSetPos, NULL},
    {"CameraModeStatic_update", (HookFn)hookCameraModeStaticUpdate, (void**)&origCameraModeStaticUpdate, NULL},
};

#define COUNT_OF(array) ((int)(sizeof(array) / sizeof((array)[0])))

static int resolve_symbols(FhMod* mod, const FhModHost* host) {
  int ok = 1;
  int i;

  for (i = 0; i < COUNT_OF(kSymbols); i++) {
    *kSymbols[i].address = host->symbolAddress(mod, kSymbols[i].name);
    if (*kSymbols[i].address == NULL) {
      modLog(FH_LOG_ERROR, "could not resolve %s", kSymbols[i].name);
      ok = 0;
    }
  }
  return ok;
}

/* Only targets this mod patched are restored, so a target another mod owns
   is never touched. A hook the host fails to remove keeps its original, so the
   still patched entry passes straight through while Noclip is off. */
static int remove_hooks(FhMod* mod, const FhModHost* host) {
  int ok = 1;
  int i;

  for (i = COUNT_OF(sHooks) - 1; i >= 0; i--) {
    if (sHooks[i].target == NULL) {
      continue;
    }
    if (host->hookRemove(mod, sHooks[i].target) != FH_MOD_OK) {
      modLog(FH_LOG_WARN, "could not unhook %s", sHooks[i].name);
      ok = 0;
      continue;
    }
    sHooks[i].target = NULL;
    *sHooks[i].original = NULL;
  }
  return ok;
}

/* The hooks go in only after every symbol resolved, and all of them come out
   again if one fails, so a failed start never leaves a patched function
   behind. */
int noclipHooksInstall(FhMod* mod, const FhModHost* host) {
  int i;

  if (!resolve_symbols(mod, host)) {
    return 0;
  }
  for (i = 0; i < COUNT_OF(sHooks); i++) {
    void* target = host->symbolAddress(mod, sHooks[i].name);

    if (target == NULL) {
      modLog(FH_LOG_ERROR, "could not resolve %s", sHooks[i].name);
      remove_hooks(mod, host);
      return 0;
    }
    if (host->hookInstall(mod, target, (void*)sHooks[i].replacement, sHooks[i].original) != FH_MOD_OK) {
      modLog(FH_LOG_ERROR, "could not hook %s (no patch pad, or another mod already hooked it)", sHooks[i].name);
      *sHooks[i].original = NULL;
      remove_hooks(mod, host);
      return 0;
    }
    sHooks[i].target = target;
  }
  return 1;
}

void noclipHooksRemove(FhMod* mod, const FhModHost* host) {
  if (remove_hooks(mod, host)) {
    memset(&game, 0, sizeof(game));
  }
}
