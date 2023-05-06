#include "game_common.h"
#include "stage_manager.h"

#include "resource_manager.h"
#include "memory_manager.h"
#include "game_utils.h"
#include "game_log.h"
#include "unit_manager.h"
#include "quest_log_manager.h"

// stage
#define STAGE_ID_BASIC_INFO   0
#define STAGE_ID_DAYTIME      1
#define STAGE_ID_PLAYER_START 2
#define STAGE_ID_GOAL         3
#define STAGE_ID_NEXT_STAGE   4
#define STAGE_ID_ITEMS_DEF    5
#define STAGE_ID_SECTION      6
#define STAGE_ID_END          7

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

static const char* empty_map_path = "map/common/n_empty.tmx";

stage_data_t* g_stage_data;
static section_data_t* current_section_data;
static int current_section_id;
static node_data_t* tmp_new_node;
static int tmp_current_enemy_phase;

// stage data instance
static stage_data_t stage_data_buffer;
static node_buffer_info_t common_items_list_buffer_info;

// section data list
#define SECTION_DATA_BUFFER_SIZE  (STAGE_MAP_WIDTH_NUM * STAGE_MAP_HEIGHT_NUM)
static section_data_t section_data_buffer[SECTION_DATA_BUFFER_SIZE];
int section_data_buffer_index_end;
static section_data_t* section_list_buffer[SECTION_DATA_BUFFER_SIZE];

// node data
static node_buffer_info_t bgm_list_buffer_info[SECTION_DATA_BUFFER_SIZE];
static node_buffer_info_t enemy_list_buffer_info[SECTION_DATA_BUFFER_SIZE * SECTION_ENEMY_PHASE_SIZE];
static node_buffer_info_t trap_list_buffer_info[SECTION_DATA_BUFFER_SIZE];
static node_buffer_info_t items_list_buffer_info[SECTION_DATA_BUFFER_SIZE];

static node_buffer_info_t drop_items_list_buffer_info[SECTION_DATA_BUFFER_SIZE];
static node_buffer_info_t goal_items_list_buffer_info[SECTION_DATA_BUFFER_SIZE];

// stock item
#define SECTION_STOCK_ITEM_SIZE  (UNIT_ITEMS_LIST_SIZE * STAGE_MAP_WIDTH_NUM * STAGE_MAP_HEIGHT_NUM)
static node_buffer_info_t section_stock_item_buffer_info[SECTION_DATA_BUFFER_SIZE];

// stage
static void load_basic_info(char* line);
static void load_daytime(char* line);
static void load_player_start(char* line);
static void load_goal(char* line);
static void load_next_stage(char* line);
static void load_items_def(char* line);
static void load_section(char* line);

// section
static int stage_manager_load_section_files();
static int load_section_file(char* path);
static void load_items(char* line);
static void load_drop_items(char* line);
static void load_goal_items(char* line);
static void load_trap(char* line);
static void clear_enemy(enemy_data_t* enemy_data);
static void load_enemy(char* line);

void stage_manager_init()
{
	// stage_data
	g_stage_data = &stage_data_buffer;
	g_stage_data->id = NULL;
	g_stage_data->next_stage_id = NULL;
	g_stage_data->friction_coef = STAGE_FRICTION_DEFAULT;
	g_stage_data->stat = STAGE_STAT_NONE;
	g_stage_data->result = STAGE_RESULT_NONE;
	g_stage_data->next_load = STAGE_NEXT_LOAD_OFF;
	g_stage_data->section_stat = SECTION_STAT_NONE;

	// stage_data common_items
	game_utils_node_init(&common_items_list_buffer_info, (int)sizeof(items_data_t));

	// section_data
	for (int i = 0; i < SECTION_DATA_BUFFER_SIZE; i++) {
		section_data_buffer[i].section_type = SECTION_TYPE_NONE;
	}
	section_data_buffer_index_end = 0;

	// section_list
	memset(section_list_buffer, 0, sizeof(section_list_buffer));
	g_stage_data->section_list = &section_list_buffer[0];

	for (int i = 0; i < SECTION_DATA_BUFFER_SIZE; i++) {
		// BGM node
		game_utils_node_init(&bgm_list_buffer_info[i], (int)sizeof(BGM_data_t));

		// enemy node
		for (int phase = 0; phase < SECTION_ENEMY_PHASE_SIZE; phase++) {
			game_utils_node_init(&enemy_list_buffer_info[i * SECTION_ENEMY_PHASE_SIZE + phase], (int)sizeof(enemy_data_t));
		}

		// trap node
		game_utils_node_init(&trap_list_buffer_info[i], (int)sizeof(trap_data_t));

		// items node
		game_utils_node_init(&items_list_buffer_info[i], (int)sizeof(items_data_t));
		game_utils_node_init(&drop_items_list_buffer_info[i], (int)sizeof(items_data_t));
		game_utils_node_init(&goal_items_list_buffer_info[i], (int)sizeof(items_data_t));

		// item stocker
		game_utils_node_init(&section_stock_item_buffer_info[i], (int)sizeof(section_stock_item_t));
	}

	// stage_map
	memset(g_stage_data->stage_map, 0, sizeof(g_stage_data->stage_map));
	for (int i = 0; i < LENGTH_OF(g_stage_data->stage_map); i++) {
		g_stage_data->stage_map[i].section_id = STAGE_MAP_ID_IGNORE;
	}

	// daytime
	stage_manager_daytime_init();
}

void stage_manager_unload()
{
	if (g_stage_data) {
		node_data_t* node = NULL;

		for (int sec_i = 0; sec_i < SECTION_DATA_BUFFER_SIZE; sec_i++) {
			section_data_t* p_section = g_stage_data->section_list[sec_i];
			if (p_section == NULL) continue;

			// bgm
			node = (p_section->bgm_list == NULL) ? NULL : p_section->bgm_list->start_node;
			while (node != NULL) {
				node_data_t* del_node = node;
				node = del_node->next;
				game_utils_node_delete(del_node, p_section->bgm_list);
			}

			// enemy
			for (int phase = 0; phase < SECTION_ENEMY_PHASE_SIZE; phase++) {
				node = (p_section->enemy_list[phase] == NULL) ? NULL : p_section->enemy_list[phase]->start_node;
				while (node != NULL) {
					node_data_t* del_node = node;
					node = del_node->next;

					char* tmp_path = ((enemy_data_t*)del_node)->path;
					if (tmp_path != NULL) memory_manager_delete_char_buff(tmp_path);
					game_utils_node_delete(del_node, p_section->enemy_list[phase]);
				}
			}

			// trap
			node = (p_section->trap_list == NULL) ? NULL : p_section->trap_list->start_node;
			while (node != NULL) {
				node_data_t* del_node = node;
				node = del_node->next;

				char* tmp_path = ((trap_data_t*)del_node)->path;
				if (tmp_path != NULL) memory_manager_delete_char_buff(tmp_path);
				game_utils_node_delete(del_node, p_section->trap_list);
			}

			// item
			node = (p_section->items_list == NULL) ? NULL : p_section->items_list->start_node;
			while (node != NULL) {
				node_data_t* del_node = node;
				node = del_node->next;

				char* tmp_path = ((items_data_t*)del_node)->path;
				if (tmp_path != NULL) memory_manager_delete_char_buff(tmp_path);
				game_utils_node_delete(del_node, p_section->items_list);
			}


			//p_section->drop_items_list.clear();
			node = (p_section->drop_items_list == NULL) ? NULL : p_section->drop_items_list->start_node;
			while (node != NULL) {
				node_data_t* del_node = node;
				node = del_node->next;

				char* tmp_path = ((items_data_t*)del_node)->path;
				if (tmp_path != NULL) memory_manager_delete_char_buff(tmp_path);
				game_utils_node_delete(del_node, p_section->drop_items_list);
			}

			//p_section->goal_items_list.clear();
			node = (p_section->goal_items_list == NULL) ? NULL : p_section->goal_items_list->start_node;
			while (node != NULL) {
				node_data_t* del_node = node;
				node = del_node->next;

				char* tmp_path = ((items_data_t*)del_node)->path;
				if (tmp_path != NULL) memory_manager_delete_char_buff(tmp_path);
				game_utils_node_delete(del_node, p_section->goal_items_list);
			}

			// release char_buff
			if (p_section->map_path) { memory_manager_delete_char_buff(p_section->map_path); p_section->map_path = NULL; }
			if (p_section->bgm_path) { memory_manager_delete_char_buff(p_section->bgm_path); p_section->bgm_path = NULL; }
			for (int i = 0; i < SECTION_ENEMY_PHASE_SIZE; i++) {
				if (p_section->enemy_path[i] == NULL) continue;
				memory_manager_delete_char_buff(p_section->enemy_path[i]);
				p_section->enemy_path[i] = NULL;
			}
			if (p_section->trap_path) { memory_manager_delete_char_buff(p_section->trap_path); p_section->trap_path = NULL; }
			if (p_section->items_path) { memory_manager_delete_char_buff(p_section->items_path); p_section->items_path = NULL; }

			// section
			p_section->section_type = SECTION_TYPE_NONE;
			g_stage_data->section_list[sec_i] = NULL;
		}
		g_stage_data->section_list = NULL;

		// g_stage_data->stage_map[]-> stock_item
		for (int sec_i = 0; sec_i < SECTION_DATA_BUFFER_SIZE; sec_i++) {
			node_buffer_info_t* node_buffer_info = g_stage_data->stage_map[sec_i].stock_item;
			if (node_buffer_info == NULL) continue;

			node_data_t* node = node_buffer_info->start_node;
			while (node != NULL) {
				node_data_t* del_node = (node_data_t*)node;
				node = del_node->next;

				game_utils_node_delete(del_node, node_buffer_info);
			}

			g_stage_data->stage_map[sec_i].stock_item = NULL;
		}

		// common item
		node = (g_stage_data->common_items_list == NULL) ? NULL : g_stage_data->common_items_list->start_node;
		while (node != NULL) {
			node_data_t* del_node = node;
			node = del_node->next;

			char* tmp_path = ((items_data_t*)del_node)->path;
			if (tmp_path != NULL) memory_manager_delete_char_buff(tmp_path);
			game_utils_node_delete(del_node, g_stage_data->common_items_list);
		}

		// release char_buf
		if(g_stage_data->id) memory_manager_delete_char_buff(g_stage_data->id);
		if(g_stage_data->next_stage_id) memory_manager_delete_char_buff(g_stage_data->next_stage_id);

		// stage
		g_stage_data = NULL;
	}
}

//
// data allocation
//
static section_data_t* new_section_data()
{
	if (section_data_buffer_index_end >= SECTION_DATA_BUFFER_SIZE) {
		LOG_ERROR("ERROR: new_section_data() overflow\n");
		return NULL;
	}

	section_data_t* ret = &section_data_buffer[section_data_buffer_index_end];
	if (ret->section_type != SECTION_TYPE_NONE) {
		LOG_ERROR("ERROR: new_section_data() occurred unexpected error\n");
		return NULL;
	}

	section_data_buffer_index_end++;
	return ret;
}

section_stock_item_t* stage_manager_register_stock_item(void* unit_data)
{
	int stage_map_index = g_stage_data->current_stage_map_index;

	// search empty node
	section_stock_item_t* new_node = (section_stock_item_t*)game_utils_node_new(&section_stock_item_buffer_info[stage_map_index]);
	if (new_node == NULL) { LOG_ERROR("ERROR: stage_manager_register_stock_item() overflow\n"); return NULL; }

	unit_items_data_t* items_data = (unit_items_data_t*)unit_data;
	new_node->type = items_data->base->type;
	new_node->item_id = items_data->base->id;
	new_node->x = items_data->col_shape->x;
	new_node->y = items_data->col_shape->y;

	if ((items_data->group == UNIT_ITEM_GROUP_STOCK) && (items_data->sub_id == UNIT_STOCK_SUB_ID_CHARGE)) {
		new_node->val1 = items_data->val1;
		new_node->val2 = items_data->val2;
	}

	if (g_stage_data->stage_map[g_stage_data->current_stage_map_index].stock_item == NULL) {
		g_stage_data->stage_map[g_stage_data->current_stage_map_index].stock_item = &section_stock_item_buffer_info[stage_map_index];
	}
	return new_node;
}

void stage_manager_create_all_stock_item()
{
	node_buffer_info_t* node_buffer_info = g_stage_data->stage_map[g_stage_data->current_stage_map_index].stock_item;
	if (node_buffer_info == NULL) return;

	int max_count = 0;
	node_data_t* node = node_buffer_info->start_node;
	while (node != NULL) {
		section_stock_item_t* ref_node = (section_stock_item_t*)node;
		node = ref_node->next;

		int item_id = unit_manager_create_items(ref_node->x, ref_node->y, ref_node->item_id);
		unit_items_data_t* items_data = unit_manager_get_items(item_id);
		if ((items_data->group == UNIT_ITEM_GROUP_STOCK) && (items_data->sub_id == UNIT_STOCK_SUB_ID_CHARGE)) {
			unit_manager_items_set_val(item_id, ref_node->val1, 1);
			unit_manager_items_set_val(item_id, ref_node->val2, 2);
		}
		max_count++;
	}

	if (max_count >= SECTION_STOCK_ITEM_SIZE) {
		LOG_ERROR("ERROR: stage_manager_delete_all_stock_item() overflow\n");
	}
}

void stage_manager_delete_all_stock_item()
{
	node_buffer_info_t* node_buffer_info = g_stage_data->stage_map[g_stage_data->current_stage_map_index].stock_item;
	if (node_buffer_info == NULL) return;

	int max_count = 0;
	node_data_t* node = node_buffer_info->start_node;
	while (node != NULL) {
		section_stock_item_t* del_node = (section_stock_item_t*)node;
		node = del_node->next;

		game_utils_node_delete((node_data_t*)del_node, node_buffer_info);
		memset(del_node, 0, sizeof(section_stock_item_t));

		max_count++;
	}

	if (max_count >= SECTION_STOCK_ITEM_SIZE) {
		LOG_ERROR("ERROR: stage_manager_delete_all_stock_item() overflow\n");
	}

	g_stage_data->stage_map[g_stage_data->current_stage_map_index].stock_item = NULL;
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

void stage_manager_set_section_circumstance(int stat)
{
	// already enable
	if (g_stage_data->section_circumstance & stat) return;

	g_stage_data->section_circumstance |= stat;
	if (stat & SECTION_CIRCUMSTANCE_FLAG_SLOWED_ENEMY) {
		unit_manager_set_all_enemy_slowed();
	}
}

typedef struct _load_stage_callback_data_t load_stage_callback_data_t;
struct _load_stage_callback_data_t {
	bool read_flg[STAGE_ID_END];
};
static load_stage_callback_data_t load_stage_callback_data;
static void load_stage_callback(char* line, int line_size, int line_num, void* argv)
{
	load_stage_callback_data_t* data = (load_stage_callback_data_t*)argv;

	if (line[0] == '\0') return;
	if (line[0] == '#') return;

	if (line[0] == '[') {
		if (STRCMP_EQ(line, "[basic_info]"))    { data->read_flg[STAGE_ID_BASIC_INFO]   = true;  return; }
		if (STRCMP_EQ(line, "[/basic_info]"))   { data->read_flg[STAGE_ID_BASIC_INFO]   = false; return; }
		if (STRCMP_EQ(line, "[daytime]"))       { data->read_flg[STAGE_ID_DAYTIME]      = true;  return; }
		if (STRCMP_EQ(line, "[/daytime]"))      { data->read_flg[STAGE_ID_DAYTIME]      = false; return; }
		if (STRCMP_EQ(line, "[player_start]"))  { data->read_flg[STAGE_ID_PLAYER_START] = true;  return; }
		if (STRCMP_EQ(line, "[/player_start]")) { data->read_flg[STAGE_ID_PLAYER_START] = false; return; }
		if (STRCMP_EQ(line, "[goal]"))          { data->read_flg[STAGE_ID_GOAL]         = true;  return; }
		if (STRCMP_EQ(line, "[/goal]"))         { data->read_flg[STAGE_ID_GOAL]         = false; return; }
		if (STRCMP_EQ(line, "[next_stage]"))    { data->read_flg[STAGE_ID_NEXT_STAGE]   = true;  return; }
		if (STRCMP_EQ(line, "[/next_stage]"))   { data->read_flg[STAGE_ID_NEXT_STAGE]   = false; return; }
		if (STRCMP_EQ(line, "[items_def]"))     { data->read_flg[STAGE_ID_ITEMS_DEF]    = true;  return; }
		if (STRCMP_EQ(line, "[/items_def]"))    { data->read_flg[STAGE_ID_ITEMS_DEF]    = false; return; }
		if (STRCMP_EQ(line, "[section]"))       { data->read_flg[STAGE_ID_SECTION]      = true;  return; }
		if (STRCMP_EQ(line, "[/section]"))      { data->read_flg[STAGE_ID_SECTION]      = false; return; }
	}

	if (data->read_flg[STAGE_ID_BASIC_INFO]) {
		load_basic_info(line);
	}
	if (data->read_flg[STAGE_ID_DAYTIME]) {
		load_daytime(line);
	}
	if (data->read_flg[STAGE_ID_PLAYER_START]) {
		load_player_start(line);
	}
	if (data->read_flg[STAGE_ID_GOAL]) {
		load_goal(line);
	}
	if (data->read_flg[STAGE_ID_NEXT_STAGE]) {
		load_next_stage(line);
	}
	if (data->read_flg[STAGE_ID_ITEMS_DEF]) {
		load_items_def(line);
	}
	if (data->read_flg[STAGE_ID_SECTION]) {
		load_section(line);
	}
}

int stage_manager_load(char* path)
{
	int ret = 0;

	// full_path = g_base_path + "data/" + path;
	char full_path[GAME_FULL_PATH_MAX];
	int tmp_path_size = game_utils_string_cat(full_path, g_base_path, (char*)"data/", path);
	if (tmp_path_size == 0) { LOG_ERROR("stage_manager_load failed get %s\n", path); return 1; }

	// init section 0
	{
		current_section_id = 0;
		current_section_data = new_section_data();
		current_section_data->id = 0;
		current_section_data->section_type = SECTION_TYPE_NORMAL;
		g_stage_data->section_list[current_section_id] = current_section_data;
		current_section_data->item_drop_rate = 4;

		//current_section_data->map_path = "map/common/n_empty.tmx";
		current_section_data->map_path = memory_manager_new_char_buff((int)strlen(empty_map_path));
		ret = game_utils_string_copy(current_section_data->map_path, empty_map_path);
		if (ret != 0) { LOG_ERROR("Error: stage_manager_load() copy empty_map_path\n"); return 1; }

		current_section_data->bgm_path = NULL;
		for (int i = 0; i < SECTION_ENEMY_PHASE_SIZE; i++) current_section_data->enemy_path[i] = NULL;
		current_section_data->trap_path = NULL;
		current_section_data->items_path = NULL;
	}

	// read file
	memset(load_stage_callback_data.read_flg, 0, sizeof(bool)* STAGE_ID_END);
	ret = game_utils_files_read_line(full_path, load_stage_callback, (void*)&load_stage_callback_data);
	if (ret != 0) { LOG_ERROR("stage_manager_load %s error\n", path); return 1; }

	// load section files
	ret = stage_manager_load_section_files();

	return ret;
}

static void load_basic_info(char* line) {
	char key[MEMORY_MANAGER_NAME_BUF_SIZE];
	char value[MEMORY_MANAGER_NAME_BUF_SIZE];
	game_utils_split_key_value(line, key, value);

	if (STRCMP_EQ(key, "id")) {
		//g_stage_data->id = value;
		g_stage_data->id = memory_manager_new_char_buff((int)strlen(value));
		int ret = game_utils_string_copy(g_stage_data->id, value);
		if (ret != 0) { LOG_ERROR("Error: stage_manager load_basic_info() get id\n"); return; }
		return;
	}
	if (STRCMP_EQ(key, "bonus_exp")) {
		g_stage_data->bonus_exp = atoi(value);
		return;
	}
	if (STRCMP_EQ(key,"friction_denominator")) {
		float denominator = (float)atoi(value);
		g_stage_data->friction_coef = 1.0f / denominator;
		return;
	}
}

static void load_daytime(char* line) {
	char key[MEMORY_MANAGER_NAME_BUF_SIZE];
	char value[MEMORY_MANAGER_NAME_BUF_SIZE];
	game_utils_split_key_value(line, key, value);

	if (STRCMP_EQ(key,"stat")) {
		if (STRCMP_EQ(value,"MORNING")) {
			g_stage_data->daytime_stat = STAGE_DAYTIME_STAT_MORNING;
		}
		else if (STRCMP_EQ(value,"AFTERNOON")) {
			g_stage_data->daytime_stat = STAGE_DAYTIME_STAT_AFTERNOON;
		}
		else if (STRCMP_EQ(value,"EVENING")) {
			g_stage_data->daytime_stat = STAGE_DAYTIME_STAT_EVENING;
		}
		else if (STRCMP_EQ(value,"LATE_NIGHT")) {
			g_stage_data->daytime_stat = STAGE_DAYTIME_STAT_LATE_NIGHT;
		}
		else {
			g_stage_data->daytime_stat = STAGE_DAYTIME_STAT_NONE;
		}
		return;
	}
	if (STRCMP_EQ(key, "frame_time")) {
		g_stage_data->daytime_frame_time = atoi(value);
		return;
	}
	if (STRCMP_EQ(key, "default_timer")) {
		g_stage_data->daytime_timer = atoi(value);
		return;
	}
}

static void load_player_start(char* line) {
	char key[MEMORY_MANAGER_NAME_BUF_SIZE];
	char value[MEMORY_MANAGER_NAME_BUF_SIZE];
	game_utils_split_key_value(line, key, value);

	if (STRCMP_EQ(key, "x")) {
		g_stage_data->start_x = atoi(value);
		return;
	}
	if (STRCMP_EQ(key, "y")) {
		g_stage_data->start_y = atoi(value);
		return;
	}
}

static void load_goal(char* line) {
	char key[MEMORY_MANAGER_NAME_BUF_SIZE];
	char value[MEMORY_MANAGER_NAME_BUF_SIZE];
	game_utils_split_key_value(line, key, value);

	if (STRCMP_EQ(key, "x")) {
		g_stage_data->goal_x = atoi(value);
		return;
	}
	if (STRCMP_EQ(key, "y")) {
		g_stage_data->goal_y = atoi(value);
		return;
	}
	if (STRCMP_EQ(key, "w")) {
		g_stage_data->goal_w = atoi(value);
		return;
	}
	if (STRCMP_EQ(key, "h")) {
		g_stage_data->goal_h = atoi(value);
		return;
	}
}

static void load_next_stage(char* line) {
	//g_stage_data->next_stage_id = line;
	g_stage_data->next_stage_id = memory_manager_new_char_buff((int)strlen(line));
	int ret = game_utils_string_copy(g_stage_data->next_stage_id, line);
	if (ret != 0) { LOG_ERROR("Error: stage_manager load_next_stage() get next_stage_id\n"); return; }
}

static void load_items_def(char* line) {
	items_data_t* new_item = (items_data_t*)game_utils_node_new(&common_items_list_buffer_info);

	//std::string items_def = line;
	new_item->path = memory_manager_new_char_buff((int)strlen(line));
	int ret = game_utils_string_copy(new_item->path, line);
	if (ret != 0) { LOG_ERROR("Error: stage_manager load_items_def() copy path \n"); return; }

	//g_stage_data->common_items_list.push_back(items_def);
	g_stage_data->common_items_list = &common_items_list_buffer_info;
}

static void load_section(char* line) {
	int ret = 0;
	char key[MEMORY_MANAGER_NAME_BUF_SIZE];
	char value[MEMORY_MANAGER_STRING_BUF_SIZE];
	game_utils_split_key_value(line, key, value);

	if (STRCMP_EQ(key,"section")) {
		current_section_id += 1;
		current_section_data = new_section_data();
		current_section_data->id = atoi(value);
		current_section_data->section_type = SECTION_TYPE_NORMAL;
		current_section_data->map_path = NULL;
		current_section_data->bgm_path = NULL;
		for (int i = 0; i < SECTION_ENEMY_PHASE_SIZE; i++) current_section_data->enemy_path[i] = NULL;
		current_section_data->trap_path = NULL;
		current_section_data->items_path = NULL;

		g_stage_data->section_list[current_section_id] = current_section_data;
		tmp_current_enemy_phase = -1;
		return;
	}
	if (STRCMP_EQ(key,"type")) {
		if (STRCMP_EQ(value,"BOSS")) current_section_data->section_type = SECTION_TYPE_BOSS;
		else if (STRCMP_EQ(value,"HIDE")) current_section_data->section_type = SECTION_TYPE_HIDE;
		else if (STRCMP_EQ(value,"ITEM")) current_section_data->section_type = SECTION_TYPE_ITEM;
		else if (STRCMP_EQ(value,"NEST")) current_section_data->section_type = SECTION_TYPE_NEST;
		else if (STRCMP_EQ(value,"NORMAL")) current_section_data->section_type = SECTION_TYPE_NORMAL;
		else current_section_data->section_type = SECTION_TYPE_NONE;

		return;
	}
	if (STRCMP_EQ(key, "item_drop_rate")) {
		current_section_data->item_drop_rate = atoi(value);
		return;
	}

	if (STRCMP_EQ(key, "map_path")) {
		//current_section_data->map_path = value;
		current_section_data->map_path = memory_manager_new_char_buff((int)strlen(value));
		ret = game_utils_string_copy(current_section_data->map_path, value);
		if (ret != 0) { LOG_ERROR("Error: stage_manager load_section() copy map_path\n"); return; }
		return;
	}
	if (STRCMP_EQ(key,"bgm_path")) {
		//current_section_data->bgm_path = value;
		current_section_data->bgm_path = memory_manager_new_char_buff((int)strlen(value));
		ret = game_utils_string_copy(current_section_data->bgm_path, value);
		if (ret != 0) { LOG_ERROR("Error: stage_manager load_section() copy bgm_path\n"); return; }

		BGM_data_t* new_bgm = (BGM_data_t*)game_utils_node_new(&bgm_list_buffer_info[current_section_id]);
		new_bgm->res_chunk = resource_manager_getChunkFromPath(value);
		current_section_data->bgm_list = &bgm_list_buffer_info[current_section_id];
		return;
	}

	if (STRCMP_EQ(key,"enemy_path")) {
		tmp_current_enemy_phase += 1;
		if (tmp_current_enemy_phase >= SECTION_ENEMY_PHASE_SIZE) {
			LOG_ERROR("ERROR: section %d enemy_path overflow", current_section_data->id);
		}
		else {
			//current_section_data->enemy_path[tmp_current_enemy_phase] = value;
			current_section_data->enemy_path[tmp_current_enemy_phase] = memory_manager_new_char_buff((int)strlen(value));
			ret = game_utils_string_copy(current_section_data->enemy_path[tmp_current_enemy_phase], value);
			if (ret != 0) { LOG_ERROR("Error: stage_manager load_section() copy enemy_path %d\n", tmp_current_enemy_phase); return; }
		}
		return;
	}
	if (STRCMP_EQ(key, "trap_path")) {
		//current_section_data->trap_path = value;
		current_section_data->trap_path = memory_manager_new_char_buff((int)strlen(value));
		ret = game_utils_string_copy(current_section_data->trap_path, value);
		if (ret != 0) { LOG_ERROR("Error: stage_manager load_section() copy trap_path\n"); return; }
		return;
	}
	if (STRCMP_EQ(key, "items_path")) {
		//current_section_data->items_path = value;
		current_section_data->items_path = memory_manager_new_char_buff((int)strlen(value));
		ret = game_utils_string_copy(current_section_data->items_path, value);
		if (ret != 0) { LOG_ERROR("Error: stage_manager load_section() copy items_path\n"); return; }
		return;
	}
}

static int stage_manager_load_section_files()
{
	for (int sec_i = 0; sec_i < SECTION_DATA_BUFFER_SIZE; sec_i++) {
		current_section_id = sec_i;
		current_section_data = g_stage_data->section_list[sec_i];
		if (current_section_data == NULL) break;

		for (int phase = 0; phase < SECTION_ENEMY_PHASE_SIZE; phase++) {
			tmp_current_enemy_phase = phase;
			if (current_section_data->enemy_path[phase] != NULL) load_section_file(current_section_data->enemy_path[phase]);
		}
		if (current_section_data->trap_path != NULL) load_section_file(current_section_data->trap_path);
		if (current_section_data->items_path != NULL) load_section_file(current_section_data->items_path);
	}
	return 0;
}

typedef struct _load_section_file_callback_data_t load_section_file_callback_data_t;
struct _load_section_file_callback_data_t {
	bool read_flg[SECTION_ID_END];
};
static load_section_file_callback_data_t load_section_file_callback_data;
static void load_section_file_callback(char* line, int line_size, int line_num, void* argv)
{
	load_section_file_callback_data_t* data = (load_section_file_callback_data_t*)argv;

	if (line[0] == '\0') return;
	if (line[0] == '#') return;

	if (line[0] == '[') {
		if (STRCMP_EQ(line, "[enemy]"))       { data->read_flg[SECTION_ID_ENEMY]      = true;  return; }
		if (STRCMP_EQ(line, "[/enemy]"))      { data->read_flg[SECTION_ID_ENEMY]      = false; return; }
		if (STRCMP_EQ(line, "[trap]"))        { data->read_flg[SECTION_ID_TRAP]       = true;  return; }
		if (STRCMP_EQ(line, "[/trap]"))       { data->read_flg[SECTION_ID_TRAP]       = false; return; }
		if (STRCMP_EQ(line, "[items]"))       { data->read_flg[SECTION_ID_ITEMS]      = true;  return; }
		if (STRCMP_EQ(line, "[/items]"))      { data->read_flg[SECTION_ID_ITEMS]      = false; return; }
		if (STRCMP_EQ(line, "[drop_items]"))  { data->read_flg[SECTION_ID_DROP_ITEMS] = true;  return; }
		if (STRCMP_EQ(line, "[/drop_items]")) { data->read_flg[SECTION_ID_DROP_ITEMS] = false; return; }
		if (STRCMP_EQ(line, "[goal_items]"))  { data->read_flg[SECTION_ID_GOAL_ITEMS] = true;  return; }
		if (STRCMP_EQ(line, "[/goal_items]")) { data->read_flg[SECTION_ID_GOAL_ITEMS] = false; return; }
	}

	if (data->read_flg[SECTION_ID_ENEMY]) {
		load_enemy(line);
	}
	if (data->read_flg[SECTION_ID_TRAP]) {
		load_trap(line);
	}
	if (data->read_flg[SECTION_ID_ITEMS]) {
		load_items(line);
	}
	if (data->read_flg[SECTION_ID_DROP_ITEMS]) {
		load_drop_items(line);
	}
	if (data->read_flg[SECTION_ID_GOAL_ITEMS]) {
		load_goal_items(line);
	}
}

static int load_section_file(char* path)
{
	// full_path = g_base_path + "data/" + path;
	char full_path[GAME_FULL_PATH_MAX];
	int tmp_path_size = game_utils_string_cat(full_path, g_base_path, (char*)"data/", path);
	if (tmp_path_size == 0) { LOG_ERROR("load_section_file failed get %s\n", path); return 1; }

	// read file
	memset(load_section_file_callback_data.read_flg, 0, sizeof(bool) * SECTION_ID_END);
	int ret = game_utils_files_read_line(full_path, load_section_file_callback, (void*)&load_section_file_callback_data);
	if (ret != 0) { LOG_ERROR("load_section_file %s error\n", path); return 1; }

	return 0;
}

static void load_items(char* line) {
	char key[MEMORY_MANAGER_NAME_BUF_SIZE];
	char value[MEMORY_MANAGER_LINE_BUF_SIZE];
	int val_list[GAME_UTILS_STRING_VALUE_LIST_SIZE_MAX];
	int val_list_size;
	game_utils_split_key_value(line, key, value);

	if (STRCMP_EQ(key,"type")) {
		items_data_t* new_item = (items_data_t*)game_utils_node_new(&items_list_buffer_info[current_section_id]);

		//new_item->path = value;
		new_item->path = memory_manager_new_char_buff((int)strlen(value));
		int ret = game_utils_string_copy(new_item->path, value);
		if (ret != 0) { LOG_ERROR("Error: stage_manager load_items() copy path \n"); return; }

		new_item->x = -1; // disable
		new_item->y = -1; // disable
		current_section_data->items_list = &items_list_buffer_info[current_section_id];

		tmp_new_node = (node_data_t*)new_item;
		return;
	}

	char* item_type = ((items_data_t*)tmp_new_node)->path;
	if (STRCMP_EQ(key,"x")) {
		val_list_size = game_utils_split_conmma_int(value, val_list, GAME_UTILS_STRING_VALUE_LIST_SIZE_MAX);

		((items_data_t*)tmp_new_node)->x = val_list[0];
		for (int i = 1; i < val_list_size; i++) {
			items_data_t* new_item = (items_data_t*)game_utils_node_new(current_section_data->items_list);
			new_item->path = item_type;
			new_item->x = val_list[i];
			new_item->y = -1; // disable
		}
		return;
	}
	if (STRCMP_EQ(key,"y")) {
		val_list_size = game_utils_split_conmma_int(value, val_list, GAME_UTILS_STRING_VALUE_LIST_SIZE_MAX);

		node_data_t* node = tmp_new_node;
		for (int i = 0; i < val_list_size; i++) {
			if (node == NULL) break;
			((items_data_t*)node)->y = val_list[i];
			node = node->next;
		}
		return;
	}
}

static void load_drop_items(char* line) {
	items_data_t* new_item = (items_data_t*)game_utils_node_new(&drop_items_list_buffer_info[current_section_id]);

	//std::string drop_items = line;
	new_item->path = memory_manager_new_char_buff((int)strlen(line));
	int ret = game_utils_string_copy(new_item->path, line);
	if (ret != 0) { LOG_ERROR("Error: stage_manager load_drop_items() copy path \n"); return; }

	//current_section_data->drop_items_list.push_back(drop_items);
	current_section_data->drop_items_list = &drop_items_list_buffer_info[current_section_id];
}

static void load_goal_items(char* line) {
	items_data_t* new_item = (items_data_t*)game_utils_node_new(&goal_items_list_buffer_info[current_section_id]);

	//std::string goal_items = line;
	new_item->path = memory_manager_new_char_buff((int)strlen(line));
	int ret = game_utils_string_copy(new_item->path, line);
	if (ret != 0) { LOG_ERROR("Error: stage_manager load_goal_items() copy path \n"); return; }

	//current_section_data->goal_items_list.push_back(goal_items);
	current_section_data->goal_items_list = &goal_items_list_buffer_info[current_section_id];
}

static void load_trap(char* line) {
	char key[MEMORY_MANAGER_NAME_BUF_SIZE];
	char value[MEMORY_MANAGER_LINE_BUF_SIZE];
	int val_list[GAME_UTILS_STRING_VALUE_LIST_SIZE_MAX];
	int val_list_size;
	game_utils_split_key_value(line, key, value);

	if (STRCMP_EQ(key,"type")) {
		trap_data_t* new_trap = (trap_data_t*)game_utils_node_new(&trap_list_buffer_info[current_section_id]);
		//new_trap->path = value;
		new_trap->path = memory_manager_new_char_buff((int)strlen(value));
		game_utils_string_copy(new_trap->path, value);
		new_trap->x = -1; // disable
		new_trap->y = -1; // disable
		current_section_data->trap_list = &trap_list_buffer_info[current_section_id];

		tmp_new_node = (node_data_t*)new_trap;
		return;
	}

	char* trap_type = ((trap_data_t*)tmp_new_node)->path;
	if (STRCMP_EQ(key,"x")) {
		val_list_size = game_utils_split_conmma_int(value, val_list, GAME_UTILS_STRING_VALUE_LIST_SIZE_MAX);

		((trap_data_t*)tmp_new_node)->x = val_list[0];
		for (int i = 1; i < val_list_size; i++) {
			trap_data_t* new_trap = (trap_data_t*)game_utils_node_new(current_section_data->trap_list);
			new_trap->path = trap_type;
			new_trap->x = val_list[i];
			new_trap->y = -1; // disable
		}
		return;
	}
	if (STRCMP_EQ(key,"y")) {
		val_list_size = game_utils_split_conmma_int(value, val_list, GAME_UTILS_STRING_VALUE_LIST_SIZE_MAX);

		node_data_t* node = tmp_new_node;
		for (int i = 0; i < val_list_size; i++) {
			if (node == NULL) break;
			((trap_data_t*)node)->y = val_list[i];
			node = node->next;
		}
		return;
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

static void load_enemy(char* line) {
	char key[MEMORY_MANAGER_NAME_BUF_SIZE];
	char value[MEMORY_MANAGER_LINE_BUF_SIZE];
	char str_list[MEMORY_MANAGER_NAME_BUF_SIZE * GAME_UTILS_STRING_VALUE_LIST_SIZE_MAX];
	int str_list_size;
	int val_list[GAME_UTILS_STRING_VALUE_LIST_SIZE_MAX];
	int val_list_size;
	game_utils_split_key_value(line, key, value);

	if (STRCMP_EQ(key,"type")) {
		int enemy_list_buffer_index = current_section_id * SECTION_ENEMY_PHASE_SIZE + tmp_current_enemy_phase;
		enemy_data_t* new_enemy = (enemy_data_t*)game_utils_node_new(&enemy_list_buffer_info[enemy_list_buffer_index]);
		clear_enemy(new_enemy);

		//new_enemy->path = value;
		new_enemy->path = memory_manager_new_char_buff((int)strlen(value));
		game_utils_string_copy(new_enemy->path, value);
		current_section_data->enemy_list[tmp_current_enemy_phase] = &enemy_list_buffer_info[enemy_list_buffer_index];

		tmp_new_node = (node_data_t*)new_enemy;
		return;
	}

	//std::string enemy_type = ((enemy_data_t*)tmp_new_node)->path;
	char* enemy_type = ((enemy_data_t*)tmp_new_node)->path;
	if (STRCMP_EQ(key,"x")) {
		val_list_size = game_utils_split_conmma_int(value, val_list, GAME_UTILS_STRING_VALUE_LIST_SIZE_MAX);

		((enemy_data_t*)tmp_new_node)->x = val_list[0];
		for (int i = 1; i < val_list_size; i++) {
			enemy_data_t* new_enemy = (enemy_data_t*)game_utils_node_new(current_section_data->enemy_list[tmp_current_enemy_phase]);
			clear_enemy(new_enemy);

			new_enemy->path = enemy_type;
			new_enemy->x = val_list[i];
		}
		return;
	}
	if (STRCMP_EQ(key,"y")) {
		val_list_size = game_utils_split_conmma_int(value, val_list, GAME_UTILS_STRING_VALUE_LIST_SIZE_MAX);

		node_data_t* node = tmp_new_node;
		for (int i = 0; i < val_list_size; i++) {
			if (node == NULL) break;
			((enemy_data_t*)node)->y = val_list[i];
			node = node->next;
		}
		return;
	}
	if (STRCMP_EQ(key,"vec_x")) {
		val_list_size = game_utils_split_conmma_int(value, val_list, GAME_UTILS_STRING_VALUE_LIST_SIZE_MAX);

		node_data_t* node = tmp_new_node;
		for (int i = 0; i < val_list_size; i++) {
			if (node == NULL) break;
			((enemy_data_t*)node)->vec_x = val_list[i];
			node = node->next;
		}
		return;
	}
	if (STRCMP_EQ(key,"vec_y")) {
		val_list_size = game_utils_split_conmma_int(value, val_list, GAME_UTILS_STRING_VALUE_LIST_SIZE_MAX);

		node_data_t* node = tmp_new_node;
		for (int i = 0; i < val_list_size; i++) {
			if (node == NULL) break;
			((enemy_data_t*)node)->vec_y = val_list[i];
			node = node->next;
		}
		return;
	}
	if (STRCMP_EQ(key,"delay")) {
		val_list_size = game_utils_split_conmma_int(value, val_list, GAME_UTILS_STRING_VALUE_LIST_SIZE_MAX);

		node_data_t* node = tmp_new_node;
		for (int i = 0; i < val_list_size; i++) {
			if (node == NULL) break;
			((enemy_data_t*)node)->delay = val_list[i];
			node = node->next;
		}
		return;
	}
	if (STRCMP_EQ(key,"face")) {
		str_list_size = game_utils_split_conmma(value, str_list, GAME_UTILS_STRING_VALUE_LIST_SIZE_MAX, MEMORY_MANAGER_NAME_BUF_SIZE);

		node_data_t* node = tmp_new_node;
		for (int i = 0; i < str_list_size; i++) {
			if (node == NULL) break;

			char* face_str = &str_list[i * MEMORY_MANAGER_NAME_BUF_SIZE];
			if (STRCMP_EQ(face_str,"N")) {
				((enemy_data_t*)node)->face = UNIT_FACE_N;
			}
			else if (STRCMP_EQ(face_str,"E")) {
				((enemy_data_t*)node)->face = UNIT_FACE_E;
			}
			else if (STRCMP_EQ(face_str,"W")) {
				((enemy_data_t*)node)->face = UNIT_FACE_W;
			}
			else if (STRCMP_EQ(face_str,"S")) {
				((enemy_data_t*)node)->face = UNIT_FACE_S;
			}
			else {
				((enemy_data_t*)node)->face = UNIT_FACE_NONE;
			}
			node = node->next;
		}
		return;
	}
	if (STRCMP_EQ(key,"ai_step")) {
		val_list_size = game_utils_split_conmma_int(value, val_list, GAME_UTILS_STRING_VALUE_LIST_SIZE_MAX);

		node_data_t* node = tmp_new_node;
		for (int i = 0; i < val_list_size; i++) {
			if (node == NULL) break;
			((enemy_data_t*)node)->ai_step = val_list[i];
			node = node->next;
		}
		return;
	}
}

// daytime I/F
void stage_manager_daytime_init()
{
	g_stage_data->daytime_stat = STAGE_DAYTIME_STAT_NONE;
	g_stage_data->daytime_frame_time = STAGE_DAYTIME_FRAME_DEFAULT_VAL;
	g_stage_data->daytime_timer = STAGE_DAYTIME_DEFAULT_VAL;
}

int stage_manager_daytime_get_hour(int daytime_timer)
{
	return daytime_timer / 3600;
}

int stage_manager_daytime_get_minutes(int daytime_timer)
{
	return (daytime_timer % 3600) / 60;
}

int stage_manager_daytime_get_seconds(int daytime_timer)
{
	return daytime_timer % 60;
}

void stage_manager_daytime_update()
{
	// if disable daytime mode
	if (g_stage_data->daytime_stat == STAGE_DAYTIME_STAT_NONE) return;

	g_stage_data->daytime_timer += g_stage_data->daytime_frame_time;

	if (g_stage_data->daytime_stat == STAGE_DAYTIME_STAT_MORNING) {
		if (g_stage_data->daytime_timer > STAGE_DAYTIME_MORNING_MAX) {
			g_stage_data->daytime_stat = STAGE_DAYTIME_STAT_AFTERNOON;
			quest_log_manager_message("Good AfterNoon!!!");
		}
	}
	else if (g_stage_data->daytime_stat == STAGE_DAYTIME_STAT_AFTERNOON) {
		if (g_stage_data->daytime_timer > STAGE_DAYTIME_AFTERNOON_MAX) {
			g_stage_data->daytime_stat = STAGE_DAYTIME_STAT_EVENING;
			quest_log_manager_message("Good Evening!!!");
		}
	}
	else if (g_stage_data->daytime_stat == STAGE_DAYTIME_STAT_EVENING) {
		if (g_stage_data->daytime_timer > STAGE_DAYTIME_EVENING_MAX) {
			g_stage_data->daytime_stat = STAGE_DAYTIME_STAT_LATE_NIGHT;

			// reset timer
			g_stage_data->daytime_timer -= STAGE_DAYTIME_MAX_VAL;
			quest_log_manager_message("Good Late-Night!!!");
		}
	}
	else if (g_stage_data->daytime_stat == STAGE_DAYTIME_STAT_LATE_NIGHT) {
		if (g_stage_data->daytime_timer > STAGE_DAYTIME_LATE_NIGHT_MAX) {
			g_stage_data->daytime_stat = STAGE_DAYTIME_STAT_MORNING;
			quest_log_manager_message("Good Morning!!!");
		}
	}
}
