#include <vector>
#include <string>
#include <fstream>
#include "game_common.h"
#include "stage_manager.h"

#include "resource_manager.h"
#include "game_utils.h"
#include "game_log.h"

// stage
#define STAGE_ID_BASIC_INFO   0
#define STAGE_ID_PLAYER_START 1
#define STAGE_ID_GOAL         2
#define STAGE_ID_NEXT_STAGE   3
#define STAGE_ID_ITEMS_DEF    4
#define STAGE_ID_SECTION      5
#define STAGE_ID_END          6

// section
#define SECTION_ID_MAP          0  // has no tag
#define SECTION_ID_BGM          1  // has no tag
#define SECTION_ID_ENEMY        2
#define SECTION_ID_TRAP         3
#define SECTION_ID_ITEMS        4
#define SECTION_ID_DROP_ITEMS   5
#define SECTION_ID_GOAL_ITEMS   6
#define SECTION_ID_END          7

#define STAGE_FRICTION_DEFAULT  (1.0f / 800.0f)  // 1.0->0.0[m/s] / 800[msec]

stage_data_t* g_stage_data;
static section_data_t* current_section_data;
static int tmp_start_index;

// stage
static void load_basic_info(std::string& line);
static void load_player_start(std::string& line);
static void load_goal(std::string& line);
static void load_next_stage(std::string& line);
static void load_items_def(std::string& line);
static void load_section(std::string& line);

// section
static int stage_manager_load_section_files();
static int load_section_file(std::string path);
static void load_items(std::string& line);
static void load_drop_items(std::string& line);
static void load_goal_items(std::string& line);
static void load_trap(std::string& line);
static void clear_enemy(enemy_data_t* enemy_data);
static void load_enemy(std::string& line);

void stage_manager_init()
{
	g_stage_data = new stage_data_t();
	g_stage_data->friction_coef = STAGE_FRICTION_DEFAULT;
	g_stage_data->stat = STAGE_STAT_NONE;
	g_stage_data->result = STAGE_RESULT_NONE;
	g_stage_data->next_load = STAGE_NEXT_LOAD_OFF;
	g_stage_data->section_stat = SECTION_STAT_NONE;
}

void stage_manager_unload()
{
	if (g_stage_data) {
		for (int sec_i = 0; sec_i < g_stage_data->section_list.size(); sec_i++) {
			section_data_t* p_section = g_stage_data->section_list[sec_i];
			if (p_section == NULL) continue;

			// bgm
			for (int i = 0; i < p_section->bgm_list.size(); i++) {
				if (p_section->bgm_list[i]) {
					delete p_section->bgm_list[i];
					p_section->bgm_list[i] = NULL;
				}
			}
			p_section->bgm_list.clear();

			// enemy
			for (int i = 0; i < p_section->enemy_list.size(); i++) {
				if (p_section->enemy_list[i]) {
					delete p_section->enemy_list[i];
					p_section->enemy_list[i] = NULL;
				}
			}
			p_section->enemy_list.clear();

			// trap
			for (int i = 0; i < p_section->trap_list.size(); i++) {
				if (p_section->trap_list[i]) {
					delete p_section->trap_list[i];
					p_section->trap_list[i] = NULL;
				}
			}
			p_section->trap_list.clear();

			// item
			for (int i = 0; i < p_section->items_list.size(); i++) {
				if (p_section->items_list[i]) {
					delete p_section->items_list[i];
					p_section->items_list[i] = NULL;
				}
			}
			p_section->items_list.clear();

			p_section->drop_items_id_list.clear();
			p_section->drop_items_list.clear();
			p_section->goal_items_id_list.clear();
			p_section->goal_items_list.clear();

			// section
			delete p_section;
			p_section = NULL;
		}

		// common item
		g_stage_data->common_items_list.clear();

		// stage
		delete g_stage_data;
		g_stage_data = NULL;
	}
}

void stage_manager_set_stat(int stat)
{
	g_stage_data->stat = stat;
}
void stage_manager_set_result(int result)
{
	g_stage_data->result = result;
}
void stage_manager_set_next_load(int stat)
{
	g_stage_data->next_load = stat;
}

int stage_manager_load(std::string path)
{
	bool read_flg[STAGE_ID_END] = { false };

	std::ifstream inFile(g_base_path + "data/" + path);
	if (inFile.is_open()) {
		std::string line;
		while (std::getline(inFile, line)) {
			if (line == "") continue;
			if (line[0] == '#') continue;

			if (line == "[basic_info]") { read_flg[STAGE_ID_BASIC_INFO] = true;  continue; }
			if (line == "[/basic_info]") { read_flg[STAGE_ID_BASIC_INFO] = false; continue; }
			if (line == "[player_start]") { read_flg[STAGE_ID_PLAYER_START] = true;  continue; }
			if (line == "[/player_start]") { read_flg[STAGE_ID_PLAYER_START] = false; continue; }
			if (line == "[goal]") { read_flg[STAGE_ID_GOAL] = true;  continue; }
			if (line == "[/goal]") { read_flg[STAGE_ID_GOAL] = false; continue; }
			if (line == "[next_stage]") { read_flg[STAGE_ID_NEXT_STAGE] = true;  continue; }
			if (line == "[/next_stage]") { read_flg[STAGE_ID_NEXT_STAGE] = false; continue; }
			if (line == "[items_def]") { read_flg[STAGE_ID_ITEMS_DEF] = true;  continue; }
			if (line == "[/items_def]") { read_flg[STAGE_ID_ITEMS_DEF] = false; continue; }
			if (line == "[section]") { read_flg[STAGE_ID_SECTION] = true;  continue; }
			if (line == "[/section]") { read_flg[STAGE_ID_SECTION] = false; continue; }

			if (read_flg[STAGE_ID_BASIC_INFO]) {
				load_basic_info(line);
			}
			if (read_flg[STAGE_ID_PLAYER_START]) {
				load_player_start(line);
			}
			if (read_flg[STAGE_ID_GOAL]) {
				load_goal(line);
			}
			if (read_flg[STAGE_ID_NEXT_STAGE]) {
				load_next_stage(line);
			}
			if (read_flg[STAGE_ID_ITEMS_DEF]) {
				load_items_def(line);
			}
			if (read_flg[STAGE_ID_SECTION]) {
				load_section(line);
			}
		}
		inFile.close();
	}
	else {
		LOG_ERROR("stage_manager_load %s error\n", path.c_str());
		return 1;
	}

	// load section files
	int ret = stage_manager_load_section_files();

	return ret;
}

static void load_basic_info(std::string& line) {
	std::string key;
	std::string value;
	game_utils_split_key_value(line, key, value);
	if (key == "id") g_stage_data->id = value;
	if (key == "bonus_exp") g_stage_data->bonus_exp = atoi(value.c_str());
	if (key == "friction_denominator") {
		float denominator = (float)atoi(value.c_str());
		g_stage_data->friction_coef = 1.0f / denominator;
	}
}

static void load_player_start(std::string& line) {
	std::string key;
	std::string value;
	game_utils_split_key_value(line, key, value);
	if (key == "x") g_stage_data->start_x = atoi(value.c_str());
	if (key == "y") g_stage_data->start_y = atoi(value.c_str());
}

static void load_goal(std::string& line) {
	std::string key;
	std::string value;
	game_utils_split_key_value(line, key, value);
	if (key == "x") g_stage_data->goal_x = atoi(value.c_str());
	if (key == "y") g_stage_data->goal_y = atoi(value.c_str());
	if (key == "w") g_stage_data->goal_w = atoi(value.c_str());
	if (key == "h") g_stage_data->goal_h = atoi(value.c_str());
}

static void load_next_stage(std::string& line) {
	g_stage_data->next_stage_id = line;
}

static void load_items_def(std::string& line) {
	g_stage_data->common_items_list.push_back(line);
}

static void load_section(std::string& line) {
	std::string key;
	std::string value;
	game_utils_split_key_value(line, key, value);

	if (key == "section") {
		current_section_data = new section_data_t();
		current_section_data->id = atoi(value.c_str());
		g_stage_data->section_list.push_back(current_section_data);
	}
	if (key == "item_drop_rate") current_section_data->item_drop_rate = atoi(value.c_str());

	if (key == "map_path")  current_section_data->map_path = value;
	if (key == "bgm_path") {
		current_section_data->bgm_path = value;
		BGM_data_t* new_bgm = new BGM_data_t();
		new_bgm->chunk = resource_manager_getChunkFromPath(value);
		current_section_data->bgm_list.push_back(new_bgm);
	}

	if (key == "enemy_path")  current_section_data->enemy_path = value;
	if (key == "trap_path")  current_section_data->trap_path = value;
	if (key == "items_path")  current_section_data->items_path = value;
}

static int stage_manager_load_section_files()
{
	for (int sec_i = 0; sec_i < g_stage_data->section_list.size(); sec_i++) {
		current_section_data = g_stage_data->section_list[sec_i];

		if (current_section_data->enemy_path.size() > 0) load_section_file(current_section_data->enemy_path);
		if (current_section_data->trap_path.size() > 0) load_section_file(current_section_data->trap_path);
		if (current_section_data->items_path.size() > 0) load_section_file(current_section_data->items_path);
	}
	return 0;
}

static int load_section_file(std::string path)
{
	bool read_flg[SECTION_ID_END] = { false };

	std::ifstream inFile(g_base_path + "data/" + path);
	if (inFile.is_open()) {
		std::string line;
		while (std::getline(inFile, line)) {
			if (line == "") continue;
			if (line[0] == '#') continue;

			if (line == "[enemy]") { read_flg[SECTION_ID_ENEMY] = true;  continue; }
			if (line == "[/enemy]") { read_flg[SECTION_ID_ENEMY] = false; continue; }
			if (line == "[trap]") { read_flg[SECTION_ID_TRAP] = true;  continue; }
			if (line == "[/trap]") { read_flg[SECTION_ID_TRAP] = false; continue; }
			if (line == "[items]") { read_flg[SECTION_ID_ITEMS] = true;  continue; }
			if (line == "[/items]") { read_flg[SECTION_ID_ITEMS] = false; continue; }
			if (line == "[drop_items]") { read_flg[SECTION_ID_DROP_ITEMS] = true;  continue; }
			if (line == "[/drop_items]") { read_flg[SECTION_ID_DROP_ITEMS] = false; continue; }
			if (line == "[goal_items]") { read_flg[SECTION_ID_GOAL_ITEMS] = true;  continue; }
			if (line == "[/goal_items]") { read_flg[SECTION_ID_GOAL_ITEMS] = false; continue; }

			if (read_flg[SECTION_ID_ENEMY]) {
				load_enemy(line);
			}
			if (read_flg[SECTION_ID_TRAP]) {
				load_trap(line);
			}
			if (read_flg[SECTION_ID_ITEMS]) {
				load_items(line);
			}
			if (read_flg[SECTION_ID_DROP_ITEMS]) {
				load_drop_items(line);
			}
			if (read_flg[SECTION_ID_GOAL_ITEMS]) {
				load_goal_items(line);
			}
		}
		inFile.close();
	}
	else {
		LOG_ERROR("load_stage_file %s error\n", path.c_str());
		return 1;
	}

	return 0;
}

static void load_items(std::string& line) {
	std::string key;
	std::string value;
	game_utils_split_key_value(line, key, value);
	if (key == "type") {
		items_data_t* new_item = new items_data_t();
		new_item->type = value;
		new_item->x = -1; // disable
		new_item->y = -1; // disable
		current_section_data->items_list.push_back(new_item);

		tmp_start_index = (int)current_section_data->items_list.size() - 1;
		return;
	}

	std::string type = current_section_data->items_list[tmp_start_index]->type;
	if (key == "x") {
		std::vector<int> val_list;
		game_utils_split_conmma(value, val_list);

		current_section_data->items_list[tmp_start_index]->x = val_list[0];
		for (int i = 1; i < val_list.size(); i++) {
			items_data_t* new_item = new items_data_t();
			new_item->type = type;
			new_item->x = -1; // disable
			new_item->y = -1; // disable
			current_section_data->items_list.push_back(new_item);
			current_section_data->items_list[(size_t)tmp_start_index + i]->x = val_list[i];
		}
	}
	if (key == "y") {
		std::vector<int> val_list;
		game_utils_split_conmma(value, val_list);
		for (int i = 0; i < val_list.size(); i++) {
			current_section_data->items_list[(size_t)tmp_start_index + i]->y = val_list[i];
		}
	}
}

static void load_drop_items(std::string& line) {
	current_section_data->drop_items_list.push_back(line);
}

static void load_goal_items(std::string& line) {
	current_section_data->goal_items_list.push_back(line);
}

static void load_trap(std::string& line) {
	std::string key;
	std::string value;
	game_utils_split_key_value(line, key, value);
	if (key == "type") {
		trap_data_t* new_trap = new trap_data_t();
		new_trap->type = value;
		new_trap->x = -1; // disable
		new_trap->y = -1; // disable
		current_section_data->trap_list.push_back(new_trap);

		tmp_start_index = (int)current_section_data->trap_list.size() - 1;
		return;
	}

	std::string type = current_section_data->trap_list[tmp_start_index]->type;
	if (key == "x") {
		std::vector<int> val_list;
		game_utils_split_conmma(value, val_list);

		current_section_data->trap_list[tmp_start_index]->x = val_list[0];
		for (int i = 1; i < val_list.size(); i++) {
			trap_data_t* new_trap = new trap_data_t();
			new_trap->type = type;
			new_trap->x = -1; // disable
			new_trap->y = -1; // disable
			current_section_data->trap_list.push_back(new_trap);
			current_section_data->trap_list[(size_t)tmp_start_index + i]->x = val_list[i];
		}
	}
	if (key == "y") {
		std::vector<int> val_list;
		game_utils_split_conmma(value, val_list);
		for (int i = 0; i < val_list.size(); i++) {
			current_section_data->trap_list[(size_t)tmp_start_index + i]->y = val_list[i];
		}
	}
}

static void clear_enemy(enemy_data_t* enemy_data) {
	enemy_data->x = 0;
	enemy_data->y = 0;
	enemy_data->vec_x = 0;
	enemy_data->vec_y = 0;
	enemy_data->delay = 0;
	enemy_data->face = UNIT_FACE_W;
	enemy_data->ai_step = 0;
}

static void load_enemy(std::string& line) {
	std::string key;
	std::string value;
	game_utils_split_key_value(line, key, value);
	if (key == "type") {
		enemy_data_t* new_enemy = new enemy_data_t();
		clear_enemy(new_enemy);

		new_enemy->type = value;
		current_section_data->enemy_list.push_back(new_enemy);

		tmp_start_index = (int)current_section_data->enemy_list.size() - 1;
		return;
	}

	std::string type = current_section_data->enemy_list[tmp_start_index]->type;
	if (key == "x") {
		std::vector<int> val_list;
		game_utils_split_conmma(value, val_list);

		current_section_data->enemy_list[tmp_start_index]->x = val_list[0];
		for (int i = 1; i < val_list.size(); i++) {
			enemy_data_t* new_enemy = new enemy_data_t();
			clear_enemy(new_enemy);

			new_enemy->type = type;
			current_section_data->enemy_list.push_back(new_enemy);
			current_section_data->enemy_list[(size_t)tmp_start_index + i]->x = val_list[i];
		}
	}
	if (key == "y") {
		std::vector<int> val_list;
		game_utils_split_conmma(value, val_list);
		for (int i = 0; i < val_list.size(); i++) {
			current_section_data->enemy_list[(size_t)tmp_start_index + i]->y = val_list[i];
		}
	}
	if (key == "vec_x") {
		std::vector<int> val_list;
		game_utils_split_conmma(value, val_list);
		for (int i = 0; i < val_list.size(); i++) {
			current_section_data->enemy_list[(size_t)tmp_start_index + i]->vec_x = val_list[i];
		}
	}
	if (key == "vec_y") {
		std::vector<int> val_list;
		game_utils_split_conmma(value, val_list);
		for (int i = 0; i < val_list.size(); i++) {
			current_section_data->enemy_list[(size_t)tmp_start_index + i]->vec_y = val_list[i];
		}
	}
	if (key == "delay") {
		std::vector<int> val_list;
		game_utils_split_conmma(value, val_list);
		for (int i = 0; i < val_list.size(); i++) {
			current_section_data->enemy_list[(size_t)tmp_start_index + i]->delay = val_list[i];
		}
	}
	if (key == "face") {
		std::vector<std::string> val_list;
		game_utils_split_conmma(value, val_list);
		for (int i = 0; i < val_list.size(); i++) {
			if (val_list[i] == "N") {
				current_section_data->enemy_list[(size_t)tmp_start_index + i]->face = UNIT_FACE_N;
			}
			else if (val_list[i] == "E") {
				current_section_data->enemy_list[(size_t)tmp_start_index + i]->face = UNIT_FACE_E;
			}
			else if (val_list[i] == "W") {
				current_section_data->enemy_list[(size_t)tmp_start_index + i]->face = UNIT_FACE_W;
			}
			else if (val_list[i] == "S") {
				current_section_data->enemy_list[(size_t)tmp_start_index + i]->face = UNIT_FACE_S;
			}
			else {
				current_section_data->enemy_list[(size_t)tmp_start_index + i]->face = UNIT_FACE_NONE;
			}
		}
	}
	if (key == "ai_step") {
		std::vector<int> val_list;
		game_utils_split_conmma(value, val_list);
		for (int i = 0; i < val_list.size(); i++) {
			current_section_data->enemy_list[(size_t)tmp_start_index + i]->ai_step = val_list[i];
		}
	}
}
