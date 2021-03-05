#include "game_common.h"
#include "story_manager.h"

#include "resource_manager.h"
#include "memory_manager.h"
#include "game_utils.h"
#include "game_log.h"

#define STORY_ID_BASIC_INFO   0
#define STORY_ID_IMG          1
#define STORY_ID_BGM          2
#define STORY_ID_AUTO_TEXT    3
#define STORY_ID_END          4

story_data_t* g_story_data;
story_data_t g_story_data_buffer;
node_buffer_info_t auto_text_list_inst;

static void load_basic_info(char* line);
static void load_img(char* line);
static void load_bgm(char* line);
static void load_auto_text(char* line);

void story_manager_init()
{
	g_story_data = &g_story_data_buffer;
	g_story_data->stat = STORY_STAT_NONE;
	game_utils_node_init(&auto_text_list_inst, (int)sizeof(auto_text_data_t));
	g_story_data->auto_text_list = &auto_text_list_inst;
}

void story_manager_unload()
{
	//g_story_data->auto_text_list.clear();
	if (g_story_data->auto_text_list->used_buffer_size > 0) {
		node_data_t* node = g_story_data->auto_text_list->start_node;
		while (node != NULL) {
			node_data_t* del_node = node;
			node = node->next;

			game_utils_node_delete(del_node, g_story_data->auto_text_list);
		}
	}
	g_story_data->auto_text_list = NULL;

	// releae char_buf
	if (g_story_data->id) { memory_manager_delete_char_buff(g_story_data->id); g_story_data->id = NULL; }
	if (g_story_data->story_path) { memory_manager_delete_char_buff(g_story_data->story_path); g_story_data->story_path = NULL; }
}

void story_manager_set_stat(int stat)
{
	g_story_data->stat = stat;
}

typedef struct _load_story_callback_data_t load_story_callback_data_t;
struct _load_story_callback_data_t {
	bool read_flg[STORY_ID_END];
};
static load_story_callback_data_t load_story_callback_data;
static void load_story_callback(char* line, int line_size, int line_num, void* argv)
{
	load_story_callback_data_t* data = (load_story_callback_data_t*)argv;

	if (line[0] == '\0') return;
	if (line[0] == '#') return;

	if (line[0] == '[') {
		if (STRCMP_EQ(line, "[basic_info]"))  { data->read_flg[STORY_ID_BASIC_INFO] = true;  return; }
		if (STRCMP_EQ(line, "[/basic_info]")) { data->read_flg[STORY_ID_BASIC_INFO] = false; return; }
		if (STRCMP_EQ(line, "[img]"))         { data->read_flg[STORY_ID_IMG]        = true;  return; }
		if (STRCMP_EQ(line, "[/img]"))        { data->read_flg[STORY_ID_IMG]        = false; return; }
		if (STRCMP_EQ(line, "[bgm]"))         { data->read_flg[STORY_ID_BGM]        = true;  return; }
		if (STRCMP_EQ(line, "[/bgm]"))        { data->read_flg[STORY_ID_BGM]        = false; return; }
		if (STRCMP_EQ(line, "[auto_text]"))   { data->read_flg[STORY_ID_AUTO_TEXT]  = true;  return; }
		if (STRCMP_EQ(line, "[/auto_text]"))  { data->read_flg[STORY_ID_AUTO_TEXT]  = false; return; }
	}

	if (data->read_flg[STORY_ID_BASIC_INFO]) {
		load_basic_info(line);
	}
	if (data->read_flg[STORY_ID_IMG]) {
		load_img(line);
	}
	if (data->read_flg[STORY_ID_BGM]) {
		load_bgm(line);
	}
	if (data->read_flg[STORY_ID_AUTO_TEXT]) {
		load_auto_text(line);
	}
}

int story_manager_load(char* path)
{
	// full_path = g_base_path + "data/" + path;
	char full_path[GAME_FULL_PATH_MAX];
	int tmp_path_size = game_utils_string_cat(full_path, g_base_path, (char*)"data/", path);
	if (tmp_path_size == 0) { LOG_ERROR("story_manager_load failed get %s\n", path); return 1; }

	//g_story_data->story_path = path;
	g_story_data->story_path = memory_manager_new_char_buff((int)strlen(path));
	int ret = game_utils_string_copy(g_story_data->story_path, path);
	if (ret != 0) { LOG_ERROR("Error: story_manager_load() copy path\n"); return 1; }

	g_story_data->res_img = NULL;
	g_story_data->res_chunk = NULL;

	// read file
	memset(load_story_callback_data.read_flg, 0, sizeof(bool)* STORY_ID_END);
	ret = game_utils_files_read_line(full_path, load_story_callback, (void*)&load_story_callback_data);
	if (ret != 0) { LOG_ERROR("story_manager_load %s error\n", path); return 1; }

	return 0;
}

static void load_basic_info(char* line) {
	char key[GAME_UTILS_STRING_NAME_BUF_SIZE];
	char value[GAME_UTILS_STRING_NAME_BUF_SIZE];
	game_utils_split_key_value(line, key, value);

	if (STRCMP_EQ(key, "id")) {
		//g_story_data->id = value;
		g_story_data->id = memory_manager_new_char_buff((int)strlen(value));
		int ret = game_utils_string_copy(g_story_data->id, value);
		if (ret != 0) { LOG_ERROR("Error: story_manager load_basic_info() copy id\n"); return; }
		return;
	}
}

static void load_img(char* line) {
	if (line[0] != '\0') {
		g_story_data->res_img = resource_manager_getTextureFromPath(line);
	}
}

static void load_bgm(char* line) {
	if (line[0] != '\0') {
		g_story_data->res_chunk = resource_manager_getChunkFromPath(line);
	}
}

static void load_auto_text(char* line) {
	char key[GAME_UTILS_STRING_NAME_BUF_SIZE];
	char value[GAME_UTILS_STRING_NAME_BUF_SIZE];
	game_utils_split_key_value(line, key, value);

	if (STRCMP_EQ(key,"time")) {
		g_story_data->auto_text_time = atoi(value);
		return;
	}
	else {
		//g_story_data->auto_text_list.push_back(line);
		auto_text_data_t* auto_text_data = (auto_text_data_t*)game_utils_node_new(g_story_data->auto_text_list);
		int ret = game_utils_string_copy(auto_text_data->auto_text, line);
		if (ret != 0) { LOG_ERROR("Error: story_manager load_auto_text() copy auto_text\n"); return; }
	}
}
