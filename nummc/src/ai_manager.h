#pragma once
#include "game_common.h"

//#define AI_DEBUG

// enemy ai
#define AI_TYPE_NONE          0
#define AI_TYPE_SIMPLE        1
#define AI_TYPE_LEFT_RIGHT    2
#define AI_TYPE_UP_DOWN       3
#define AI_TYPE_STAY          4
#define AI_TYPE_FACE_ROUND    5
#define AI_TYPE_ROUND         6
#define AI_TYPE_ROUND_LR      7
#define AI_TYPE_ROUND_MOVE    8
#define AI_TYPE_RANDOM        9
#define AI_TYPE_RANDOM_GRID  10
#define AI_TYPE_GO_TO_BOM    11
//#define AI_TYPE_END          12

// enemy ai(boss)
#define AI_TYPE_BOSS         (0x00010000)
#define AI_TYPE_BOSS_ONE     (AI_TYPE_BOSS |  1)
#define AI_TYPE_BOSS_TWO     (AI_TYPE_BOSS |  2)
#define AI_TYPE_BOSS_THREE   (AI_TYPE_BOSS |  3)
#define AI_TYPE_BOSS_FOUR    (AI_TYPE_BOSS |  4)
#define AI_TYPE_BOSS_FIVE    (AI_TYPE_BOSS |  5)
#define AI_TYPE_BOSS_SIX     (AI_TYPE_BOSS |  6)
#define AI_TYPE_BOSS_SEVEN   (AI_TYPE_BOSS |  7)
#define AI_TYPE_BOSS_EIGHT   (AI_TYPE_BOSS |  8)
#define AI_TYPE_BOSS_NINE    (AI_TYPE_BOSS |  9)
#define AI_TYPE_BOSS_X       (AI_TYPE_BOSS | 10)
#define AI_TYPE_BOSS_Y       (AI_TYPE_BOSS | 11)
#define AI_TYPE_BOSS_Z       (AI_TYPE_BOSS | 12)

#define AI_PARAM_NONE          (0x00000000)
#define AI_PARAM_ATTACK        (0x00000001)
#define AI_PARAM_ALWAYS        (0x00000002)
#define AI_PARAM_IN_REGION     (0x00000004)
#define AI_PARAM_SLOPE_ATTACK  (0x00000008)

// bullet ai
#define AI_TYPE_BULLET             (0x00010000)

#define AI_STAT_STEP_N    0
#define AI_STAT_STEP_E    1
#define AI_STAT_STEP_W    2
#define AI_STAT_STEP_S    3
#define AI_STAT_STEP_END  4

#define AI_BULLET_PARAM_NONE          (0x00000000)
#define AI_BULLET_PARAM_CONTINUE      (0x00000001)

// ai timer
#define AI_WAIT_TIMER_SIMPLE      1000
#define AI_WAIT_TIMER_LEFT_RIGHT  1000
#define AI_WAIT_TIMER_STAY        1500
#define AI_WAIT_TIMER_ROUND       (ONE_FRAME_TIME * 10)
#define AI_WAIT_TIMER_GO_TO_BOM   1000
// boss
#define AI_WAIT_TIMER_BOSS_ONE    1000
#define AI_WAIT_TIMER_BOSS_TWO    1000
#define AI_WAIT_TIMER_BOSS_THREE  1000
#define AI_WAIT_TIMER_BOSS_FOUR   1000

typedef struct _ai_data_t ai_data_t;
typedef struct _ai_common_data_t ai_common_data_t;
typedef struct _ai_stat_data_t ai_stat_data_t;
typedef struct _ai_bullet_t ai_bullet_t;
typedef struct _ai_stat_bullet_t ai_stat_bullet_t;

struct _ai_data_t {
	int type;        // NONE:0
	int id;          // ai_id_end
	void* obj;       // object address
	int reserve0;

	ai_data_t* prev;
	ai_data_t* next;
	int reserv1;
	int reserv2;

	int val1;
	int val2;
	int val3;
	int val4;

	Uint32 data_field[20];
};

// enemy ai
struct _ai_common_data_t {
	int type;        // SIMPLE,SIMPLE_FIRE, ...
	int id;          // ai_id_end
	void* obj;       // object address
	int reserve0;

	ai_data_t* prev;
	ai_data_t* next;
	int reserv1;
	int reserv2;

	int val1;
	int val2;
	int val3;
	int val4;
};

struct _ai_stat_data_t {
	int type;        // SIMPLE,SIMPLE_FIRE, ...
	int id;          // ai_id_end
	void* obj;       // unit address
	int reserve0;

	ai_data_t* prev;
	ai_data_t* next;
	int reserv1;
	int reserv2;

	int val1;
	int val2;
	int val3;
	int val4;

	int timer1;
	int timer2;
	int reserv3;
	int reserv4;

	int step[AI_STAT_STEP_END];
};

// bullet ai
struct _ai_bullet_t {
	int type;        // AI_TYPE_BULLET
	int id;          // ai_id_end
	void* obj;       // object address
	int reserve0;

	ai_data_t* prev;
	ai_data_t* next;
	int reserv1;
	int reserv2;

	int val1;
	int val2;
	int val3;
	int val4;

	int bullet_path_index;	// g_enemy_bullet_path index
	int bullet_track_type;	// LINE,RADIAL,WAVE,CROSS,XCROSS
	int bullet_num;			// SINGLE,DOUBLE,TRIPLE 
	int bullet_face;
};

struct _ai_stat_bullet_t {
	int type;        // AI_TYPE_BULLET
	int id;          // ai_id_end
	void* obj;       // object address
	int reserve0;

	ai_data_t* prev;
	ai_data_t* next;
	int reserv1;
	int reserv2;

	int val1;
	int val2;
	int val3;
	int val4;

	int bullet_path_index;	// g_enemy_bullet_path index
	int bullet_track_type;	// LINE,RADIAL,WAVE,CROSS,XCROSS
	int bullet_num;			// SINGLE,DOUBLE,TRIPLE 
	int bullet_face;

	int timer1;
	int timer2;
	int timer3;
	int timer4;

	int step[AI_STAT_STEP_END];
};

extern int ai_manager_init();
extern void ai_manager_unload();
extern void ai_manager_copy(ai_data_t* dst, ai_data_t* src);
extern void ai_manager_bullet_copy(ai_bullet_t* dst, ai_bullet_t* src);
extern int ai_manager_get_ai_type(std::string& value);
extern int ai_manager_load_bullet_file(std::string path, ai_bullet_t* bullet_base);
extern void ai_manager_delete_ai_data(ai_data_t* delete_data);
extern ai_data_t* ai_manager_new_ai_base_data();
extern ai_data_t* ai_manager_new_ai_data();
extern int ai_manager_update(ai_data_t* ai_data);
extern int ai_manager_bullet_update(ai_data_t* ai_data);
extern void ai_manager_display();

// ai_manager_boss
extern void ai_manager_boss_update_one(ai_data_t* ai_data);
extern void ai_manager_boss_update_two(ai_data_t* ai_data);
extern void ai_manager_boss_update_three(ai_data_t* ai_data);
extern void ai_manager_boss_update_four(ai_data_t* ai_data);
