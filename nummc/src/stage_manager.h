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

typedef struct _BGM_data_t BGM_data_t;
typedef struct _items_data_t items_data_t;
typedef struct _trap_data_t trap_data_t;
typedef struct _enemy_data_t enemy_data_t;
typedef struct _section_data_t section_data_t;
typedef struct _section_stock_item_t section_stock_item_t;
typedef struct _stage_map_data_t stage_map_data_t;
typedef struct _stage_data_t stage_data_t;

struct _BGM_data_t {
	Mix_Chunk* chunk;
	Uint32 time;       // start time[msec]
};

struct _items_data_t {
	std::string type;
	int x;
	int y;
};

struct _trap_data_t {
	std::string type;
	int x;
	int y;
};

struct _enemy_data_t {
	std::string type;
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

	std::vector<BGM_data_t*> bgm_list;
	std::vector<enemy_data_t*> enemy_list[SECTION_ENEMY_PHASE_SIZE];
	std::vector<trap_data_t*> trap_list;

	std::vector<items_data_t*> items_list;
	std::vector<std::string> drop_items_list;
	std::vector<std::string> goal_items_list;

	// tmp region
	std::vector<int> drop_items_id_list;
	std::vector<int> goal_items_id_list;
};

struct _section_stock_item_t {
	int type;
	int id;
	int x;
	int y;
	int val1;
	int val2;

	section_stock_item_t* prev;
	section_stock_item_t* next;
};

struct _stage_map_data_t {
	int section_id;
	int section_type;           // copy from section_data_t section_type
	int stat;                   // none/win/goal/hint
	int mini_map_icon;          // STAGE_MINI_MAP_ICON_XXX

	section_stock_item_t* stock_item;   // node head
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

	int section_stat;
	int section_timer;
	int section_enemy_phase;
	std::vector<section_data_t*> section_list;

	// tmp region
	int drop_judge_count;
	section_data_t* current_section_data;
	int current_section_index;

	// stage map
	stage_map_data_t stage_map[STAGE_MAP_WIDTH_NUM * STAGE_MAP_HEIGHT_NUM];
	int current_stage_map_index;
};

extern void stage_manager_init();
extern void stage_manager_unload();
extern section_stock_item_t* stage_manager_register_stock_item(void* unit_data);
extern void stage_manager_create_all_stock_item();
extern void stage_manager_delete_all_stock_item();
extern void stage_manager_delete_stock_item(section_stock_item_t* stock_item);
extern void stage_manager_set_stat(int stat);
extern void stage_manager_set_result(int result);
extern void stage_manager_set_next_load(int stat);
extern int stage_manager_load(std::string path);

extern stage_data_t* g_stage_data;
