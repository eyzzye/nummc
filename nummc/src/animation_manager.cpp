#include <fstream>
#include "game_common.h"
#include "animation_manager.h"

#include "game_log.h"
#include "game_utils.h"
#include "resource_manager.h"
#include "sound_manager.h"

#define ANIM_TAG_FRAME      0
#define ANIM_TAG_IMG        1
#define ANIM_TAG_SND        2
#define ANIM_TAG_END        3

#define ANIM_DATA_LIST_SIZE 256
static anim_data_t anim_data_list[ANIM_DATA_LIST_SIZE];
static anim_data_t* anim_data_list_start;
static anim_data_t* anim_data_list_end;
int anim_data_index_end;

#define ANIM_STAT_BASE_LIST_SIZE (ANIM_DATA_LIST_SIZE * ANIM_STAT_END)
static anim_stat_base_data_t anim_stat_base_data_list[ANIM_STAT_BASE_LIST_SIZE];
int anim_stat_base_data_index_end;

#define ANIM_STAT_LIST_SIZE (ANIM_DATA_LIST_SIZE * ANIM_STAT_END)
static anim_stat_data_t anim_stat_data_list[ANIM_STAT_LIST_SIZE];
int anim_stat_data_index_end;

static void load_anim_frame(std::string line, anim_data_t* anim_data, int stat);
static void load_anim_img(std::string line, anim_data_t* anim_data, int stat);
static void load_anim_snd(std::string line, anim_data_t* anim_data, int stat);

static std::string dir_path;

int animation_manager_init()
{
	memset(anim_data_list, 0, sizeof(anim_data_list));
	memset(anim_stat_base_data_list, 0, sizeof(anim_stat_base_data_list));
	memset(anim_stat_data_list, 0, sizeof(anim_stat_data_list));

	anim_data_index_end = 0;
	anim_data_list_start = NULL;
	anim_data_list_end = NULL;
	anim_stat_base_data_index_end = 0;
	anim_stat_data_index_end = 0;

	return 0;
}

void animation_manager_unload()
{
	for (int i = 0; i < ANIM_STAT_BASE_LIST_SIZE; i++) {
		if (anim_stat_base_data_list[i].obj) {
			delete [] anim_stat_base_data_list[i].obj;
			anim_stat_base_data_list[i].obj = NULL;
		}

		for (int fi = 0; fi < anim_stat_base_data_list[i].frame_size; fi++) {
			if (anim_stat_base_data_list[i].frame_list[fi]) {
				delete anim_stat_base_data_list[i].frame_list[fi];
				anim_stat_base_data_list[i].frame_list[fi] = NULL;
			}
		}
	}
}

void animation_manager_delete_anim_stat_data(anim_data_t* delete_data)
{
	anim_data_t* tmp1 = delete_data->prev;
	anim_data_t* tmp2 = delete_data->next;
	if (tmp1) tmp1->next = tmp2;
	if (tmp2) tmp2->prev = tmp1;

	if (delete_data == anim_data_list_start) anim_data_list_start = tmp2;
	if (delete_data == anim_data_list_end) anim_data_list_end = tmp1;

	// clear anim_stat_data
	for (int i = 0; i < ANIM_STAT_END; i++) {
		if (delete_data->anim_stat_list[i] != NULL) {
			delete_data->anim_stat_list[i]->type = ANIM_DATA_DISABLE;
		}
	}
	memset(delete_data, 0, sizeof(anim_data_t));
}

anim_frame_data_t* animation_manager_new_anim_frame()
{
	anim_frame_data_t* anim_frame_data = new anim_frame_data_t;
	return anim_frame_data;
}

void animation_manager_new_anim_stat_base_data(anim_data_t* anim_data)
{
	for (int i = 0; i < ANIM_STAT_END; i++) {
		anim_data->anim_stat_base_list[i] = &anim_stat_base_data_list[anim_stat_base_data_index_end];
		anim_stat_base_data_index_end += 1;
	}
}

anim_stat_data_t* animation_manager_new_anim_stat_data()
{
	int return_index = -1;
	if (anim_stat_data_list[anim_stat_data_index_end].type == ANIM_DATA_DISABLE)
	{
		return_index = anim_stat_data_index_end;
		anim_stat_data_list[return_index].type = ANIM_DATA_ENABLE;
		anim_stat_data_index_end += 1;
		if (anim_stat_data_index_end >= ANIM_STAT_LIST_SIZE) anim_stat_data_index_end = 0;
		return &anim_stat_data_list[return_index];
	}

	for (int i = anim_stat_data_index_end; i < ANIM_STAT_LIST_SIZE; i++) {
		if (anim_stat_data_list[i].type == ANIM_DATA_DISABLE)
		{
			return_index = i;
			anim_stat_data_list[return_index].type = ANIM_DATA_ENABLE;
			anim_stat_data_index_end = i + 1;
			if (anim_stat_data_index_end >= ANIM_STAT_LIST_SIZE) anim_stat_data_index_end = 0;
			return &anim_stat_data_list[return_index];
		}
	}

	if (anim_stat_data_index_end > 0) {
		for (int i = 0; i < anim_stat_data_index_end; i++) {
			if (anim_stat_data_list[i].type == ANIM_DATA_DISABLE)
			{
				return_index = i;
				anim_stat_data_list[return_index].type = ANIM_DATA_ENABLE;
				anim_stat_data_index_end = i + 1;
				if (anim_stat_data_index_end >= ANIM_STAT_LIST_SIZE) anim_stat_data_index_end = 0;
				return &anim_stat_data_list[return_index];
			}
		}
	}

	LOG_ERROR("animation_manager_new_anim_stat_data error\n");
	return NULL;
}

anim_data_t* animation_manager_new_anim_data()
{
	anim_data_t* anim_data = NULL;

	int start_index = anim_data_index_end % ANIM_DATA_LIST_SIZE;
	int new_index = -1;
	for (int i = 0; i < ANIM_DATA_LIST_SIZE; i++) {
		int index = start_index + i;
		if (index >= ANIM_DATA_LIST_SIZE) index -= ANIM_DATA_LIST_SIZE;
		if (anim_data_list[index].type == ANIM_TYPE_NONE) {
			new_index = index;
			anim_data_index_end += i;
			break;
		}
	}
	if (new_index == -1) {
		LOG_ERROR("animation_manager_new_anim_data error\n");
		return NULL;
	}

	anim_data = &anim_data_list[new_index];
	anim_data->id = anim_data_index_end;
	anim_data->type = ANIM_TYPE_DYNAMIC;

	for (int i = 0; i < ANIM_STAT_END; i++) {
		anim_data->anim_stat_list[i] = animation_manager_new_anim_stat_data();
		anim_data->anim_stat_list[i]->current_time = 0;
		anim_data->anim_stat_list[i]->current_frame = 0;
		anim_data->anim_stat_list[i]->chunk_frame = -1;
		anim_data->anim_stat_list[i]->command_frame = -1;
	}

	// set prev, next
	anim_data->prev = anim_data_list_end;
	anim_data->next = NULL;
	if (anim_data_list_end) anim_data_list_end->next = anim_data;
	anim_data_list_end = anim_data;

	// only first node
	if (anim_data_list_start == NULL) {
		anim_data->prev = NULL;
		anim_data_list_start = anim_data;
	}

	anim_data_index_end += 1;
	return anim_data;
}

int animation_manager_load_file(std::string path, anim_data_t* anim_data, int stat)
{
	bool read_flg[ANIM_TAG_END] = { false };

	std::ifstream inFile(g_base_path + "data/" + path);
	if (inFile.is_open()) {
		std::string line;
		while (std::getline(inFile, line)) {
			if (line == "") continue;
			if (line[0] == '#') continue;

			if (line == "[frame]") { read_flg[ANIM_TAG_FRAME] = true; continue; }
			if (line == "[/frame]") { read_flg[ANIM_TAG_FRAME] = false; continue; }
			if (line == "[img]") { read_flg[ANIM_TAG_IMG] = true;  continue; }
			if (line == "[/img]") { read_flg[ANIM_TAG_IMG] = false; continue; }
			if (line == "[snd]") { read_flg[ANIM_TAG_SND] = true;  continue; }
			if (line == "[/snd]") { read_flg[ANIM_TAG_SND] = false; continue; }

			if (read_flg[ANIM_TAG_FRAME]) {
				load_anim_frame(line, anim_data, stat);
			}
			if (read_flg[ANIM_TAG_IMG]) {
				load_anim_img(line, anim_data, stat);
			}
			if (read_flg[ANIM_TAG_SND]) {
				load_anim_snd(line, anim_data, stat);
			}
		}
		inFile.close();
	}
	else {
		LOG_ERROR("animation_manager_load_file %s error\n", path.c_str());
		return 1;
	}
	return 0;
}

static void load_anim_frame(std::string line, anim_data_t* anim_data, int stat)
{
	std::string key;
	std::string value;
	game_utils_split_key_value(line, key, value);

	if (key == "duration") {
		if (value == "*") {
			anim_data->anim_stat_base_list[stat]->type = ANIM_TYPE_STATIC;
			anim_frame_data_t* anim_frame_data = animation_manager_new_anim_frame();
			anim_frame_data->frame_time = 0;
			anim_frame_data->tex = NULL;
			anim_frame_data->chunk = NULL;
			anim_frame_data->command = ANIM_FRAME_COMMAND_OFF;
			anim_data->anim_stat_base_list[stat]->frame_list[0] = anim_frame_data;
			anim_data->anim_stat_base_list[stat]->total_time = 0;
			anim_data->anim_stat_base_list[stat]->frame_size = 1;
			anim_data->anim_stat_base_list[stat]->snd_channel = SOUND_MANAGER_CH_AUTO;
		}
		else {
			std::vector<int> int_list;
			game_utils_split_conmma(value, int_list);

			anim_data->anim_stat_base_list[stat]->type = ANIM_TYPE_DYNAMIC;
			anim_data->anim_stat_base_list[stat]->total_time = 0;
			for (int i = 0; i < int_list.size(); i++) {
				anim_frame_data_t* anim_frame_data = animation_manager_new_anim_frame();
				anim_frame_data->frame_time = int_list[i];
				anim_frame_data->tex = NULL;
				anim_frame_data->chunk = NULL;
				anim_frame_data->command = ANIM_FRAME_COMMAND_OFF;
				anim_data->anim_stat_base_list[stat]->frame_list[i] = anim_frame_data;
				anim_data->anim_stat_base_list[stat]->total_time += int_list[i];
			}
			anim_data->anim_stat_base_list[stat]->frame_size = (int)int_list.size();
			anim_data->anim_stat_base_list[stat]->snd_channel = SOUND_MANAGER_CH_AUTO;
		}
	}

	if (key == "command") {
		std::vector<int> int_list;
		game_utils_split_conmma(value, int_list);
		for (int i = 0; i < int_list.size(); i++) {
			anim_data->anim_stat_base_list[stat]->frame_list[int_list[i]]->command = ANIM_FRAME_COMMAND_ON;
		}
	}
}

static void load_anim_img(std::string line, anim_data_t* anim_data, int stat)
{
	std::string key;
	std::string value;
	game_utils_split_key_value(line, key, value);

	if (key == "layer") {
		anim_data->anim_stat_base_list[stat]->tex_layer = atoi(value.c_str());
	}
	if (key == "dir_path") {
		dir_path = value;
	}
	if (key == "path") {
		std::string expand_str;
		std::vector<std::string> str_list;
		game_utils_expand_value(value, expand_str);
		game_utils_split_conmma(expand_str, str_list);

		std::string image_filename = dir_path + "/";
		for (int i = 0; i < str_list.size(); i++) {
			anim_frame_data_t* anim_frame_data = anim_data->anim_stat_base_list[stat]->frame_list[i];
			anim_frame_data->tex = resource_manager_getTextureFromPath(image_filename + str_list[i]);
		}
	}
	if (key == "effect") {
		std::vector<std::string> str_list;
		game_utils_split_conmma(value, str_list);

		if ((str_list.size() == 1) && (str_list[0] == "*")) {
			int w, h;
			for (int fi = 0; fi < anim_data->anim_stat_base_list[stat]->frame_size; fi++) {
				int ret = SDL_QueryTexture(anim_data->anim_stat_base_list[stat]->frame_list[fi]->tex, NULL, NULL, &w, &h);
				if (ret == 0) {
					anim_frame_data_t* anim_frame_data = anim_data->anim_stat_base_list[stat]->frame_list[fi];
					anim_frame_data->src_rect = { 0, 0, w, h };
				}
			}
		}
		else {
			for (int fi = 0; fi < str_list.size(); fi++) {
				if (str_list[fi].substr(0, 10) == "tile_clip(") {
					int sep_index[4] = { 0 };
					int sep_i = 0;
					for (int i = 10; i < str_list[fi].size(); i++) {
						if (str_list[fi][i] == ':') {
							sep_index[sep_i] = i;
							sep_i++;
							continue;
						}
						if (str_list[fi][i] == ')') {
							sep_index[sep_i] = i;
							sep_i++;
							break;
						}
					}
					if (sep_i != 4) {
						LOG_ERROR("load_tile_img effect title_clip error\n");
						return; // format error
					}

					int x = atoi(str_list[fi].substr(10, sep_index[0] - 10).c_str());
					int y = atoi(str_list[fi].substr(sep_index[0] + 1, sep_index[1] - sep_index[0]).c_str());
					int w = atoi(str_list[fi].substr(sep_index[1] + 1, sep_index[2] - sep_index[1]).c_str());
					int h = atoi(str_list[fi].substr(sep_index[2] + 1, sep_index[3] - sep_index[2]).c_str());

					anim_frame_data_t* anim_frame_data = anim_data->anim_stat_base_list[stat]->frame_list[fi];
					anim_frame_data->src_rect = { x, y, w, h };
				}
			}
		}

	}
}

static void load_anim_snd(std::string line, anim_data_t* anim_data, int stat)
{
	std::string key;
	std::string value;
	game_utils_split_key_value(line, key, value);

	if (key == "channel") {
		anim_data->anim_stat_base_list[stat]->snd_channel = atoi(value.c_str());
	}
	if (key == "dir_path") {
		dir_path = value;
	}
	if (key == "path") {
		std::string expand_str;
		std::vector<std::string> str_list;
		game_utils_expand_value(value, expand_str);
		game_utils_split_conmma(expand_str, str_list);

		std::string snd_filename = dir_path + "/";
		for (int i = 0; i < str_list.size(); i++) {
			if (str_list[i] == "*") {
				anim_data->anim_stat_base_list[stat]->frame_list[i]->chunk = NULL;
			}
			else {
				anim_frame_data_t* anim_frame_data = anim_data->anim_stat_base_list[stat]->frame_list[i];
				anim_frame_data->chunk = resource_manager_getChunkFromPath(snd_filename + str_list[i]);
			}
		}

		// set NULL
		if (anim_data->anim_stat_base_list[stat]->frame_size > str_list.size()) {
			if (str_list[str_list.size() - 1] == "*") {
				for (int i = (int)str_list.size(); i < anim_data->anim_stat_base_list[stat]->frame_size; i++) {
					anim_data->anim_stat_base_list[stat]->frame_list[i]->chunk = NULL;
				}
			}
		}
	}

	if (key == "volume") {
		std::vector<std::string> str_list;
		game_utils_split_conmma(value, str_list);
		if ((str_list.size() == 1) && (str_list[0] == "*")) {
			//
			// set relative volume
			//
		}
	}
}
