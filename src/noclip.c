#include "noclip.h"

#include <math.h>
#include <string.h>

#define GAME_STATE_RUNNING 1
#define UI_DLL_GAMEPLAY 1
#define UI_DLL_FRONTEND_FIRST 2
#define UI_DLL_FRONTEND_LAST 7

#define PLAYER_MODE_IDLE 1
#define PLAYER_MODE_MOVING 2
#define PLAYER_MODE_ON_CLOUDRUNNER 0x1a
#define PLAYER_FLAG_LOCKED 0x200000
#define OBJECT_OBJFLAG_PARENT_SLACK 0x1000
#define PLAYER_FLAG_TELEPORT_HOLD 0x4000
#define BADDIE_FLAG_NO_GRAVITY 0x200000
#define OBJHITS_PRIORITY_STATE_IMMOVABLE 0x0400
#define PAIR_RESPONSE_LATERAL_CLASS 1

#define NO_FLOOR_Y -1e+05f
#define NOCLIP_SYNC_LOCAL_POINTS 1
#define NOCLIP_PUSH_EPSILON_SQ 1e-04f
#define NOCLIP_FLOOR_QUERY_MASK 1

/* Void recovery thresholds: frames (60 Hz units) of a fall without collision
   floor, and how far below its start the player must have fallen. */
#define VOID_FALL_TIME 30.0f
#define VOID_FALL_DROP 100.0f

#define CURVES_POINT_COUNT_SEGMENT_SHIFT 4
#define CURVES_TRACE_POINTS 4

/* playerUpdateVelocityFromMotion damps and accelerates velocityY like this,
   and playerUpdate clamps it before the main objMove. */
#define VELOCITY_Y_DAMPING 0.97f
#define VELOCITY_Y_LIMIT 4.0f
#define VELOCITY_Y_MATCH_EPSILON 1e-04f

/* Arguments of the probe traces in playerCheckIfClimbingOntoWall. Probe 11
   (bump and particles) traces with mask 0x13; every other probe uses one of
   CLIMB_PROBE_SKIPPED_MASKS. */
#define CLIMB_PROBE_MODE 3
#define CLIMB_PROBE_FLAGS 1
#define CLIMB_PROBE_SLOT 0xff
#define CLIMB_PROBE_ARG10 10
#define CLIMB_PROBE_SKIPPED_MASKS                                                                              \
  ((1u << 0x02) | (1u << 0x03) | (1u << 0x04) | (1u << 0x05) | (1u << 0x06) | (1u << 0x0a) | (1u << 0x0b) | \
   (1u << 0x0e) | (1u << 0x10) | (1u << 0x12))

enum { PAIR_MOVES_NONE, PAIR_MOVES_A, PAIR_MOVES_B, PAIR_MOVES_BOTH };

/* The player position and velocityY at the end of playerAnimate. Nothing
   between there and the main objMove writes either of them, so they are what
   the main move starts from. */
typedef struct MoveStart {
  GameObject* player;
  float x;
  float y;
  float z;
  float velocityY;
} MoveStart;

typedef struct SafePosition {
  int valid;
  float x;
  float y;
  float z;
  int mapId;
  int32_t layer;
} SafePosition;

typedef struct VoidFall {
  GameObject* player;
  int falling;
  float startY;
  float time;
  int pending;
} VoidFall;

typedef struct PairVector {
  int armed;
  int skip;
  GameObject* parent;
  float x;
  float y;
  float z;
} PairVector;

static const int16_t sNoclipCombatModes[] = {0x1f, 0x23, 0x24, 0x25, 0x26, 0x27, 0x37,
                                             0x38, 0x39, 0x3a, 0x3b, 0x3c, 0x3d};

static int sSessionActive;
static int sNoclip;
static int sNoclipToggleWasDown;
static int sNoclipCollision;
static GameObject* sNoclipCollisionPlayer;
static float sNoclipPosX;
static float sNoclipPosZ;
static int sNoclipHold;
static float sNoclipHoldY;
static GameObject* sNoclipHoldPlayer;
static MoveStart sMoveStart;
static GameObject* sProbePlayer;
static PairVector sPairVector;
static int sCameraGuard;
static SafePosition sSafe;
static VoidFall sVoid;

int noclipGameplayActive(void) {
  return game.getGameState() == GAME_STATE_RUNNING && game.getCurUiDll() == UI_DLL_GAMEPLAY &&
         game.getSaveGameLoadStatus() == 0 && (game.Obj_GetPlayerObject() != NULL || game.getArwing() != NULL);
}

static int arwing_active(void) { return noclipGameplayActive() && game.getArwing() != NULL; }

static void set_noclip(int enabled) {
  sNoclip = enabled != 0;
  sNoclipHold = 0;
  if (sNoclip) {
    sCameraGuard = 1;
  }
}

static void clear_frame_state(void) {
  sNoclipCollision = 0;
  sNoclipCollisionPlayer = NULL;
  sNoclipHoldPlayer = NULL;
  sMoveStart.player = NULL;
  sProbePlayer = NULL;
  sPairVector.armed = 0;
  memset(&sSafe, 0, sizeof(sSafe));
  memset(&sVoid, 0, sizeof(sVoid));
}

/* Noclip belongs to the loaded save: it is cleared once the game stops running
   or a front-end screen (title, save select) takes over. */
void noclipUpdateSession(void) {
  int uiDll;

  if (noclipGameplayActive()) {
    sSessionActive = 1;
    return;
  }
  uiDll = game.getCurUiDll();
  if (sSessionActive && (game.getGameState() != GAME_STATE_RUNNING ||
                         (uiDll >= UI_DLL_FRONTEND_FIRST && uiDll <= UI_DLL_FRONTEND_LAST))) {
    sSessionActive = 0;
    if (sNoclip) {
      modLog(FH_LOG_INFO, "disabled (save session ended)");
    }
    set_noclip(0);
    sCameraGuard = 0;
    clear_frame_state();
  }
}

/* Key state is passed in every frame, focused or not, so a key already held
   when the game regains focus is not seen as a new press. */
void noclipPoll(int toggleKeyDown, int active) {
  toggleKeyDown = toggleKeyDown != 0;
  if (active && toggleKeyDown && !sNoclipToggleWasDown && !arwing_active()) {
    set_noclip(!sNoclip);
    modLog(FH_LOG_INFO, sNoclip ? "enabled" : "disabled");
  }
  sNoclipToggleWasDown = toggleKeyDown;
}

static int player_controllable_on_foot(GameObject* player, PlayerState* st) {
  if (!noclipGameplayActive() || arwing_active() || st->controlMode == PLAYER_MODE_ON_CLOUDRUNNER) {
    return 0;
  }
  if (game.getCurSeqNo() != 0) {
    return 0;
  }
  if ((st->flags360 & PLAYER_FLAG_LOCKED) != 0 || st->characterId == -1) {
    return 0;
  }
  if (st->playerStatus == NULL || st->playerStatus->health <= 0) {
    return 0;
  }
  if (st->heldObj != NULL || st->verticalVel != 0.0f) {
    return 0;
  }
  return (player->objectFlags & OBJECT_OBJFLAG_PARENT_SLACK) == 0;
}

/* The same player modes the cheat menu accepted for Fly Mode and Noclip:
   standing, moving and the combat stances. */
static int noclip_mode_allowed(int16_t mode) {
  int i;

  if (mode == PLAYER_MODE_IDLE || mode == PLAYER_MODE_MOVING) {
    return 1;
  }
  for (i = 0; i < (int)(sizeof(sNoclipCombatModes) / sizeof(sNoclipCombatModes[0])); i++) {
    if (sNoclipCombatModes[i] == mode) {
      return 1;
    }
  }
  return 0;
}

static int noclip_allowed(GameObject* player, PlayerState* st) {
  return player_controllable_on_foot(player, st) && st->focusObject == NULL && noclip_mode_allowed(st->controlMode);
}

static int noclip_active(GameObject* player) {
  return sNoclip && player != NULL && player == game.Obj_GetPlayerObject() && player->extra != NULL &&
         noclip_allowed(player, player->extra);
}

static int warp_active(void) { return *game.gArrivedWarpIndex != -1 || *game.gPendingWarpIndex != -1; }

void noclipCaptureMove(GameObject* obj) {
  sMoveStart.player = NULL;
  if ((!sNoclip && !sVoid.falling) || obj == NULL) {
    return;
  }
  sMoveStart.player = obj;
  sMoveStart.x = obj->localPosX;
  sMoveStart.y = obj->localPosY;
  sMoveStart.z = obj->localPosZ;
  sMoveStart.velocityY = obj->velocityY;
}

/* The cheat menu zeroed the horizontal velocity before the main move when the
   step it predicted, scaled by its Fast Movement factor, would end in an empty
   map block. This runs after the main move, so it checks where the move really
   put the player, including any scaling Player Cheats applied, and undoes only
   the horizontal step: the same position and velocity the cheat menu left. */
static void noclip_keep_inside_map(GameObject* player, PlayerState* st, const MoveStart* start) {
  if (player->parent != NULL || st->focusObject != NULL) {
    return;
  }
  if (game.isInBounds(player->localPosX, player->localPosZ) == 0) {
    player->localPosX = start->x;
    player->localPosZ = start->z;
    player->velocityX = 0.0f;
    player->velocityZ = 0.0f;
  }
}

/* Stands in for the cheat menu's check of its own Fly Mode toggle. Vanilla
   playerUpdate moves the player with exactly the velocityY that
   playerUpdateVelocityFromMotion integrated (damping and gravity), clamped.
   Any other value means another mod drove the vertical motion this frame,
   as Fly Mode does with its up, down and hover speeds, so the floor hold
   gives way to it. */
static int vertical_motion_overridden(GameObject* player, PlayerState* st, const MoveStart* start) {
  float expected = start->velocityY;
  float dt = *game.timeDelta;

  if ((st->baddieFlags0 & BADDIE_FLAG_NO_GRAVITY) == 0) {
    expected = game.powfBitEstimate(VELOCITY_Y_DAMPING, dt) * expected;
    expected = expected - st->gravity * dt;
  }
  if (expected < -VELOCITY_Y_LIMIT) {
    expected = -VELOCITY_Y_LIMIT;
  } else if (expected > VELOCITY_Y_LIMIT) {
    expected = VELOCITY_Y_LIMIT;
  }
  return fabsf(player->velocityY - expected) > VELOCITY_Y_MATCH_EPSILON;
}

/* 1 when there is floor below, 0 when there is none, -1 when the collision
   result cannot tell. Queried at the position the main move started from, as
   the cheat menu did. */
static int noclip_floor_below(GameObject* player, PlayerState* st, const MoveStart* start) {
  float depth;

  if (!(st->resultFloorY <= NO_FLOOR_Y)) {
    return 1;
  }
  if (st->resultFloorGap <= 0.0f) {
    return -1;
  }
  return game.trackGetHeightAboveGround(player, start->x, start->y, start->z, &depth, NOCLIP_FLOOR_QUERY_MASK) != 0;
}

/* Runs inside playerUpdate right after the main objMove, at the entry of
   playerDoEyeAnims. The cheat menu ran this between
   playerUpdateVelocityFromMotion and the velocity clamp; nothing in between
   reads what it changes, so the player leaves this point exactly as it did. */
/* The cheat menu's update. Returns 1 when another mod drove the vertical
   motion this frame. */
static int noclip_hold(GameObject* player, PlayerState* st, const MoveStart* start) {
  int floor;

  noclip_keep_inside_map(player, st, start);
  if ((st->flags3F0 & FLAGS3F0_SWIMMING) || player->parent != NULL || warp_active()) {
    sNoclipHold = 0;
    return 0;
  }
  if (vertical_motion_overridden(player, st, start)) {
    sNoclipHold = 0;
    return 1;
  }
  floor = noclip_floor_below(player, st, start);
  if (floor > 0 || (floor < 0 && !sNoclipHold)) {
    sNoclipHold = 0;
    return 0;
  }
  if (!sNoclipHold || sNoclipHoldPlayer != player) {
    sNoclipHold = 1;
    sNoclipHoldPlayer = player;
    sNoclipHoldY = start->y;
  }
  player->localPosY = sNoclipHoldY;
  player->velocityY = 0.0f;
  st->flags3F0 &= (uint8_t)~FLAGS3F0_B08;
  st->flags3F0 &= (uint8_t)~FLAGS3F0_B04;
  st->staffHoldFrames = 0;
  return 0;
}

static void void_reset(void) {
  sVoid.falling = 0;
  sVoid.time = 0.0f;
  sVoid.pending = 0;
}

static int in_safe_area(void) {
  return sSafe.valid && !warp_active() && *game.gGameLoopPendingMapId == sSafe.mapId &&
         game.getCurMapLayer() == sSafe.layer;
}

/* Void recovery. The floor hold only prevents falling where nothing is below;
   when the floor query reports geometry somewhere below (up to 10000 units
   down), the hold lets the player fall toward it. Scenery outside the play
   area can report such floors without ever catching the player, who then
   falls past them and is held in empty space far below the level with no
   way back. A fall counts only when it is the game's own (not held, not driven
   by another mod) and started with Noclip on; it becomes a void once there has
   been no collision floor for VOID_FALL_TIME, the player is VOID_FALL_DROP
   below where the fall began, and no floor at all remains below. Standing on
   collision floor refreshes the safe position and ends any fall, so falling
   frames never overwrite it. */
static void void_track(GameObject* player, PlayerState* st, const MoveStart* start, int overridden) {
  float depth;

  if (sSafe.valid && !in_safe_area()) {
    sSafe.valid = 0;
  }
  if (sVoid.player != player) {
    void_reset();
    sSafe.valid = 0;
    sVoid.player = player;
  }
  if (warp_active() || player->parent != NULL || (st->flags3F0 & FLAGS3F0_SWIMMING) || overridden) {
    void_reset();
    return;
  }
  if ((st->flags3F1 & FLAGS3F1_ON_GROUND) && !(st->resultFloorY <= NO_FLOOR_Y)) {
    void_reset();
    if (sNoclip && !sNoclipHold && game.isInBounds(start->x, start->z) == 1 &&
        (st->flags360 & PLAYER_FLAG_TELEPORT_HOLD) == 0) {
      sSafe.valid = 1;
      sSafe.x = start->x;
      sSafe.y = start->y;
      sSafe.z = start->z;
      sSafe.mapId = *game.gGameLoopPendingMapId;
      sSafe.layer = game.getCurMapLayer();
    }
    return;
  }
  if (!sVoid.falling) {
    if (!sNoclip || sNoclipHold || !(player->localPosY < start->y)) {
      return;
    }
    sVoid.falling = 1;
    sVoid.startY = start->y;
    sVoid.time = 0.0f;
  }
  sVoid.time += *game.timeDelta;
  if (sVoid.pending || !sSafe.valid || sVoid.time < VOID_FALL_TIME ||
      sVoid.startY - player->localPosY < VOID_FALL_DROP) {
    return;
  }
  if (game.trackGetHeightAboveGround(player, player->localPosX, player->localPosY, player->localPosZ, &depth,
                                     NOCLIP_FLOOR_QUERY_MASK) == 0) {
    sVoid.pending = 1;
  }
}

/* Runs inside playerUpdate right after the main objMove, at the entry of
   playerDoEyeAnims. The cheat menu ran this between
   playerUpdateVelocityFromMotion and the velocity clamp; nothing in between
   reads what it changes, so the player leaves this point exactly as it did.
   A fall that began with Noclip on is still followed after Noclip is turned
   off. */
void noclipUpdate(GameObject* player) {
  MoveStart start = sMoveStart;
  PlayerState* st;
  int overridden;

  sMoveStart.player = NULL;
  if (player == NULL || start.player != player || player->extra == NULL) {
    sNoclipHold = 0;
    return;
  }
  st = player->extra;
  if (noclip_active(player)) {
    overridden = noclip_hold(player, st, &start);
  } else {
    sNoclipHold = 0;
    if (!sVoid.falling || player != game.Obj_GetPlayerObject() || !noclip_allowed(player, st)) {
      void_reset();
      return;
    }
    overridden = vertical_motion_overridden(player, st, &start);
  }
  void_track(player, st, &start, overridden);
}

/* Called between frames (fh_mod_update), where Fly Mode's safe return also
   runs, with the same teleport sequence the cheat menu used. */
void noclipRecoverFromVoid(void) {
  GameObject* player;
  PlayerState* st;
  float position[3];

  if (!sVoid.pending) {
    return;
  }
  sVoid.pending = 0;
  player = game.Obj_GetPlayerObject();
  if (player == NULL || player != sVoid.player || (st = player->extra) == NULL || !in_safe_area() ||
      player->parent != NULL || !noclip_allowed(player, st)) {
    void_reset();
    return;
  }
  player->velocityX = 0.0f;
  player->velocityY = 0.0f;
  player->velocityZ = 0.0f;
  st->flags3F0 &= (uint8_t)~FLAGS3F0_B04;
  st->flags3F0 &= (uint8_t)~FLAGS3F0_B08;
  st->staffHoldFrames = 0;
  position[0] = sSafe.x;
  position[1] = sSafe.y;
  position[2] = sSafe.z;
  game.playerTeleport(player, position, NULL, 0);
  game.objSetPos(player, sSafe.x, sSafe.y, sSafe.z);
  game.playerTeleport(player, NULL, NULL, 0);
  sNoclipHold = 0;
  void_reset();
  modLog(FH_LOG_WARN, "void recovery: returned to last safe position");
}

static void* player_collision(PlayerState* st) { return (uint8_t*)st + PLAYER_STATE_CURVES_COLLISION; }

/* playerDoHitDetection is the only caller that passes the player with its own
   collision state, right before the path control update, apply and advance.
   Other objects (baddies keep theirs at the same offset) are ignored. */
void noclipBeginCollision(GameObject* obj, void* collision) {
  if (!sNoclip || obj == NULL || obj != game.Obj_GetPlayerObject() || obj->extra == NULL ||
      collision != player_collision(obj->extra)) {
    return;
  }
  sNoclipCollision = noclip_active(obj);
  if (sNoclipCollision) {
    sNoclipCollisionPlayer = obj;
    sNoclipPosX = obj->localPosX;
    sNoclipPosZ = obj->localPosZ;
  }
}

/* Moves the per-segment trace starts to the restored position so the next
   collision pass does not sweep back across the wall the push was cancelled
   for. traceStart has room for four segments. */
static void noclip_reanchor_traces(GameObject* player, PlayerState* st) {
  int count = st->pointCounts >> CURVES_POINT_COUNT_SEGMENT_SHIFT;
  int i;

  if (count > CURVES_TRACE_POINTS) {
    count = CURVES_TRACE_POINTS;
  }
  for (i = 0; i < count; i++) {
    st->traceStart[i][0] = player->worldPosX;
    st->traceStart[i][2] = player->worldPosZ;
  }
  game.playerRefreshCollisionState(player, st, NOCLIP_SYNC_LOCAL_POINTS);
}

void noclipEndCollision(GameObject* obj, void* collision) {
  float dx;
  float dz;

  if (!sNoclipCollision || obj != sNoclipCollisionPlayer || collision != player_collision(obj->extra)) {
    return;
  }
  sNoclipCollision = 0;
  sNoclipCollisionPlayer = NULL;
  dx = obj->localPosX - sNoclipPosX;
  dz = obj->localPosZ - sNoclipPosZ;
  obj->localPosX = sNoclipPosX;
  obj->localPosZ = sNoclipPosZ;
  if (obj->parent != NULL) {
    game.Obj_TransformLocalPointToWorld(obj->localPosX, obj->localPosY, obj->localPosZ, &obj->worldPosX,
                                        &obj->worldPosY, &obj->worldPosZ, obj->parent);
  } else {
    obj->worldPosX = obj->localPosX;
    obj->worldPosZ = obj->localPosZ;
  }
  if (dx * dx + dz * dz > NOCLIP_PUSH_EPSILON_SQ) {
    noclip_reanchor_traces(obj, obj->extra);
  }
}

/* While Noclip is active, playerCheckIfClimbingOntoWall keeps only probe 11.
   The skipped probes are refused at their trace, which returns no hit, so each
   one continues exactly as if it had been skipped before tracing. The vehicle
   mount check after the probes does not trace and is unaffected. */
GameObject* noclipBeginProbes(GameObject* obj) {
  GameObject* outer = sProbePlayer;

  sProbePlayer = noclip_active(obj) ? obj : NULL;
  return outer;
}

void noclipEndProbes(GameObject* outer) { sProbePlayer = outer; }

int noclipSkipProbeTrace(GameObject* self, float radius, int mode, int8_t flags, int8_t mask, int slot, int8_t arg10) {
  if (sProbePlayer == NULL || self != sProbePlayer) {
    return 0;
  }
  if (radius != 0.0f || mode != CLIMB_PROBE_MODE || flags != CLIMB_PROBE_FLAGS || slot != CLIMB_PROBE_SLOT ||
      arg10 != CLIMB_PROBE_ARG10 || mask < 0 || mask >= 32) {
    return 0;
  }
  return (CLIMB_PROBE_SKIPPED_MASKS & (1u << mask)) != 0;
}

/* Which participants ObjHits_ApplyPairResponse displaces, in its own order. */
static int pair_mover(GameObject* objA, GameObject* objB) {
  const ObjHitsState* hitA = objA->hitReactState;
  const ObjHitsState* hitB = objB->hitReactState;

  if (objA->classId == PAIR_RESPONSE_LATERAL_CLASS && hitA->lateralResponseWeight != 0 &&
      (hitB->flags & OBJHITS_PRIORITY_STATE_IMMOVABLE) == 0) {
    return PAIR_MOVES_A;
  }
  if (objB->classId == PAIR_RESPONSE_LATERAL_CLASS && hitB->lateralResponseWeight != 0 &&
      (hitA->flags & OBJHITS_PRIORITY_STATE_IMMOVABLE) == 0) {
    return PAIR_MOVES_B;
  }
  if (hitB->lateralResponseWeight == 0) {
    return hitA->lateralResponseWeight != 0 ? PAIR_MOVES_A : PAIR_MOVES_NONE;
  }
  if (hitA->lateralResponseWeight == 0) {
    return PAIR_MOVES_B;
  }
  return PAIR_MOVES_BOTH;
}

/* The cheat menu gave the Noclip participant a push of (0, y, 0) and the other
   one the full push. When only the player moves, the push itself becomes
   (0, y, 0). When both move, the shared push still sets the blend and the other
   object's share, so the player's part is fixed separately: a parented player
   gets (0, y, 0) where the push is turned into its parent's space, and an
   unparented player, whose share is the push itself, keeps its X/Z. The only
   contact callback the game registers (the landed Arwing's) does not move
   either object or change what decides the branch. */
void noclipPairResponse(GameObject* objA, GameObject* objB, float x, float y, float z, int flag,
                        PairResponseFn original) {
  GameObject* player;
  PairVector outer;
  int playerIsA;
  int mover;
  float keepX;
  float keepZ;

  if (noclip_active(objA)) {
    player = objA;
    playerIsA = 1;
  } else if (noclip_active(objB)) {
    player = objB;
    playerIsA = 0;
  } else {
    original(objA, objB, x, y, z, flag);
    return;
  }
  mover = pair_mover(objA, objB);
  if (mover == PAIR_MOVES_NONE || mover == (playerIsA ? PAIR_MOVES_B : PAIR_MOVES_A)) {
    original(objA, objB, x, y, z, flag);
    return;
  }
  if (mover != PAIR_MOVES_BOTH) {
    original(objA, objB, 0.0f, y, 0.0f, flag);
    return;
  }
  if (player->parent != NULL) {
    outer = sPairVector;
    sPairVector.armed = 1;
    sPairVector.skip = !playerIsA && objA->parent == player->parent;
    sPairVector.parent = player->parent;
    sPairVector.x = x;
    sPairVector.y = y;
    sPairVector.z = z;
    original(objA, objB, x, y, z, flag);
    sPairVector = outer;
    return;
  }
  keepX = player->localPosX;
  keepZ = player->localPosZ;
  original(objA, objB, x, y, z, flag);
  player->localPosX = keepX;
  player->localPosZ = keepZ;
  player->worldPosX = keepX;
  player->worldPosZ = keepZ;
}

/* Obj_TransformWorldVectorToLocal turns the push into each parented
   participant's space, A first. Only the armed player's conversion changes. */
void noclipPairVector(float* x, float y, float* z, GameObject* parent) {
  if (!sPairVector.armed || parent != sPairVector.parent || *x != sPairVector.x || y != sPairVector.y ||
      *z != sPairVector.z) {
    return;
  }
  if (sPairVector.skip) {
    sPairVector.skip = 0;
    return;
  }
  sPairVector.armed = 0;
  *x = 0.0f;
  *z = 0.0f;
}

/* objSetPos places the player outright (Fly Mode's safe return and the game's
   own teleports use it). The cheat menu dropped the hold after its safe
   return, so a height held before the jump is never applied after it. A fall
   being followed for void recovery ends there too. */
void noclipPlayerMoved(GameObject* obj) {
  if (obj != NULL && obj == sNoclipHoldPlayer) {
    sNoclipHold = 0;
  }
  if (obj != NULL && obj == sVoid.player) {
    void_reset();
  }
}

/* A fixed-angle camera zone keeps a raw pointer to its staticcamera anchor
   until one of the zone's camera triggers switches the camera back. Walls
   normally make the player cross such a trigger before the anchor's area
   unloads; walking out through a wall does not, the anchor is freed, and the
   camera then reads freed memory. Right before the camera reads it, check
   that the anchor is still one of the live static camera objects (the list
   the camera found it in) and, if not, mark it missing, which makes the
   camera take its own fallback to the default camera. Armed for the rest of
   the save once Noclip has been on, since the camera can outlive turning
   Noclip off. */
void noclipCheckStaticCamera(void) {
  StaticCameraState* state;
  GameObject** anchors;
  int count = 0;
  int i;

  if (!sCameraGuard || (state = *game.gCameraModeStaticState) == NULL || state->missingAnchor) {
    return;
  }
  anchors = game.objGetAllOfType(STATIC_CAMERA_OBJECT_GROUP, &count);
  for (i = 0; anchors != NULL && i < count; i++) {
    if (anchors[i] == state->anchor) {
      return;
    }
  }
  state->missingAnchor = 1;
  modLog(FH_LOG_WARN, "fixed camera released: its anchor object was unloaded");
}

void noclipReset(void) {
  set_noclip(0);
  sCameraGuard = 0;
  sSessionActive = 0;
  sNoclipToggleWasDown = 0;
  clear_frame_state();
  memset(&sPairVector, 0, sizeof(sPairVector));
}
