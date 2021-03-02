#pragma once
#include <vector>
#include "game_common.h"
#include "map_manager.h"

#define STAGE_STAT_NONE       0
#define STAGE_STAT_IDLE       1
#define STAGE_STAT_ACTIVE     2
#define STAGE_STAT_TERMINATE  3
#define STAGE_STAT_END        4

#define STAGE_RESULT_NONE     0
#define STAGE_RESULT_WIN      1
#define STAGE_RESULT_LOSE     2
#define STAGE_RESULT_END      3

#define STAGE_NEXT_LOAD_OFF   0
#define STAGE_NEXT_LOAD_ON    1

#define SECTION_TYPE_NONE       0
#define SECTION_TYPE_BOSS       1
#define SECTION_TYPE_HIDE       2
#define SECTION_TYPE_ITEM       3
#define SECTION_TYPE_NEST       4
#define SECTION_TYPE_NORMAL     5
#define SECTION_TYPE_END        6

#define SECTION_STAT_NONE              0
#define SECTION_STAT_IDLE              1
#define SECTION_STAT_ACTIVE            2
#define SECTION_STAT_TERMINATE         3
#define SECTION_STAT_DOOR_SELECT_WAIT  4
#define SECTION_STAT_ENEMY_PHASE_WAIT  5
#define SECTION_STAT_NEXT_WAIT         6
#define SECTION_STAT_END               7

#define SECTION_CIRCUMSTANCE_NONE                 (0)
#define SECTION_CIRCUMSTANCE_FLAG_SLOWED_ENEMY    (0x00000001 << 1)

#define SECTION_INDEX_START     0
#define SECTION_TIMER_NEXT_WAIT_TIME  5000
#define SECTION_TIMER_ENEMY_PHASE_WAIT_TIME  2000
#define SECTION_ENEMY_PHASE_SIZE  5

#define STAGE_MAP_WIDTH_NUM    9
#define STAGE_MAP_HEIGHT_NUM   5
#define STAGE_MAP_ID_IGNORE    (-1)

#define STAGE_MAP_STAT_NONE    (0x00000000)
#define STAGE_MAP_STAT_WIN     (0x00000001)
#define STAGE_MAP_STAT_GOAL    (0x00000002)
#define STAGE_MAP_STAT_HINT    (0x00000080)

#define STAGE_MINI_MAP_ICON_NONE     0
#define STAGE_MINI_MAP_ICON_TBOX     1
#define STAGE_MINI_MAP_ICON_UNKNOWN  2
#define STAGE_MINI_MAP_ICON_CHARGE   3
#define STAGE_MINI_MAP_ICON_STOCK    4
#define STAGE_MINI_MAP_ICON_HEART    5
#define STAGE_MINI_MAP_ICON_BOM      6
#define STAGE_MINI_MAP_ICON_ITEM     7
#define STAGE_MINI_MAP_ICON_END      8

// clock-wise
#define STAGE_MAP_FACE_W       0
#define STAGE_MAP_FACE_N       1
#define STAGE_MAP_FACE_E       2
#define STAGE_MAP_FACE_S       3
#define STAGE_MAP_FACE_END     4

#define STAGE_MAP_FLAG_FACE_W  (0x01 << STAGE_MAP_FACE_W)
#define STAGE_MAP_FLAG_FACE_N  (0x01 << STAGE_MAP_FACE_N)
#define STAGE_MAP_FLAG_FACE_E  (0x01 << STAGE_MAP_FACE_E)
#define STAGE_MAP_FLAG_FACE_S  (0x01 << STAGE_MAP_FACE_S)

// daytime
#define STAGE_DAYTIME_STAT_NONE          0
#define STAGE_DAYTIME_STAT_MORNING       1
#define STAGE_DAYTIME_STAT_AFTERNOON     2
#define STAGE_DAYTIME_STAT_EVENING       3
#define STAGE_DAYTIME_STAT_LATE_NIGHT    4

#define STAGE_DAYTIME_MIN_VAL        (0)
#define STAGE_DAYTIME_MAX_VAL        (24 * 60 * 60)  /* 86400 */

#define STAGE_DAYTIME_MORNING_MIN    (6 * 60 * 60)    /* 21600 */
#define STAGE_DAYTIME_MORNING_MAX    (12 * 60 * 60)   /* 43200 */
#define STAGE_DAYTIME_AFTERNOON_MIN  (12 * 60 * 60)   /* 43200 */
#define STAGE_DAYTIME_AFTERNOON_MAX  (18 * 60 * 60)   /* 64800 */
#define STAGE_DAYTIME_EVENING_MIN    (18 * 60 * 60)   /* 64800 */
#define STAGE_DAYTIME_EVENING_MAX    (24 * 60 * 60)   /* 86400 == 0 */
#define STAGE_DAYTIME_LATE_NIGHT_MIN (0)              /* 0 == 86400 */
#define STAGE_DAYTIME_LATE_NIGHT_MAX (6 * 60 * 60)    /* 21600 */

#define STAGE_DAYTIME_DEFAULT_VAL        STAGE_DAYTIME_MORNING_MIN
#define STAGE_DAYTIME_FRAME_DEFAULT_VAL  (10)            /* 1min -> about 6h */
#define STAGE_DAYTIME_LATE_NIGHT_BEFORE  (STAGE_DAYTIME_EVENING_MAX    - 60 * 60)
#define STAGE_DAYTIME_MORNING_BEFORE     (STAGE_DAYTIME_LATE_NIGHT_MAX - 60 * 60)

typedef struct _BGM_data_t BGM_data_t;
typedef struct _items_data_t items_data_t;
typedef struct _trap_data_t trap_data_t;
typedef struct _enemy_data_t enemy_data_t;
typedef struct _section_data_t section_data_t;
typedef struct _section_stock_item_t section_stock_item_t;
typedef struct _stage_map_data_t stage_map_data_t;
typedef struct _stage_data_t stage_data_t;

struct _BGM_data_t {
	int type;
	int id;
	node_data_t* prev;
	node_data_t* next;

	ResourceChunk* res_chunk;
	Uint32 time;       // start time[msec]
};

struct _items_data_t {
	int type;
	int id;
	node_data_t* prev;
	node_data_t* next;

	std::string path;
	int x;
	int y;
};

struct _trap_data_t {
	int type;
	int id;
	node_data_t* prev;
	node_data_t* next;

	std::string path;
	int x;
	int y;
};

struct _enemy_data_t {
	int type;
	int id;
	node_data_t* prev;
	node_data_t* next;

	std::string path;
	int x;
	int y;
	int vec_x;
	int vec_y;
	Uint32 delay;
	int face;
	int ai_step;
};

struct _section_data_t {
	int id;
	int section_type;  // SECTION_TYPE_XXX
	int item_drop_rate;

	std::string map_path;
	std::string bgm_path;
	std::string enemy_path[SECTION_ENEMY_PHASE_SIZE];
	std::string trap_path;
	std::string items_path;

	node_buffer_info_t* bgm_list;
	node_buffer_info_t* enemy_list[SECTION_ENEMY_PHASE_SIZE];
	node_buffer_info_t* trap_list;

	node_buffer_info_t* items_list;
	std::vector<std::string> drop_items_list;
	std::vector<std::string> goal_items_list;

	// tmp region
	std::vector<int> drop_items_id_list;
	std::vector<int> goal_items_id_list;
};

struct _section_stock_item_t {
	int type;
	int id;
	node_data_t* prev;
	node_data_t* next;

	int item_id;
	int x;
	int y;
	int val1;
	int val2;
};

struct _stage_map_data_t {
	int section_id;
	int section_type;           // copy from section_data_t section_type
	int stat;                   // none/win/goal/hint
	int mini_map_icon;          // STAGE_MINI_MAP_ICON_XXX

	node_buffer_info_t* stock_item;  // node buffer
	int stock_item_count;

	char section_map[MAP_TYPE_END][MAP_WIDTH_NUM_MAX * MAP_HEIGHT_NUM_MAX];
};

struct _stage_data_t {
	std::string id;
	int stat;
	int result;
	int next_load;

	int start_x;
	int start_y;
	int section_start_x;
	int section_start_y;

	int goal_x;
	int goal_y;
	int goal_w;
	int goal_h;

	int bonus_exp;
	float friction_coef;

	std::string next_stage_id;
	std::vector<std::string> common_items_list;

	// section values
	section_data_t** section_list;  // section_list[STAGE_MAP_WIDTH_NUM * STAGE_MAP_HEIGHT_NUM] head data pointer
	int current_section_index;
	section_data_t* current_section_data;

	int section_stat;
	int section_circumstance;
	int section_timer;
	int section_enemy_phase;
	int drop_judge_count;

	// stage values
	stage_map_data_t stage_map[STAGE_MAP_WIDTH_NUM * STAGE_MAP_HEIGHT_NUM];
	int current_stage_map_index;

	int daytime_stat;
	int daytime_frame_time;
	int daytime_timer;  // 0 ... 86400 (24h x 60min x 60sec)
};

extern void stage_manager_init();
extern void stage_manager_unload();
extern section_stock_item_t* stage_manager_register_stock_item(void* unit_data);
extern void stage_manager_create_all_stock_item();
extern void stage_manager_delete_all_stock_item();
extern void stage_manager_set_stat(int stat);
extern void stage_manager_set_result(int result);
extern void stage_manager_set_next_load(int stat);
extern void stage_manager_set_section_circumstance(int stat);
extern int stage_manager_load(char* path);

// daytime
extern void stage_manager_daytime_init();
extern int stage_manager_daytime_get_hour(int daytime_timer);
extern int stage_manager_daytime_get_minutes(int daytime_timer);
extern int stage_manager_daytime_get_seconds(int daytime_timer);
extern void stage_manager_daytime_update();

extern stage_data_t* g_stage_data;
