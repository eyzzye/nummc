#pragma once
#include "game_common.h"
#include "collision_manager.h"
#include "animation_manager.h"
#include "unit_manager.h"

#define TILE_TYPE_NONE      0
#define TILE_TYPE_BASE      1
#define TILE_TYPE_INSTANCE  2
#define TILE_TYPE_END       3

#define TILE_BREAKABLE_FALSE  0
#define TILE_BREAKABLE_TRUE   1

#define MAP_TYPE_NONE    0
#define MAP_TYPE_FIELD   1
#define MAP_TYPE_BLOCK   2
#define MAP_TYPE_EFFECT  3
#define MAP_TYPE_END     4

typedef struct _tile_data_t tile_data_t;
typedef struct _tile_base_data_t tile_base_data_t;
typedef struct _tile_instance_data_t tile_instance_data_t;
typedef struct _map_data_t map_data_t;
typedef struct _map_field_data_t map_field_data_t;
typedef struct _map_block_data_t map_block_data_t;
typedef struct _map_effect_data_t map_effect_data_t;

//
// Tile
//
struct _tile_data_t {
	int type;        // NONE:0, TILE:6
	int id;          // tile_id
	void* obj;       // object address
	int tile_type;   // NONE:0, BASE:1, INSTANCE:2

	struct _unit_data_t* prev;
	struct _unit_data_t* next;
	shape_data* col_shape;
	anim_data_t* anim;

	unit_data_t* base;
	int* stat_timer;
	int stat;
	int breakable;

	Uint32 data_field[16];
};

struct _tile_base_data_t {
	int type;        // NONE:0, TILE:6
	int id;          // tile_id
	void* obj;       // object address
	int tile_type;   // BASE:1

	struct _unit_data_t* prev;
	struct _unit_data_t* next;
	shape_data* col_shape;
	anim_data_t* anim;

	unit_data_t* base;
	int* stat_timer;
	int stat;
	int breakable;
};

struct _tile_instance_data_t {
	int type;        // NONE:0, TILE:6
	int id;          // tile_id
	void* obj;       // tile_base address
	int tile_type;   // INSTANCE:2

	struct _unit_data_t* prev;
	struct _unit_data_t* next;
	shape_data* col_shape;
	anim_data_t* anim;

	unit_data_t* base;
	int* stat_timer;
	int stat;
	int breakable;
};

//
// Map
//
struct _map_data_t {
	int type;        // NONE:0, FIELD:1, BLOCK:2, EFFECT:3
	int id;          // map_id_end
	void* obj;       // object address
	int reserve0;

	map_data_t* prev;
	map_data_t* next;
	int reserv1;
	int reserv2;

	Uint32 data_field[16];
};

struct _map_field_data_t {
	int type;        // FIELD:1
	int id;          // map_id_end
	void* obj;       // object address
	int reserve0;

	map_data_t* prev;
	map_data_t* next;
	int reserv1;
	int reserv2;

	int layer;
	int x;
	int y;
	tile_instance_data_t* map_raw_data;
};

struct _map_block_data_t {
	int type;        // BLOCK:2
	int id;          // map_id_end
	void* obj;       // object address
	int reserve0;

	map_data_t* prev;
	map_data_t* next;
	int reserv1;
	int reserv2;

	int layer;
	int x;
	int y;
	tile_instance_data_t* map_raw_data;
};

struct _map_effect_data_t {
	int type;        // EFFECT:3
	int id;          // map_id_end
	void* obj;       // object address
	int reserve0;

	map_data_t* prev;
	map_data_t* next;
	int reserv1;
	int reserv2;

	int layer;
	int x;
	int y;
	tile_instance_data_t* map_raw_data;
};

#ifdef _MAP_OFFSET_ENABLE_
extern int g_map_offset_x;
extern int g_map_offset_y;
#else
extern const int g_map_offset_x;
extern const int g_map_offset_y;
#endif
extern int g_map_x_max;
extern int g_map_y_max;
extern int g_tile_width;
extern int g_tile_height;

extern int map_manager_init();
extern void map_manager_unload();
extern void map_manager_update();
extern void map_manager_display(int layer);
#ifdef _MAP_OFFSET_ENABLE_
extern void map_manager_set_offset(int x, int y);
#endif
extern int map_manager_load(std::string path);
extern shape_data* map_manager_get_col_shape(int x, int y);
extern void map_manager_create_instance();
extern void map_manager_create_wall();
extern void map_manager_break_block(int x, int y, int w, int h);
