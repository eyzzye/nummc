#pragma once
#include "game_common.h"
#include "resource_manager.h"

#define ANIM_DATA_DISABLE  0
#define ANIM_DATA_ENABLE   1

#define ANIM_TYPE_NONE              0
#define ANIM_TYPE_STATIC            1
#define ANIM_TYPE_DYNAMIC           2
//#define ANIM_TYPE_END               3
#define ANIM_TYPE_DRAW              (0x00010000)
#define ANIM_TYPE_FILL              (0x00020000)
#define ANIM_TYPE_DRAW_RECT         (ANIM_TYPE_DRAW | 1)
#define ANIM_TYPE_DRAW_CIRCLE       (ANIM_TYPE_DRAW | 2)
#define ANIM_TYPE_DRAW_RECT_FILL    (ANIM_TYPE_FILL | ANIM_TYPE_DRAW_RECT)

#define ANIM_STAT_IDLE     0
#define ANIM_STAT_MOVE     1
#define ANIM_STAT_ATTACK1  2
#define ANIM_STAT_ATTACK2  3
#define ANIM_STAT_DEFENCE  4
#define ANIM_STAT_DIE      5
#define ANIM_STAT_SPAWN    6
#define ANIM_STAT_HIDE     7
#define ANIM_STAT_END      8

#define ANIM_STAT_FLAG_NONE     (0x00000000)
#define ANIM_STAT_FLAG_IDLE     (0x00000001 << ANIM_STAT_IDLE)
#define ANIM_STAT_FLAG_MOVE     (0x00000001 << ANIM_STAT_MOVE)
#define ANIM_STAT_FLAG_ATTACK1  (0x00000001 << ANIM_STAT_ATTACK1)
#define ANIM_STAT_FLAG_ATTACK2  (0x00000001 << ANIM_STAT_ATTACK2)
#define ANIM_STAT_FLAG_DEFENCE  (0x00000001 << ANIM_STAT_DEFENCE)
#define ANIM_STAT_FLAG_DIE      (0x00000001 << ANIM_STAT_DIE)
#define ANIM_STAT_FLAG_SPAWN    (0x00000001 << ANIM_STAT_SPAWN)
#define ANIM_STAT_FLAG_HIDE     (0x00000001 << ANIM_STAT_HIDE)

#define ANIM_STAT_ATTACK       ANIM_STAT_ATTACK1
#define ANIM_STAT_FLAG_ATTACK  ANIM_STAT_FLAG_ATTACK1

// anim frame
#define ANIM_FRAME_TYPE_NONE  0
#define ANIM_FRAME_TYPE_FRAME 1
#define ANIM_FRAME_TYPE_END   2

#define ANIM_FRAME_NUM_MAX  8

#define ANIM_FRAME_COMMAND_OFF    0
#define ANIM_FRAME_COMMAND_ON     1

// tex
#define ANIM_TEX_LAYER_MIN  1
#define ANIM_TEX_LAYER_MAX  3

// base_size
#define ANIM_BASE_SIZE_32x32  0
#define ANIM_BASE_SIZE_48x48  1
#define ANIM_BASE_SIZE_64x64  2
#define ANIM_BASE_SIZE_END    3

typedef struct _anim_frame_data_t anim_frame_data_t;
typedef struct _anim_stat_base_data_t anim_stat_base_data_t;
typedef struct _anim_stat_data_t anim_stat_data_t;
typedef struct _anim_data_t anim_data_t;

struct _anim_frame_data_t {
	int type;        // NONE:0, FRAME:1
	int id;          // frame_id_end
	anim_frame_data_t* prev;
	anim_frame_data_t* next;

	void* parent;    // anim_stat_base_data_t address
	int frame_time;
	int reserve0;
	int reserve1;

	ResourceImg* res_img;
	ResourceChunk* res_chunk;
	int command;       // OFF:0, ON:1
	int reserve2;

	// int x,y,w,h
	SDL_Rect src_rect;
};

struct _anim_stat_base_data_t {
	int type;        // NONE:0, STATIC:1, DYNAMIC:2
	int id;          // anim_stat_id_end
	anim_stat_base_data_t* prev;
	anim_stat_base_data_t* next;

	void* obj;       // obj address
	int total_time;
	int frame_size;
	int reserve0;

	anim_frame_data_t* frame_list[ANIM_FRAME_NUM_MAX];

	int tex_layer;
	int snd_channel;
	int reserve1;
	int reserve2;

	// draw color
	int color_r;
	int color_g;
	int color_b;
	int color_a;
};

struct _anim_stat_data_t {
	int type;        // NONE:0, ENABLE:1
	int id;          // anim_stat_id_end
	anim_stat_data_t* prev;
	anim_stat_data_t* next;

	void* parent;    // anim_data_t address
	int reserve0;
	int reserve1;
	int reserve2;

	int current_time;
	int current_frame;
	int chunk_frame;
	int command_frame;
};

struct _anim_data_t {
	int type;        // NONE:0
	int id;          // anim_id_end
	anim_data_t* prev;
	anim_data_t* next;

	void* obj;       // unit_data_t*
	int stat;        // NONE:0, IDLE:0x01, MOVE:0x02, ATTACK1-3:0x04,0x08,0x10, DAMAGE:0x20, DIE:0x40
	int base_w;
	int base_h;

	anim_stat_base_data_t* anim_stat_base_list[ANIM_STAT_END];
	anim_stat_data_t* anim_stat_list[ANIM_STAT_END];
};

extern int animation_manager_init();
extern void animation_manager_unload();
extern void animation_manager_delete_anim_stat_data(anim_data_t* delete_data);
extern anim_frame_data_t* animation_manager_new_anim_frame();
extern void animation_manager_new_anim_stat_base_data(anim_data_t* anim_data);
extern anim_data_t* animation_manager_new_anim_data();
extern int animation_manager_load_file(char* path, anim_data_t* anim_data, int stat);
extern int animation_manager_get_base_size_index(int base_size);
