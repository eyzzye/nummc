#include <vector>
#include <string>
#include "game_common.h"
#include "story_manager.h"

#include "resource_manager.h"
#include "game_utils.h"
#include "game_log.h"

#define STORY_ID_BASIC_INFO   0
#define STORY_ID_IMG          1
#define STORY_ID_BGM          2
#define STORY_ID_AUTO_TEXT    3
#define STORY_ID_END          4

story_data_t* g_story_data;
story_data_t g_story_data_buffer;

static void load_basic_info(char* line);
static void load_img(char* line);
static void load_bgm(char* line);
static void load_auto_text(char* line);

void story_manager_init()
{
	g_story_data = &g_story_data_buffer;
	g_story_data->stat = STORY_STAT_NONE;
}

void story_manager_unload()
{
	g_story_data->auto_text_list.clear();
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
	if (tmp_path_size == 0) {
		LOG_ERROR("story_manager_load failed get %s\n", path);
		return 1;
	}

	g_story_data->story_path = path;
	g_story_data->res_img = NULL;
	g_story_data->res_chunk = NULL;

	// read file
	memset(load_story_callback_data.read_flg, 0, sizeof(bool)* STORY_ID_END);
	int ret = game_utils_files_read_line(full_path, load_story_callback, (void*)&load_story_callback_data);
	if (ret != 0) {
		LOG_ERROR("story_manager_load %s error\n", path);
		return 1;
	}

	return 0;
}

static void load_basic_info(char* line) {
	char key[GAME_UTILS_STRING_NAME_BUF_SIZE];
	char value[GAME_UTILS_STRING_NAME_BUF_SIZE];
	game_utils_split_key_value(line, key, value);

	if (STRCMP_EQ(key, "id")) {
		g_story_data->id = value;
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
		g_story_data->auto_text_list.push_back(line);
	}
}
