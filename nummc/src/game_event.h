#pragma once
#include "game_common.h"
#include "collision_manager.h"
#include "animation_manager.h"
#include "unit_manager.h"

#define EVENT_MESSAGE_MAX  256

typedef struct _game_event_t game_event_t;
struct _game_event_t {
	int id;
	void* param;
};

// param common template
typedef struct _game_event_param_t game_event_param_t;
struct _game_event_param_t {
	void* data_field[2];
};

extern int game_event_init();
extern void game_event_clear();
extern game_event_param_t* game_event_get_new_param();
extern int game_event_push(game_event_t* msg);
extern int game_event_pop(game_event_t* msg);

//
// <EVENT_MSG>
//
// 0x 00 00 00 00 00 00 00 00
//   32    24    16     8    bit
//   |<--------->|
//   |           |<--->|
//   |           |     |<--->|
//   |           |     |
//   |           |     |
//   |           |     +- VAL1(FLAG,GROUP)
//   |           +- VAL2(GROUP)
//   |
//   +-EVENT_MSG_ID

#define EVENT_MSG_UNIT_PLAYER  0x00010000
#define EVENT_MSG_UNIT_PLAYER_ATTACK_CMD  (EVENT_MSG_UNIT_PLAYER | ANIM_STAT_FLAG_ATTACK)

#define EVENT_MSG_UNIT_ENEMY   0x00020000
#define EVENT_MSG_UNIT_ENEMY_ATTACK1_CMD   (EVENT_MSG_UNIT_ENEMY | ANIM_STAT_FLAG_ATTACK1)
#define EVENT_MSG_UNIT_ENEMY_ATTACK2_CMD   (EVENT_MSG_UNIT_ENEMY | ANIM_STAT_FLAG_ATTACK2)
#define EVENT_MSG_UNIT_ENEMY_ATTACK_CMD    EVENT_MSG_UNIT_ENEMY_ATTACK1_CMD

#define EVENT_MSG_UNIT_ITEMS   0x00040000
#define EVENT_MSG_UNIT_ITEMS_SPAWN_CMD (EVENT_MSG_UNIT_ITEMS | ANIM_STAT_FLAG_SPAWN)
#define EVENT_MSG_UNIT_ITEMS_DIE_CMD   (EVENT_MSG_UNIT_ITEMS | ANIM_STAT_FLAG_DIE)

#define EVENT_MSG_UNIT_TRAP   0x00080000
#define EVENT_MSG_UNIT_TRAP_SPAWN_CMD (EVENT_MSG_UNIT_TRAP | ANIM_STAT_FLAG_SPAWN)
#define EVENT_MSG_UNIT_TRAP_DIE_CMD   (EVENT_MSG_UNIT_TRAP | ANIM_STAT_FLAG_DIE)

#define EVENT_MSG_UNIT_EFFECT   0x00100000
#define EVENT_MSG_UNIT_EFFECT_DIE_CMD   (EVENT_MSG_UNIT_EFFECT | ANIM_STAT_FLAG_DIE)

#define EVENT_MSG_UNIT_PLAYER_BULLET  0x00200000
#define EVENT_MSG_UNIT_PLAYER_BULLET_DIE_CMD  (EVENT_MSG_UNIT_PLAYER_BULLET | ANIM_STAT_FLAG_DIE)

#define EVENT_MSG_UNIT_ENEMY_BULLET  0x00400000
#define EVENT_MSG_UNIT_ENEMY_BULLET_DIE_CMD  (EVENT_MSG_UNIT_ENEMY_BULLET | ANIM_STAT_FLAG_DIE)

typedef struct _game_event_unit_t game_event_unit_t;
struct _game_event_unit_t {
	unit_data_t* obj1;
};

#define EVENT_MSG_COLLISION_DYNAMIC  0x01000000
#define EVENT_MSG_COLLISION_DYNAMIC_PvI   (EVENT_MSG_COLLISION_DYNAMIC | (COLLISION_GROUP_PLAYER << 8)        | COLLISION_GROUP_ITEMS)
#define EVENT_MSG_COLLISION_DYNAMIC_PvT   (EVENT_MSG_COLLISION_DYNAMIC | (COLLISION_GROUP_PLAYER << 8)        | COLLISION_GROUP_TRAP)
#define EVENT_MSG_COLLISION_DYNAMIC_EvT   (EVENT_MSG_COLLISION_DYNAMIC | (COLLISION_GROUP_ENEMY << 8)         | COLLISION_GROUP_TRAP)
#define EVENT_MSG_COLLISION_DYNAMIC_PvE   (EVENT_MSG_COLLISION_DYNAMIC | (COLLISION_GROUP_PLAYER << 8)        | COLLISION_GROUP_ENEMY)
#define EVENT_MSG_COLLISION_DYNAMIC_PvEB  (EVENT_MSG_COLLISION_DYNAMIC | (COLLISION_GROUP_PLAYER << 8)        | COLLISION_GROUP_ENEMY_BULLET)
#define EVENT_MSG_COLLISION_DYNAMIC_PBvE  (EVENT_MSG_COLLISION_DYNAMIC | (COLLISION_GROUP_PLAYER_BULLET << 8) | COLLISION_GROUP_ENEMY)
#define EVENT_MSG_COLLISION_DYNAMIC_PBvM  (EVENT_MSG_COLLISION_DYNAMIC | (COLLISION_GROUP_PLAYER_BULLET << 8) | COLLISION_GROUP_MAP)
#define EVENT_MSG_COLLISION_DYNAMIC_EBvP  (EVENT_MSG_COLLISION_DYNAMIC | (COLLISION_GROUP_ENEMY_BULLET << 8)  | COLLISION_GROUP_PLAYER)
#define EVENT_MSG_COLLISION_DYNAMIC_EBvM  (EVENT_MSG_COLLISION_DYNAMIC | (COLLISION_GROUP_ENEMY_BULLET << 8)  | COLLISION_GROUP_MAP)

typedef struct _game_event_collision_dynamic_t game_event_collision_dynamic_t;
struct _game_event_collision_dynamic_t {
	shape_data* obj1;
	shape_data* obj2;
};
