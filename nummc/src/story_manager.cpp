#include <vector>
#include <string>
#include <fstream>
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

static void load_basic_info(std::string& line);
static void load_img(std::string& line);
static void load_bgm(std::string& line);
static void load_auto_text(std::string& line);

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

int story_manager_load(std::string path)
{
	bool read_flg[STORY_ID_END] = { false };

	// full_path = g_base_path + "data/" + path;
	char full_path[GAME_FULL_PATH_MAX];
	int tmp_path_size = game_utils_string_cat(full_path, g_base_path, (char*)"data/", (char*)path.c_str());
	if (tmp_path_size == 0) {
		LOG_ERROR("story_manager_load failed get %s\n", path.c_str());
		return 1;
	}

	std::ifstream inFile(full_path);
	if (inFile.is_open()) {
		g_story_data->story_path = path;
		g_story_data->res_img = NULL;
		g_story_data->res_chunk = NULL;

		std::string line;
		while (std::getline(inFile, line)) {
			if (line == "") continue;
			if (line[0] == '#') continue;

			if (line == "[basic_info]") { read_flg[STORY_ID_BASIC_INFO] = true;  continue; }
			if (line == "[/basic_info]") { read_flg[STORY_ID_BASIC_INFO] = false; continue; }
			if (line == "[img]") { read_flg[STORY_ID_IMG] = true;  continue; }
			if (line == "[/img]") { read_flg[STORY_ID_IMG] = false; continue; }
			if (line == "[bgm]") { read_flg[STORY_ID_BGM] = true;  continue; }
			if (line == "[/bgm]") { read_flg[STORY_ID_BGM] = false; continue; }
			if (line == "[auto_text]") { read_flg[STORY_ID_AUTO_TEXT] = true;  continue; }
			if (line == "[/auto_text]") { read_flg[STORY_ID_AUTO_TEXT] = false; continue; }

			if (read_flg[STORY_ID_BASIC_INFO]) {
				load_basic_info(line);
			}
			if (read_flg[STORY_ID_IMG]) {
				load_img(line);
			}
			if (read_flg[STORY_ID_BGM]) {
				load_bgm(line);
			}
			if (read_flg[STORY_ID_AUTO_TEXT]) {
				load_auto_text(line);
			}
		}
		inFile.close();
	}
	else {
		LOG_ERROR("story_manager_load %s error\n", path.c_str());
		return 1;
	}
	return 0;
}

static void load_basic_info(std::string& line) {
	char key[GAME_UTILS_STRING_NAME_BUF_SIZE];
	char value[GAME_UTILS_STRING_NAME_BUF_SIZE];
	game_utils_split_key_value((char*)line.c_str(), key, value);

	if (STRCMP_EQ(key, "id")) {
		g_story_data->id = value;
		return;
	}
}

static void load_img(std::string& line) {
	if (line != "") {
		g_story_data->res_img = resource_manager_getTextureFromPath(line);
	}
}

static void load_bgm(std::string& line) {
	if (line != "") {
		g_story_data->res_chunk = resource_manager_getChunkFromPath(line);
	}
}

static void load_auto_text(std::string& line) {
	char key[GAME_UTILS_STRING_NAME_BUF_SIZE];
	char value[GAME_UTILS_STRING_NAME_BUF_SIZE];
	game_utils_split_key_value((char*)line.c_str(), key, value);

	if (STRCMP_EQ(key,"time")) {
		g_story_data->auto_text_time = atoi(value);
		return;
	}
	else {
		g_story_data->auto_text_list.push_back(line);
	}
}
