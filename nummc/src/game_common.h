#pragma once
#include <string>
#include "SDL.h"
#include "SDL_image.h"
#include "SDL_mixer.h"
#include "SDL_ttf.h"

#define GAME_VERSION "nummc ver.1.01.000\n"

#define _COLLISION_ENABLE_BOX_2D_
//#define _MAP_OFFSET_ENABLE_

#define LENGTH_OF(_X) (sizeof(_X)/sizeof(_X[0]))
#define MAX(a,b) ((a) > (b) ? (a) : (b))
#define MIN(a,b) ((a) < (b) ? (a) : (b))
#define ABS(x) (((x) < 0) ? -(x) : (x))
#define FLOAT_NEAR_ZERO  (0.0005f)

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
#define UNIT_ITEMS_LIST_SIZE           32
#define UNIT_TRAP_LIST_SIZE            64
#define UNIT_EFFECT_LIST_SIZE         256  // (PLAYER * UNIT_EFFECT_ID_P_END) + (ENEMY_LIST_SIZE * UNIT_EFFECT_ID_E_END) + others(item effect, trap effect ...)
#define UNIT_PLAYER_BULLET_LIST_SIZE   32
#define UNIT_ENEMY_BULLET_LIST_SIZE   128  // ENEMY_LIST_SIZE * TRIPLE
typedef void void_func();
typedef void void_p_func(void*);
typedef void event_func(SDL_Event*);
typedef void int_func(int);
typedef int ret_int_func();

extern std::string g_base_path;
