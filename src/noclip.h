#ifndef NOCLIP_H_
#define NOCLIP_H_

#include <stddef.h>
#include <stdint.h>

#include "foxhollow_mod_api.h"

/* Partial x64 layouts of the game records Noclip touches. The game headers'
   STATIC_ASSERT offsets describe the 32-bit GameCube layout, so these offsets
   come from the headers compiled for x64 and match what objMove,
   playerUpdateVelocityFromMotion, playerDoHitDetection, ObjHits_ApplyPairResponse
   and the curves collision code read and write in the Foxhollow build. */
typedef struct GameObject GameObject;

struct GameObject {
  uint8_t pad000[0x0C];
  float localPosX;
  float localPosY;
  float localPosZ;
  float worldPosX;
  float worldPosY;
  float worldPosZ;
  float velocityX;
  float velocityY;
  float velocityZ;
  GameObject* parent;
  uint8_t pad038[0x18];
  int16_t classId;
  uint8_t pad052[0x16];
  void* hitReactState;
  uint8_t pad070[0x88];
  uint16_t objectFlags;
  uint8_t pad0FA[0x06];
  void* extra;
};

typedef struct PlayerStatus {
  int8_t health;
} PlayerStatus;

/* The baddie state sits at the start of the player state, and its curves
   collision state starts at PLAYER_STATE_CURVES_COLLISION. */
typedef struct PlayerState {
  uint32_t baddieFlags0;
  uint8_t pad004[0x3C];
  float traceStart[4][3];
  uint8_t pad070[0x154];
  float resultFloorGap;
  uint8_t pad1C8[0x08];
  float resultFloorY;
  uint8_t pad1D4[0xA0];
  uint8_t pointCounts;
  uint8_t pad275[0x3B];
  int16_t controlMode;
  uint8_t pad2B2[0x3A];
  float gravity;
  uint8_t pad2F0[0xC0];
  PlayerStatus* playerStatus;
  uint32_t flags360;
  uint8_t pad3BC[0x8C];
  uint8_t flags3F0;
  uint8_t flags3F1;
  uint8_t pad44A[0x27];
  uint8_t staffHoldFrames;
  uint8_t pad472[0x3BE];
  float verticalVel;
  uint8_t pad834[0x6C];
  GameObject* focusObject;
  uint8_t pad8A8[0x08];
  GameObject* heldObj;
  uint8_t pad8B8[0x1E];
  int16_t characterId;
} PlayerState;

/* ObjHitsPriorityState, reached through GameObject.hitReactState. */
typedef struct ObjHitsState {
  uint8_t pad000[0x64];
  int16_t flags;
  uint8_t pad066[0x08];
  uint8_t lateralResponseWeight;
} ObjHitsState;

/* State of the fixed-angle camera mode (camera mode 0x48). The anchor is the
   staticcamera map object the mode found when it started. */
typedef struct StaticCameraState {
  GameObject* anchor;
  uint8_t pad008[0xF0];
  uint8_t active;
  uint8_t missingAnchor;
} StaticCameraState;

#define STATIC_CAMERA_OBJECT_GROUP 7

#define PLAYER_STATE_CURVES_COLLISION 0x008
#define CURVES_TRACE_START 0x038

_Static_assert(offsetof(GameObject, localPosX) == 0x0C, "GameObject.anim.localPosX");
_Static_assert(offsetof(GameObject, localPosY) == 0x10, "GameObject.anim.localPosY");
_Static_assert(offsetof(GameObject, localPosZ) == 0x14, "GameObject.anim.localPosZ");
_Static_assert(offsetof(GameObject, worldPosX) == 0x18, "GameObject.anim.worldPosX");
_Static_assert(offsetof(GameObject, worldPosY) == 0x1C, "GameObject.anim.worldPosY");
_Static_assert(offsetof(GameObject, worldPosZ) == 0x20, "GameObject.anim.worldPosZ");
_Static_assert(offsetof(GameObject, velocityX) == 0x24, "GameObject.anim.velocityX");
_Static_assert(offsetof(GameObject, velocityY) == 0x28, "GameObject.anim.velocityY");
_Static_assert(offsetof(GameObject, velocityZ) == 0x2C, "GameObject.anim.velocityZ");
_Static_assert(offsetof(GameObject, parent) == 0x30, "GameObject.anim.parent");
_Static_assert(offsetof(GameObject, classId) == 0x50, "GameObject.anim.classId");
_Static_assert(offsetof(GameObject, hitReactState) == 0x68, "GameObject.anim.hitReactState");
_Static_assert(offsetof(GameObject, objectFlags) == 0xF8, "GameObject.objectFlags");
_Static_assert(offsetof(GameObject, extra) == 0x100, "GameObject.extra");
_Static_assert(offsetof(PlayerState, baddieFlags0) == 0x000, "PlayerState.baddie.flags0");
_Static_assert(offsetof(PlayerState, traceStart) == PLAYER_STATE_CURVES_COLLISION + CURVES_TRACE_START,
               "PlayerState.baddie.curvesCollision.traceStart");
_Static_assert(offsetof(PlayerState, resultFloorGap) == 0x1C4, "PlayerState.baddie.curvesCollision.resultFloorGap");
_Static_assert(offsetof(PlayerState, resultFloorY) == 0x1D0, "PlayerState.baddie.curvesCollision.resultFloorY");
_Static_assert(offsetof(PlayerState, pointCounts) == 0x274, "PlayerState.baddie.curvesCollision.pointCounts");
_Static_assert(offsetof(PlayerState, controlMode) == 0x2B0, "PlayerState.baddie.controlMode");
_Static_assert(offsetof(PlayerState, gravity) == 0x2EC, "PlayerState.baddie.gravity");
_Static_assert(offsetof(PlayerState, playerStatus) == 0x3B0, "PlayerState.playerStatus");
_Static_assert(offsetof(PlayerState, flags360) == 0x3B8, "PlayerState.flags360");
_Static_assert(offsetof(PlayerState, flags3F0) == 0x448, "PlayerState.flags3F0");
_Static_assert(offsetof(PlayerState, flags3F1) == 0x449, "PlayerState.flags3F1");
_Static_assert(offsetof(PlayerState, staffHoldFrames) == 0x471, "PlayerState.staffHoldFrames");
_Static_assert(offsetof(PlayerState, verticalVel) == 0x830, "PlayerState.verticalVel");
_Static_assert(offsetof(PlayerState, focusObject) == 0x8A0, "PlayerState.focusObject");
_Static_assert(offsetof(PlayerState, heldObj) == 0x8B0, "PlayerState.heldObj");
_Static_assert(offsetof(PlayerState, characterId) == 0x8D6, "PlayerState.characterId");
_Static_assert(offsetof(ObjHitsState, flags) == 0x64, "ObjHitsPriorityState.flags");
_Static_assert(offsetof(ObjHitsState, lateralResponseWeight) == 0x6E, "ObjHitsPriorityState.lateralResponseWeight");
_Static_assert(offsetof(StaticCameraState, anchor) == 0x00, "CameraModeStaticState.anchor");
_Static_assert(offsetof(StaticCameraState, active) == 0xF8, "CameraModeStaticState.active");
_Static_assert(offsetof(StaticCameraState, missingAnchor) == 0xF9, "CameraModeStaticState.missingAnchor");

/* ByteFlags masks of PlayerState.flags3F0 (b04 is bit 0x04). */
#define FLAGS3F0_B04 0x04
#define FLAGS3F0_B08 0x08
#define FLAGS3F0_SWIMMING 0x20
#define FLAGS3F1_ON_GROUND 0x01

typedef struct NoclipGame {
  int (*getGameState)(void);
  int (*getCurUiDll)(void);
  int (*getSaveGameLoadStatus)(void);
  GameObject* (*Obj_GetPlayerObject)(void);
  GameObject* (*getArwing)(void);
  int (*getCurSeqNo)(void);
  int32_t (*getCurMapLayer)(void);
  int (*isInBounds)(float x, float z);
  int (*trackGetHeightAboveGround)(GameObject* obj, float x, float y, float z, float* outDepth, int queryMask);
  void (*playerRefreshCollisionState)(GameObject* obj, PlayerState* state, int flags);
  void (*Obj_TransformLocalPointToWorld)(float x, float y, float z, float* outX, float* outY, float* outZ,
                                         GameObject* obj);
  float (*powfBitEstimate)(float base, float exponent);
  void (*playerTeleport)(GameObject* player, const float* position, const void* rotation, int unused);
  void (*objSetPos)(GameObject* player, float x, float y, float z);
  GameObject** (*objGetAllOfType)(int group, int* count);
  StaticCameraState** gCameraModeStaticState;
  int16_t* gArrivedWarpIndex;
  int16_t* gPendingWarpIndex;
  int* gGameLoopPendingMapId;
  float* timeDelta;
} NoclipGame;

typedef void (*PairResponseFn)(GameObject* objA, GameObject* objB, float x, float y, float z, int flag);

extern NoclipGame game;

void modLog(FhLogLevel level, const char* format, ...);

int noclipHooksInstall(FhMod* mod, const FhModHost* host);
void noclipHooksRemove(FhMod* mod, const FhModHost* host);

int noclipGameplayActive(void);
void noclipUpdateSession(void);
void noclipPoll(int toggleKeyDown, int active);
void noclipReset(void);

void noclipCaptureMove(GameObject* obj);
void noclipUpdate(GameObject* player);
void noclipBeginCollision(GameObject* obj, void* collision);
void noclipEndCollision(GameObject* obj, void* collision);
GameObject* noclipBeginProbes(GameObject* obj);
void noclipEndProbes(GameObject* outer);
int noclipSkipProbeTrace(GameObject* self, float radius, int mode, int8_t flags, int8_t mask, int slot, int8_t arg10);
void noclipPairResponse(GameObject* objA, GameObject* objB, float x, float y, float z, int flag,
                        PairResponseFn original);
void noclipPairVector(float* x, float y, float* z, GameObject* parent);
void noclipPlayerMoved(GameObject* obj);
void noclipCheckStaticCamera(void);
void noclipRecoverFromVoid(void);

#endif
