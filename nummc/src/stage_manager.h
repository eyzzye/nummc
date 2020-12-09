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

#define SECTION_STAT_NONE       0
#define SECTION_STAT_IDLE       1
#define SECTION_STAT_ACTIVE     2
#define SECTION_STAT_TERMINATE  3
#define SECTION_STAT_NEXT_WAIT  4
#define SECTION_STAT_END        5

#define SECTION_TIMER_NEXT_WAIT_TIME  5000

typedef struct _BGM_data_t BGM_data_t;
typedef struct _items_data_t items_data_t;
typedef struct _trap_data_t trap_data_t;
typedef struct _enemy_data_t enemy_data_t;
typedef struct _section_data_t section_data_t;
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
};

struct _section_data_t {
	int id;
	int level1_drop_rate;

	std::string map_path;
	std::string bgm_path;
	std::string enemy_path;
	std::string trap_path;
	std::string items_path;

	std::vector<BGM_data_t*> bgm_list;
	std::vector<enemy_data_t*> enemy_list;
	std::vector<trap_data_t*> trap_list;

	std::vector<items_data_t*> items_list;
	std::vector<std::string> drop_items_list;
	std::vector<std::string> goal_items_list;

	// tmp region
	std::vector<int> drop_items_id_list;
	std::vector<int> goal_items_id_list;
};

struct _stage_data_t {
	std::string id;
	int stat;
	int result;
	int next_load;

	int start_x;
	int start_y;

	int goal_x;
	int goal_y;
	int goal_w;
	int goal_h;

	int bonus_exp;
	std::string next_stage_id;
	std::vector<std::string> common_items_list;

	int section_stat;
	int section_timer;
	std::vector<section_data_t*> section_list;

	// tmp region
	int level1_drop_judge_count;
	section_data_t* current_section_data;
	int current_section_index;
};

extern void stage_manager_init();
extern void stage_manager_unload();
extern void stage_manager_set_stat(int stat);
extern void stage_manager_set_result(int result);
extern void stage_manager_set_next_load(int stat);
extern int stage_manager_load(std::string path);

extern stage_data_t* g_stage_data;
