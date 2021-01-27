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
#define AI_TYPE_BOSS         (0x00100000)
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

// bullet ai
#define AI_TYPE_BULLET             (0x00010000)

// ai_stat->val1
#define AI_PARAM_NONE          (0x00000000)
#define AI_PARAM_ATTACK        (0x00000001)
#define AI_PARAM_ALWAYS        (0x00000002)
#define AI_PARAM_IN_REGION     (0x00000004)
#define AI_PARAM_SLOPE_ATTACK  (0x00000008)
#define AI_PARAM_SPAWNER       (0x00000100)

// ai_stat->val2
#define AI_PARAM_MINI_BOSS_OFF (0)
#define AI_PARAM_MINI_BOSS_ON  (1)

// ai_stat->val4
#define AI_PARAM_SPAWN_OFF (0)
#define AI_PARAM_SPAWN_ON  (1)

// ai_stat->step[], ai_stat_bullet->step[]
#define AI_STAT_STEP_N    0
#define AI_STAT_STEP_E    1
#define AI_STAT_STEP_W    2
#define AI_STAT_STEP_S    3
#define AI_STAT_STEP_END  4

// bullet_ai->val1
#define AI_BULLET_PARAM_NONE          (0x00000000)
#define AI_BULLET_PARAM_CONTINUE      (0x00000001)
#define AI_BULLET_PARAM_TARGET        (0x00000002)

// bullet_ai->val2
#define AI_BULLET_PARAM_XCROSS_OFF    (0)
#define AI_BULLET_PARAM_XCROSS_ON     (1)

// ai timer
#define AI_WAIT_TIMER_SIMPLE          1000
#define AI_WAIT_TIMER_LEFT_RIGHT      1000
#define AI_WAIT_TIMER_STAY            1500
#define AI_WAIT_TIMER_ROUND           (ONE_FRAME_TIME * 10)
#define AI_WAIT_TIMER_GO_TO_BOM       1000
// boss
#define AI_WAIT_TIMER_BOSS_ONE        1000
#define AI_WAIT_TIMER_BOSS_TWO        1000
#define AI_WAIT_TIMER_BOSS_THREE      1000
#define AI_WAIT_TIMER_BOSS_FOUR        800
#define AI_WAIT_TIMER_BOSS_FIVE       (ONE_FRAME_TIME * 10)
#define AI_WAIT_TIMER_BOSS_SIX        1000
#define AI_WAIT_TIMER_BOSS_SEVEN      1000
#define AI_WAIT_TIMER_BOSS_EIGHT      1000
#define AI_WAIT_TIMER_BOSS_NINE       1000
#define AI_WAIT_TIMER_BOSS_X          1000
#define AI_WAIT_TIMER_BOSS_X_SPAWN     600

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
	int ghost_id;
	int reserv1;

	int val1;
	int val2;
	int val3;
	int val4;

	int timer1;
	int timer2;
	int reserv2;
	int reserv3;

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
// ai data control
extern void ai_manager_copy(ai_data_t* dst, ai_data_t* src);
extern void ai_manager_bullet_copy(ai_bullet_t* dst, ai_bullet_t* src);
extern int ai_manager_get_ai_type(std::string& value);
extern int ai_manager_load_bullet_file(std::string path, ai_bullet_t* bullet_base);
extern void ai_manager_delete_ai_data(ai_data_t* delete_data);
extern int ai_manager_delete_ghost(ai_data_t* ai_data);
extern ai_data_t* ai_manager_new_ai_base_data();
extern ai_data_t* ai_manager_new_ai_data();
// ai I/F
extern int ai_manager_spawn(ai_data_t* ai_data);
extern int ai_manager_stop(ai_data_t* ai_data);
extern int ai_manager_update(ai_data_t* ai_data);
extern int ai_manager_bullet_update(ai_data_t* ai_data);
// ai debug
extern void ai_manager_display();

// ai_manager_boss
extern void ai_manager_boss_update_one(ai_data_t* ai_data);
extern void ai_manager_boss_update_two(ai_data_t* ai_data);
extern void ai_manager_boss_update_three(ai_data_t* ai_data);
extern void ai_manager_boss_update_four(ai_data_t* ai_data);
extern void ai_manager_boss_update_five(ai_data_t* ai_data);
extern void ai_manager_boss_update_six(ai_data_t* ai_data);
extern void ai_manager_boss_update_seven(ai_data_t* ai_data);
extern void ai_manager_boss_update_eight(ai_data_t* ai_data);
extern void ai_manager_boss_update_nine(ai_data_t* ai_data);
extern void ai_manager_boss_update_x(ai_data_t* ai_data);

extern int ai_manager_boss_spawn_six(ai_data_t* ai_data);
extern int ai_manager_boss_spawn_eight(ai_data_t* ai_data);

extern int ai_manager_boss_stop_three(ai_data_t* ai_data);
extern int ai_manager_boss_stop_four(ai_data_t* ai_data);
extern int ai_manager_boss_spawn_x(ai_data_t* ai_data);
