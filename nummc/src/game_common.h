#pragma once
#include <string>
#include "SDL.h"
#include "SDL_image.h"
#include "SDL_mixer.h"
#include "SDL_ttf.h"

#define GAME_VERSION "nummc ver.1.03.000\n"

#define _COLLISION_ENABLE_BOX_2D_
//#define _MAP_OFFSET_ENABLE_

#define LENGTH_OF(_X) (sizeof(_X)/sizeof(_X[0]))
#define MAX(a,b) ((a) > (b) ? (a) : (b))
#define MIN(a,b) ((a) < (b) ? (a) : (b))
#define ABS(x) (((x) < 0) ? -(x) : (x))
#define UPPER_BIT(a) (a & 0xFFFF0000)
#define LOWER_BIT(a) (a & 0x0000FFFF)
#define FLOAT_NEAR_ZERO  (0.0005f)

// file
#define GAME_FULL_PATH_MAX  256

// map
#define TILE_TEX_NUM  16

// unit
#define UNIT_FACE_TYPE_NONE  0
#define UNIT_FACE_TYPE_LR    1
#define UNIT_FACE_TYPE_UD    2
#define UNIT_FACE_TYPE_ALL   3
#define UNIT_FACE_TYPE_END   4

#define UNIT_FACE_NONE  0
#define UNIT_FACE_N     1
#define UNIT_FACE_E     2
#define UNIT_FACE_W     3    // default
#define UNIT_FACE_S     4
#define UNIT_FACE_END   5

#define UNIT_ENEMY_LIST_SIZE           32
#define UNIT_ITEMS_LIST_SIZE           64
#define UNIT_TRAP_LIST_SIZE            64
#define UNIT_PLAYER_BULLET_LIST_SIZE   32
#define UNIT_ENEMY_BULLET_LIST_SIZE    (UNIT_ENEMY_LIST_SIZE * 4)  // ENEMY_LIST_SIZE * bullet_num

// trash(32) + trash smoke(32) + high_light_line(32) + goal smoke(1) + item star(64) + item smoke(64)
// + enemy smoke(32) + enemy effect(32*(UNIT_EFFECT_ID_P_END-1) = 32*2) + enemy damage(32*4)
// + player smoke(1) + player effect(UNIT_EFFECT_ID_P_END-1 = 5) + player star(2) + player damage(1)
#define UNIT_EFFECT_LIST_SIZE             ((((32*2 + 32 + 1 + UNIT_ITEMS_LIST_SIZE * 2 + UNIT_ENEMY_LIST_SIZE * (1+2+5) + 8) + 64) / 64) * 64)  // 64bit alignment

#define UNIT_PLAYER_BASE_LIST_SIZE         16
#define UNIT_ENEMY_BASE_LIST_SIZE          (UNIT_ENEMY_LIST_SIZE * 2)
#define UNIT_ITEMS_BASE_LIST_SIZE          (UNIT_ITEMS_LIST_SIZE)
#define UNIT_EFFECT_BASE_LIST_SIZE         32
#define UNIT_TRAP_BASE_LIST_SIZE           (UNIT_TRAP_LIST_SIZE / 4)           // div 4 => rough estimate
#define UNIT_PLAYER_BULLET_BASE_LIST_SIZE  (UNIT_PLAYER_BULLET_LIST_SIZE / 2)  // div 2 => rough estimate
#define UNIT_ENEMY_BULLET_BASE_LIST_SIZE   (UNIT_ENEMY_BULLET_LIST_SIZE / 8)   // div 8 => rough estimate

// ANIM_DATA_LIST_SIZE =
//  all unit base num
//   + TILE_TEX_NUM
//   + PLAYER + ENEMY_LIST_SIZE
//   + UNIT_ITEMS_LIST_SIZE + UNIT_TRAP_LIST_SIZE + UNIT_PLAYER_BULLET_LIST_SIZE + UNIT_ENEMY_BULLET_LIST_SIZE
//   + effect inst num
#define ANIM_DATA_LIST_SIZE ((((UNIT_PLAYER_BASE_LIST_SIZE + UNIT_ENEMY_BASE_LIST_SIZE + UNIT_ITEMS_BASE_LIST_SIZE \
        + UNIT_EFFECT_BASE_LIST_SIZE + UNIT_TRAP_BASE_LIST_SIZE + UNIT_PLAYER_BULLET_BASE_LIST_SIZE + UNIT_ENEMY_BULLET_BASE_LIST_SIZE \
        + TILE_TEX_NUM \
        + 1 + UNIT_ENEMY_LIST_SIZE \
        + UNIT_ITEMS_LIST_SIZE + UNIT_TRAP_LIST_SIZE + UNIT_PLAYER_BULLET_LIST_SIZE + UNIT_ENEMY_BULLET_LIST_SIZE \
        + UNIT_EFFECT_LIST_SIZE) + 64) / 64) * 64)

// node data
#define GAME_UTILS_NODE_TYPE_NONE  0
#define GAME_UTILS_NODE_TYPE_USED  1

typedef struct _node_data_t node_data_t;
struct _node_data_t {
	int type;        // NONE:0
	int id;          // index
	node_data_t* prev;
	node_data_t* next;
};

typedef struct _node_buffer_info_t node_buffer_info_t;
struct _node_buffer_info_t {
	int node_size;            // sizeof(xxx_data_t)
	int buffer_size;          // sizeof(buffer) / node_size
	int used_buffer_size;     // used_buffer < buffer_size
	node_data_t* head_node;   // buffer[]
	node_data_t* start_node;  // prev=NULL data
	node_data_t* end_node;    // next=NULL data
};

// char buffer
#define GAME_UTILS_STRING_TYPE_NONE      0
#define GAME_UTILS_STRING_TYPE_CHAR_BUF  1

#define GAME_UTILS_STRING_LINE_BUF_SIZE  256
#define GAME_UTILS_STRING_CHAR_BUF_SIZE  128
#define GAME_UTILS_STRING_NAME_BUF_SIZE   32
typedef struct _game_utils_string_t game_utils_string_t;
struct _game_utils_string_t {
	int type;        // NONE:0, CHAR_BUF:1
	char buffer[GAME_UTILS_STRING_CHAR_BUF_SIZE];
};

// function pointers
typedef void void_func();
typedef void void_p_func(void*);
typedef void event_func(SDL_Event*);
typedef void int_func(int);
typedef int ret_int_func();

extern int g_base_path_size;
extern char g_base_path[GAME_UTILS_STRING_CHAR_BUF_SIZE];
