#pragma once
#include "game_common.h"

//#define AI_DEBUG

#define AI_TYPE_NONE          0
#define AI_TYPE_SIMPLE        1
#define AI_TYPE_SIMPLE_FIRE   2
#define AI_TYPE_LEFT_RIGHT    3
#define AI_TYPE_UP_DOWN       4
#define AI_TYPE_STAY          5
#define AI_TYPE_FACE_ROUND    6
#define AI_TYPE_ROUND         7
#define AI_TYPE_ROUND_LR      8
#define AI_TYPE_ROUND_MOVE    9
#define AI_TYPE_RANDOM       10
#define AI_TYPE_RANDOM_GRID  11
#define AI_TYPE_GO_TO_BOM    12
#define AI_TYPE_END          13

#define AI_STAT_STEP_N    0
#define AI_STAT_STEP_E    1
#define AI_STAT_STEP_W    2
#define AI_STAT_STEP_S    3
#define AI_STAT_STEP_END  4

#define AI_SIMPLE_WAIT_TIMER      1000
#define AI_LEFT_RIGHT_WAIT_TIMER  1000
#define AI_STAY_WAIT_TIMER        1500

typedef struct _ai_data_t ai_data_t;
typedef struct _ai_common_data_t ai_common_data_t;
typedef struct _ai_stat_data_t ai_stat_data_t;

struct _ai_data_t {
	int type;        // NONE:0
	int id;          // ai_id_end
	void* obj;       // object address
	int reserve0;

	ai_data_t* prev;
	ai_data_t* next;
	int reserv1;
	int reserv2;

	Uint32 data_field[24];
};

struct _ai_common_data_t {
	int type;        // SIMPLE:1, STAY:4
	int id;          // ai_id_end
	void* obj;       // object address
	int reserve0;

	ai_data_t* prev;
	ai_data_t* next;
	int reserv1;
	int reserv2;

	int x;
	int y;
	int w;
	int h;

	int bullet1;
	int bullet2;
	int bullet1_num;
	int bullet2_num;

	int bullet1_face;
	int bullet2_face;
	int reserv3;
	int reserv4;
};

struct _ai_stat_data_t {
	int type;        // SIMPLE:1, STAY:4
	int id;          // ai_id_end
	void* obj;       // unit address
	int reserve0;

	ai_data_t* prev;
	ai_data_t* next;
	int reserv1;
	int reserv2;

	int x;
	int y;
	int w;
	int h;

	int bullet1;
	int bullet2;
	int bullet1_num;
	int bullet2_num;

	int bullet1_face;
	int bullet2_face;
	int reserv3;
	int reserv4;

	int start_x;
	int start_y;
	int timer1;
	int timer2;

	int step[AI_STAT_STEP_END];
};

extern int ai_manager_init();
extern void ai_manager_unload();
extern void ai_manager_delete_ai_data(ai_data_t* delete_data);
extern ai_data_t* ai_manager_new_ai_base_data();
extern ai_data_t* ai_manager_new_ai_data();
extern int ai_manager_update(ai_data_t* ai_data);
extern void ai_manager_display();
