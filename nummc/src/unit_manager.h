#pragma once
#include "game_common.h"
#include "collision_manager.h"
#include "animation_manager.h"
#include "ai_manager.h"

#define UNIT_TAG_UNIT       0
#define UNIT_TAG_COLLISION  1
#define UNIT_TAG_ANIM       2
#define UNIT_TAG_AI         3
#define UNIT_TAG_BULLET     4
#define UNIT_TAG_END        5

#define UNIT_TYPE_NONE           0
#define UNIT_TYPE_PLAYER         1
#define UNIT_TYPE_ENEMY          2
#define UNIT_TYPE_ITEMS          3
#define UNIT_TYPE_PLAYER_BULLET  4
#define UNIT_TYPE_ENEMY_BULLET   5
#define UNIT_TYPE_TILE           6
#define UNIT_TYPE_TRAP           7
#define UNIT_TYPE_EFFECT         8
#define UNIT_TYPE_END            9

#define UNIT_STAT_DAMAGE      0
#define UNIT_STAT_RECOVERY    1
#define UNIT_STAT_SPECIAL     2
#define UNIT_STAT_INVINCIBLE  3
#define UNIT_STAT_END         4

#define UNIT_STAT_FLAG_NONE        (0x00000000)
#define UNIT_STAT_FLAG_DAMAGE      (0x00000001 << UNIT_STAT_DAMAGE)
#define UNIT_STAT_FLAG_RECOVERY    (0x00000001 << UNIT_STAT_RECOVERY)
#define UNIT_STAT_FLAG_SPECIAL     (0x00000001 << UNIT_STAT_SPECIAL)
#define UNIT_STAT_FLAG_INVINCIBLE  (0x00000001 << UNIT_STAT_INVINCIBLE)

#define UNIT_ANIM_BLINK_TIMER       200

#define UNIT_BULLET_TYPE_NONE     0
#define UNIT_BULLET_TYPE_ICE      1
#define UNIT_BULLET_TYPE_FIRE     2

#define UNIT_BULLET_TRACK_NONE    0
#define UNIT_BULLET_TRACK_LINE    1
#define UNIT_BULLET_TRACK_RADIAL  2
#define UNIT_BULLET_TRACK_WAVE    3
#define UNIT_BULLET_TRACK_CROSS   4
#define UNIT_BULLET_TRACK_XCROSS  5
#define UNIT_BULLET_TRACK_RANDOM  6
#define UNIT_BULLET_TRACK_END     7

#define UNIT_BULLET_NUM_NONE       0
#define UNIT_BULLET_NUM_SINGLE     1
#define UNIT_BULLET_NUM_DOUBLE     2
#define UNIT_BULLET_NUM_TRIPLE     3
#define UNIT_BULLET_NUM_QUADRUPLE  4
#define UNIT_BULLET_NUM_MAX        5

#define UNIT_BULLET_SPEC_SET_NUM(_X,_VAL)      (_X)->bullet_spec = (((_X)->bullet_spec & 0xFFFFFF00) | _VAL)
#define UNIT_BULLET_SPEC_GET_NUM(_X)           ((_X)->bullet_spec & 0x000000FF)
#define UNIT_BULLET_SPEC_SET_CURVING(_X,_VAL)  (_X)->bullet_spec = (((_X)->bullet_spec & 0xFFFF00FF) | ((_VAL << 8) & 0x0000FF00))
#define UNIT_BULLET_SPEC_GET_CURVING(_X)       (((_X)->bullet_spec & 0x0000FF00) >> 8)
#define UNIT_BULLET_SPEC_SET_STRENGTH(_X,_VAL) (_X)->bullet_spec = (((_X)->bullet_spec & 0xFF00FFFF) | ((_VAL << 16) & 0x00FF0000))
#define UNIT_BULLET_SPEC_GET_STRENGTH(_X)      (((_X)->bullet_spec & 0x00FF0000) >> 16)

#define UNIT_SPEC_SET_LUCK(_X,_VAL)            (_X)->spec = (((_X)->spec & 0xFFFFFF00) | _VAL)
#define UNIT_SPEC_GET_LUCK(_X)                 ((_X)->spec & 0x000000FF)

#include "unit_manager_player.h"
#include "unit_manager_enemy.h"
#include "unit_manager_items.h"
#include "unit_manager_trap.h"
#include "unit_manager_effect.h"
#include "unit_manager_player_bullet.h"
#include "unit_manager_enemy_bullet.h"

typedef struct _unit_data_t unit_data_t;
typedef struct _unit_player_data_t unit_player_data_t;
typedef struct _unit_enemy_data_t unit_enemy_data_t;
typedef struct _unit_items_data_t unit_items_data_t;
typedef struct _unit_trap_data_t unit_trap_data_t;
typedef struct _unit_effect_data_t unit_effect_data_t;
typedef struct _unit_effect_stat_data_t unit_effect_stat_data_t;
typedef struct _unit_player_bullet_data_t unit_player_bullet_data_t;
typedef struct _unit_enemy_bullet_data_t unit_enemy_bullet_data_t;


struct _unit_data_t {
	int type;        // NONE:0, PLAYER:1, ENEMY:2, ITEMS:3, PLAYER_BULLET:4, ENEMY_BULLET:5
	int id;          // unit_id_end
	unit_data_t* prev;
	unit_data_t* next;

	void* obj;       // object address
	shape_data* col_shape;
	anim_data_t* anim;
	int reserve0;

	unit_data_t* base;
	int* stat_timer;
	int stat;
	int reserve2;

	Uint64 data_field[20];
};

struct _unit_player_data_t {
	int type;        // PLAYER:1
	int id;          // unit_id_end
	unit_data_t* prev;
	unit_data_t* next;

	void* obj;       // char* path
	shape_data* col_shape;
	anim_data_t* anim;
	int reserve0;

	unit_player_data_t* base;
	int* stat_timer;
	int stat;
	int reserve1;

	int hp;
	int exp;
	int effect_stat;	                    // FLAG_P_FIRE_UP,FREEZE_UP,BOOST,SHIELD
	unit_effect_stat_data_t* effect_param;  // player_base_effect[UNIT_EFFECT_ID_P_END]

	int resistance_stat;					// FLAG_P_FIRE_UP,FREEZE_UP
	int attack_wait_timer;
	int bullet_life_timer;
	int bullet_spec;                        // | reserve(FF) | strength(FF) | curving(FF) | num(FF)

	int speed;
	int weapon;
	int armor;
	int spec;                               //  | reserve(FFFFFF) | luck(FF)

	int hp_max;
	int exp_max;
	int level;
	char* next_level;
};

struct _unit_enemy_data_t {
	int type;        // ENEMY:2
	int id;          // unit_id_end
	unit_data_t* prev;
	unit_data_t* next;

	void* obj;       // char* path
	shape_data* col_shape;
	anim_data_t* anim;
	int reserve0;

	unit_enemy_data_t* base;
	int* stat_timer;
	int stat;
	int reserve1;

	int hp;
	int exp;
	int bullet_life_timer;
	int bullet_curving;

	int speed;
	int strength;
	int weapon;
	int armor;

	int hp_max;
	int exp_max;
	int level;
	char* next_level;

	int drop_item;
	int effect_stat;	                    // FLAG_E_FIRE_UP,FREEZE_UP
	unit_effect_stat_data_t* effect_param;  // player_base_effect[UNIT_EFFECT_ID_P_END]
	int resistance_stat;					// FLAG_E_FIRE_UP,FREEZE_UP

	ai_data_t* ai;
	ai_data_t* bullet[UNIT_ENEMY_BULLET_NUM];
	int reserve3;
};

struct _unit_items_data_t {
	int type;        // ITEMS:3
	int id;          // unit_id_end
	unit_data_t* prev;
	unit_data_t* next;

	void* obj;       // char* path
	shape_data* col_shape;
	anim_data_t* anim;
	int group;       // item group

	unit_items_data_t* base;
	int* stat_timer;
	int stat;
	int reserve0;

	int item_id;
	int sub_id;
	int hp;
	int time;

	int val1;
	int val2;
	int val3;
	int val4;

	int val5;
	int val6;
	int reserve1;
	int reserve2;
};

struct _unit_trap_data_t {
	int type;        // TRAP:7
	int id;          // unit_id_end
	unit_data_t* prev;
	unit_data_t* next;

	void* obj;       // char* path
	shape_data* col_shape;
	anim_data_t* anim;
	int group;       // item group

	unit_trap_data_t* base;
	int* stat_timer;
	int stat;
	int reserve0;

	int sub_id;
	int hp;
	unit_data_t* trace_unit;
	int reserve2;
};

struct _unit_effect_data_t {
	int type;        // EFFECT:8
	int id;          // unit_id_end
	unit_data_t* prev;
	unit_data_t* next;

	void* obj;       // char* path
	shape_data* col_shape;
	anim_data_t* anim;
	int group;       // STATIC, DYNAMIC, DIE_AUTO

	unit_effect_data_t* base;
	int* stat_timer;
	int stat;
	int reserve0;

	int life_timer;
	unit_data_t* trace_unit;
	int clear_type;  // NONE, KEEP_ON_STAGE
	int reserve3;
};

struct _unit_effect_stat_data_t {
	int id;
	int timer;
	int counter;
	int delta_time;
	int damage;
};

struct _unit_player_bullet_data_t {
	int type;        // PLAYER_BULLET:4
	int id;          // unit_id_end
	unit_data_t* prev;
	unit_data_t* next;

	void* obj;       // char* path
	shape_data* col_shape;
	anim_data_t* anim;
	int reserve0;

	unit_player_bullet_data_t* base;
	int* stat_timer;
	int stat;
	int reserve2;

	int hp;
	int bullet_life_timer;
	int effect_stat;	                    // FLAG_P_FIRE_UP,FREEZE_UP,BOOST,SHIELD
	int reserve3;

	int speed;
	int special;
	int special_value;
	int reserve4;
};

struct _unit_enemy_bullet_data_t {
	int type;        // ENEMY_BULLET:5
	int id;          // unit_id_end
	unit_data_t* prev;
	unit_data_t* next;

	void* obj;       // char* path
	shape_data* col_shape;
	anim_data_t* anim;
	int reserve0;

	unit_enemy_bullet_data_t* base;
	int* stat_timer;
	int stat;
	int reserve2;

	int hp;
	int bullet_life_timer;
	int owner_base_id;
	int effect_stat;	                    // FLAG_E_FIRE_UP,FREEZE_UP

	int speed;
	int special;
	int special_value;
	ai_data_t* ai_bullet; // copy from unit->bullet[]
};

// global variable
extern unit_player_data_t g_player;
extern unit_player_data_t g_player_backup;

extern const char* g_player_bullet_path[];
extern const char* g_enemy_bullet_path[];

// functions
extern int unit_manager_init();
extern int unit_manager_init_player();
extern int unit_manager_init_enemy();
extern int unit_manager_init_items();
extern int unit_manager_init_trap();
extern int unit_manager_init_effect();
extern int unit_manager_init_player_bullet();
extern int unit_manager_init_enemy_bullet();

extern void unit_manager_unload();
extern void unit_manager_unload_player();
extern void unit_manager_unload_enemy();
extern void unit_manager_unload_items();
extern void unit_manager_unload_trap();
extern void unit_manager_unload_effect();
extern void unit_manager_unload_player_bullet();
extern void unit_manager_unload_enemy_bullet();

extern void unit_manager_clear_all_enemy();
extern void unit_manager_clear_all_trap();
extern void unit_manager_clear_all_items();
extern void unit_manager_clear_all_effect(int clear_type);
extern void unit_manager_clear_all_player_bullet();
extern void unit_manager_clear_all_enemy_bullet();

extern int unit_manager_unit_get_anim_stat(unit_data_t* unit_data);
extern void unit_manager_unit_set_anim_stat(unit_data_t* unit_data, int stat);
extern void unit_manager_player_set_anim_stat(int stat);
extern void unit_manager_player_set_effect_stat(int stat, bool off_on);
extern void unit_manager_enemy_set_anim_stat(int unit_id, int stat);
extern void unit_manager_items_set_anim_stat(int unit_id, int stat);
extern void unit_manager_trap_set_anim_stat(int unit_id, int stat);
extern void unit_manager_effect_set_anim_stat(int unit_id, int stat);
extern void unit_manager_player_bullet_set_anim_stat(int unit_id, int stat);
extern void unit_manager_player_bullet_set_effect_stat(int unit_id, int stat);
extern void unit_manager_enemy_bullet_set_anim_stat(int unit_id, int stat);
extern void unit_manager_enemy_bullet_set_effect_stat(int unit_id, int stat);

extern void load_collision(char* line, shape_data** col_shape);
extern void load_anim(char* line, anim_data_t* anim);
extern void load_ai(char* line, ai_data_t* ai_data);
extern void load_bullet(char* line, ai_data_t** bullet_data);

extern void unit_display(unit_data_t* unit_data, int layer);
extern void unit_manager_update_unit_friction(unit_data_t* unit_data);
extern void unit_manager_unit_move(unit_data_t* unit_data, float vec_x, float vec_y, float speed=1.0f);
extern void unit_manager_get_position(unit_data_t* unit_data, int* x, int* y);
extern void unit_manager_get_center_position(unit_data_t* unit_data, int* x, int* y);
extern float unit_manager_get_distance(unit_data_t* main_unit, unit_data_t* target_unit, int* x = NULL, int* y = NULL);
extern int unit_manager_get_face(unit_data_t* unit_data);
extern int unit_manager_get_face_relative(unit_data_t* unit_data, int original_face);
extern int unit_manager_get_face_other_side(unit_data_t* unit_data);
extern void unit_manager_get_face_velocity(float* vec_x, float* vec_y, int face, float abs_velocity, int bullet_track_type = UNIT_BULLET_TRACK_LINE, int bullet_num = UNIT_BULLET_NUM_SINGLE);
extern void unit_manager_get_target_velocity(float* vec_x, float* vec_y,
	unit_data_t* unit_data, int target_x, int target_y,
	float abs_velocity, int bullet_track_type = UNIT_BULLET_TRACK_LINE, int bullet_num = UNIT_BULLET_NUM_SINGLE);
extern void unit_manager_get_bullet_start_pos(unit_data_t* unit_data, unit_data_t* unit_bullet_data, int bullet_track_type, int bullet_num, int face, int* x, int* y);
extern void unit_manager_get_spawn_items_pos(unit_data_t* spawner_unit, unit_data_t* avoid_unit, int item_num, int* x, int* y);
extern void unit_manager_get_spawn_items_pos_under_foot(unit_data_t* spawner_unit, int item_num, int* x, int* y);
extern void unit_manager_get_spawn_items_pos_for_target(unit_data_t* spawner_unit, unit_data_t* target_unit, int item_num, int* x, int* y);

// player
extern void unit_manager_backup_player();
extern void unit_manager_restore_player();
extern void unit_manager_clear_player_backup();
extern int unit_manager_load_player_effects();
extern int unit_manager_load_player(char* path);
extern void unit_manager_create_player(int x, int y);
extern void unit_manager_player_clear_stats();
extern void unit_manager_player_set_stat(int stat);
extern void unit_manager_player_get_face_velocity(float* vec_x, float* vec_y, int face, float abs_velocity, int bullet_track_type = UNIT_BULLET_TRACK_LINE, int bullet_num = UNIT_BULLET_NUM_SINGLE);
extern int unit_manager_player_get_bullet_strength();
extern int unit_manager_player_get_bullet_life_timer();
extern float unit_manager_player_get_bullet_curving();
extern int unit_manager_player_get_luck();
extern int unit_manager_player_get_damage_force(int hp);
extern int unit_manager_player_get_damage(int hp);
extern int unit_manager_player_get_damage_with_bullet(unit_enemy_bullet_data_t* enemy_bullet_data);
extern int unit_manager_player_recovery(int hp);
extern int unit_manager_player_set_hp_max(int hp_max);
extern int unit_manager_player_get_exp(int exp);
extern int unit_manager_player_charge_val(int exp);
extern void unit_manager_player_get_item(unit_items_data_t* item_data);
extern void unit_manager_player_use_weapon_item();
extern void unit_manager_player_use_charge_item();
extern void unit_manager_player_use_special_item();
extern void unit_manager_player_trap(unit_trap_data_t* trap_data);
extern void unit_manager_player_gameover();
extern void unit_manager_player_move(float vec_x, float vec_y);
extern void unit_manager_player_set_position(int x, int y);
extern void unit_manager_player_update();
extern void unit_manager_player_display(int layer);

// enemy
extern int unit_manager_search_enemy(char* path);
extern void unit_manager_set_enemy_slowed(int index);
extern void unit_manager_set_all_enemy_slowed();
extern unit_enemy_data_t* unit_manager_get_enemy(int index);
extern int unit_manager_load_enemy_effects(int unit_id, int base_w = 32);
extern int unit_manager_load_enemy(char* path);
extern int unit_manager_create_enemy(int x, int y, int face, int base_index = -1);
extern void unit_manager_create_hell();
extern void unit_manager_clear_enemy(unit_enemy_data_t* enemy);
extern bool unit_manager_enemy_exist();
extern int unit_manager_enemy_get_enemy_count();
extern int unit_manager_enemy_get_delta_time(unit_enemy_data_t* enemy_data);
extern void unit_manager_enemy_get_face_velocity(unit_enemy_data_t* enemy_data, float* vec_x, float* vec_y, int face, float abs_velocity, int bullet_track_type, int bullet_num);
extern void unit_manager_enemy_get_target_velocity(unit_enemy_data_t* enemy_data, float* vec_x, float* vec_y, int target_x, int target_y, float abs_velocity, int bullet_track_type, int bullet_num);
extern int unit_manager_enemy_get_bullet_strength(int base_id);
extern int unit_manager_enemy_get_bullet_life_timer(int base_id);
extern float unit_manager_enemy_get_bullet_curving(int base_id);
extern void unit_manager_set_ai_step(int index, int step);
extern int unit_manager_enemy_get_damage_force(unit_enemy_data_t* enemy_data, int hp);
extern int unit_manager_enemy_get_damage(unit_enemy_data_t* enemy_data, int hp);
extern int unit_manager_enemy_get_damage_with_bullet(unit_enemy_data_t* enemy_data, unit_player_bullet_data_t* player_bullet_data);
extern void unit_manager_enemy_drop_item(unit_enemy_data_t* unit_data);
extern int unit_manager_enemy_attack(unit_enemy_data_t* enemy_data, int stat);
extern void unit_manager_enemy_trap(unit_enemy_data_t* enemy_data, unit_trap_data_t* trap_data);
extern void unit_manager_enemy_move(unit_enemy_data_t* enemy_data, float vec_x, float vec_y, int speed = -1);
extern void unit_manager_enemy_update();
extern void unit_manager_enemy_ai_update();
extern void unit_manager_enemy_display(int layer);

// items
extern int unit_manager_search_items(char* path);
extern const char* unit_manager_get_special_item(int index);
extern int unit_manager_items_get_val(int unit_id, int index);
extern void unit_manager_items_set_val(int unit_id, int val, int index);
extern void unit_manager_items_fire_bom(unit_items_data_t* item_data);
extern void unit_manager_items_bom_event(unit_items_data_t* item_data);
extern unit_items_data_t* unit_manager_get_items_base(int index);
extern unit_items_data_t* unit_manager_get_items(int index);
extern int unit_manager_load_items_def(char* path);
extern int unit_manager_load_items(char* path);
extern int unit_manager_create_items(int x, int y, int base_index = -1);
extern int unit_manager_create_items_by_sid(int sid, int x, int y);
extern void unit_manager_clear_items(unit_items_data_t* item);
extern void unit_manager_items_register_item_stock();
extern void unit_manager_items_update();
extern void unit_manager_items_display(int layer);

// trap
extern int unit_manager_search_trap(char* path);
extern unit_trap_data_t* unit_manager_get_trap(int index);
extern bool unit_manager_trap_within(int x, int y);
extern int unit_manager_load_trap(char* path);
extern int unit_manager_create_trap(int x, int y, int base_index = -1);
extern void unit_manager_clear_trap(unit_trap_data_t* trap);
extern void unit_manager_trap_update();
extern void unit_manager_trap_display(int layer);

// effect
extern int unit_manager_search_effect(char* path);
extern void unit_manager_effect_set_trace_unit(int unit_id, unit_data_t* unit_data);
extern void unit_manager_effect_set_b2position(int unit_id, float x, float y);
extern unit_effect_data_t* unit_manager_get_effect(int index);
extern int unit_manager_load_effect(char* path);
extern int unit_manager_create_effect(int x, int y, int base_index = -1);
extern void unit_manager_clear_effect(unit_effect_data_t* effect);
extern void unit_manager_effect_update();
extern void unit_manager_effect_display(int layer);

// player_bullet
extern int unit_manager_search_player_bullet(char* path);
extern void unit_manager_player_bullet_set_hp(int unit_id, int hp);
extern void unit_manager_player_bullet_set_bullet_life_timer(int unit_id, int bullet_life_timer);
extern unit_player_bullet_data_t* unit_manager_get_player_bullet_base(int index);
extern int unit_manager_load_player_bullet(char* path);
extern int unit_manager_create_player_bullet(int x, int y, float vec_x, float vec_y, int face, int base_index = -1);
extern void unit_manager_clear_player_bullet(unit_player_bullet_data_t* bullet);
extern void unit_manager_player_bullet_update();
extern void unit_manager_player_bullet_display(int layer);

// enemy_bullet
extern int unit_manager_search_enemy_bullet(char* path);
extern void unit_manager_enemy_bullet_set_hp(int unit_id, int hp);
extern void unit_manager_enemy_bullet_set_bullet_life_timer(int unit_id, int bullet_life_timer);
extern void unit_manager_enemy_bullet_set_force(int unit_id, float strength_x, float strength_y);
extern unit_enemy_bullet_data_t* unit_manager_get_enemy_bullet_base(int index);
extern int unit_manager_enemy_bullet_get_delta_time(unit_enemy_bullet_data_t* enemy_bullet_data);
extern int unit_manager_load_enemy_bullet(char* path);
extern int unit_manager_create_enemy_bullet(int x, int y, float vec_x, float vec_y, int face, int owner_base_id, int base_index = -1, ai_data_t* ai_bullet=NULL);
extern void unit_manager_clear_enemy_bullet(unit_enemy_bullet_data_t* bullet);
extern void unit_manager_enemy_bullet_update();
extern void unit_manager_enemy_bullet_display(int layer);
