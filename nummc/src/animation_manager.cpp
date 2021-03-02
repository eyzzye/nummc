#include "game_common.h"
#include "animation_manager.h"

#include "game_log.h"
#include "game_utils.h"
#include "resource_manager.h"
#include "gui_common.h"
#include "sound_manager.h"
#include "map_manager.h"

#define ANIM_TAG_FRAME      0
#define ANIM_TAG_IMG        1
#define ANIM_TAG_SND        2
#define ANIM_TAG_END        3

static anim_data_t anim_data_list[ANIM_DATA_LIST_SIZE];
static anim_data_t* anim_data_list_start;
static anim_data_t* anim_data_list_end;
int anim_data_index_end;

#define ANIM_STAT_BASE_LIST_SIZE_OF_UNIT_BASE ((((UNIT_PLAYER_BASE_LIST_SIZE + UNIT_ENEMY_BASE_LIST_SIZE + UNIT_ITEMS_BASE_LIST_SIZE \
        + UNIT_EFFECT_BASE_LIST_SIZE + UNIT_TRAP_BASE_LIST_SIZE + UNIT_PLAYER_BULLET_BASE_LIST_SIZE + UNIT_ENEMY_BULLET_BASE_LIST_SIZE \
        + TILE_TEX_NUM \
        ) + 64) / 64) * 64)

#define ANIM_STAT_BASE_LIST_SIZE (ANIM_STAT_BASE_LIST_SIZE_OF_UNIT_BASE * ANIM_STAT_END)
static anim_stat_base_data_t anim_stat_base_data_list[ANIM_STAT_BASE_LIST_SIZE];
int anim_stat_base_data_index_end;

#define ANIM_STAT_LIST_SIZE (ANIM_DATA_LIST_SIZE * ANIM_STAT_END)
static anim_stat_data_t anim_stat_data_list[ANIM_STAT_LIST_SIZE];
int anim_stat_data_index_end;

#define ANIM_FRAME_DATA_BUFFER_SIZE  (ANIM_STAT_BASE_LIST_SIZE_OF_UNIT_BASE * ANIM_FRAME_NUM_MAX)
static anim_frame_data_t anim_frame_data_buffer[ANIM_FRAME_DATA_BUFFER_SIZE];
int anim_frame_data_index_end;

static void load_anim_frame(char* line, anim_data_t* anim_data, int stat);
static void load_anim_img(char* line, anim_data_t* anim_data, int stat);
static void load_anim_snd(char* line, anim_data_t* anim_data, int stat);

static char dir_path[GAME_FULL_PATH_MAX];
static int dir_path_size;
static char tmp_str_list[GAME_UTILS_STRING_CHAR_BUF_SIZE * ANIM_FRAME_NUM_MAX];
static int tmp_str_list_size;

int animation_manager_init()
{
	memset(anim_data_list, 0, sizeof(anim_data_list));
	memset(anim_stat_base_data_list, 0, sizeof(anim_stat_base_data_list));
	memset(anim_stat_data_list, 0, sizeof(anim_stat_data_list));
	memset(anim_frame_data_buffer, 0, sizeof(anim_frame_data_buffer));

	anim_data_index_end = 0;
	anim_data_list_start = NULL;
	anim_data_list_end = NULL;
	anim_stat_base_data_index_end = 0;
	anim_stat_data_index_end = 0;
	anim_frame_data_index_end = 0;

	return 0;
}

void animation_manager_unload()
{
	for (int i = 0; i < ANIM_STAT_BASE_LIST_SIZE; i++) {
		if (anim_stat_base_data_list[i].obj) {
			game_utils_string_delete((char*)anim_stat_base_data_list[i].obj);
			anim_stat_base_data_list[i].obj = NULL;
		}

		for (int fi = 0; fi < anim_stat_base_data_list[i].frame_size; fi++) {
			if (anim_stat_base_data_list[i].frame_list[fi]) {
				memset(anim_stat_base_data_list[i].frame_list[fi], 0, sizeof(anim_frame_data_t));
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
	if (anim_frame_data_index_end >= ANIM_FRAME_DATA_BUFFER_SIZE) {
		LOG_ERROR("Error: animation_manager_new_anim_frame() buffer overflow\n");
		return NULL;
	}

	anim_frame_data_t* anim_frame_data = &anim_frame_data_buffer[anim_frame_data_index_end];
	if (anim_frame_data->type != ANIM_FRAME_TYPE_NONE) {
		LOG_ERROR("Error: animation_manager_new_anim_frame() occurred unexpected error\n");
		return NULL;
	}

	anim_frame_data->type = ANIM_FRAME_TYPE_FRAME;
	anim_frame_data_index_end++;
	return anim_frame_data;
}

void animation_manager_new_anim_stat_base_data(anim_data_t* anim_data)
{
	if (anim_stat_base_data_index_end >= ANIM_STAT_BASE_LIST_SIZE) {
		LOG_ERROR("Error: animation_manager_new_anim_stat_base_data() buffer overflow\n");
		return;
	}

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

typedef struct _load_file_callback_data_t load_file_callback_data_t;
struct _load_file_callback_data_t {
	bool read_flg[ANIM_TAG_END];
	anim_data_t* anim_data;
	int stat;
};
static load_file_callback_data_t load_file_callback_data;
static void load_file_callback(char* line, int line_size, int line_num, void* argv)
{
	load_file_callback_data_t* data = (load_file_callback_data_t*)argv;

	if (line[0] == '\0') return;
	if (line[0] == '#') return;

	if (line[0] == '[') {
		if (STRCMP_EQ(line, "[frame]")) { data->read_flg[ANIM_TAG_FRAME] = true; return; }
		if (STRCMP_EQ(line, "[/frame]")) { data->read_flg[ANIM_TAG_FRAME] = false; return; }
		if (STRCMP_EQ(line, "[img]")) { data->read_flg[ANIM_TAG_IMG] = true;  return; }
		if (STRCMP_EQ(line, "[/img]")) { data->read_flg[ANIM_TAG_IMG] = false; return; }
		if (STRCMP_EQ(line, "[snd]")) { data->read_flg[ANIM_TAG_SND] = true;  return; }
		if (STRCMP_EQ(line, "[/snd]")) { data->read_flg[ANIM_TAG_SND] = false; return; }
	}

	if (data->read_flg[ANIM_TAG_FRAME]) {
		load_anim_frame(line, data->anim_data, data->stat);
	}
	if (data->read_flg[ANIM_TAG_IMG]) {
		load_anim_img(line, data->anim_data, data->stat);
	}
	if (data->read_flg[ANIM_TAG_SND]) {
		load_anim_snd(line, data->anim_data, data->stat);
	}
}

int animation_manager_load_file(char* path, anim_data_t* anim_data, int stat)
{
	// full_path = g_base_path + "data/" + path;
	char full_path[GAME_FULL_PATH_MAX];
	int tmp_path_size = game_utils_string_cat(full_path, g_base_path, (char*)"data/", path);
	if (tmp_path_size == 0) {
		LOG_ERROR("animation_manager_load_file failed get %s\n", path);
		return 1;
	}

	// read file
	memset(load_file_callback_data.read_flg, 0, sizeof(bool)* ANIM_TAG_END);
	load_file_callback_data.anim_data = anim_data;
	load_file_callback_data.stat = stat;
	int ret = game_utils_files_read_line(full_path, load_file_callback, (void*)&load_file_callback_data);
	if (ret != 0) {
		LOG_ERROR("animation_manager_load_file %s error\n", path);
		return 1;
	}

	return 0;
}

static void load_anim_frame(char* line, anim_data_t* anim_data, int stat)
{
	char key[GAME_UTILS_STRING_NAME_BUF_SIZE];
	char value[GAME_UTILS_STRING_CHAR_BUF_SIZE];
	game_utils_split_key_value(line, key, value);

	if (STRCMP_EQ(key,"duration")) {
		if (STRCMP_EQ(value,"*")) {
			anim_data->anim_stat_base_list[stat]->type = ANIM_TYPE_STATIC;
			anim_frame_data_t* anim_frame_data = animation_manager_new_anim_frame();
			anim_frame_data->frame_time = 0;
			anim_frame_data->res_img = NULL;
			anim_frame_data->res_chunk = NULL;
			anim_frame_data->command = ANIM_FRAME_COMMAND_OFF;
			anim_data->anim_stat_base_list[stat]->frame_list[0] = anim_frame_data;
			anim_data->anim_stat_base_list[stat]->total_time = 0;
			anim_data->anim_stat_base_list[stat]->frame_size = 1;
			anim_data->anim_stat_base_list[stat]->snd_channel = SOUND_MANAGER_CH_AUTO;
		}
		else {
			int int_list[ANIM_FRAME_NUM_MAX];
			int int_list_size = game_utils_split_conmma_int(value, int_list, ANIM_FRAME_NUM_MAX);

			anim_data->anim_stat_base_list[stat]->type = ANIM_TYPE_DYNAMIC;
			anim_data->anim_stat_base_list[stat]->total_time = 0;
			for (int i = 0; i < int_list_size; i++) {
				anim_frame_data_t* anim_frame_data = animation_manager_new_anim_frame();
				anim_frame_data->frame_time = int_list[i];
				anim_frame_data->res_img = NULL;
				anim_frame_data->res_chunk = NULL;
				anim_frame_data->command = ANIM_FRAME_COMMAND_OFF;
				anim_data->anim_stat_base_list[stat]->frame_list[i] = anim_frame_data;
				anim_data->anim_stat_base_list[stat]->total_time += int_list[i];
			}
			anim_data->anim_stat_base_list[stat]->frame_size = int_list_size;
			anim_data->anim_stat_base_list[stat]->snd_channel = SOUND_MANAGER_CH_AUTO;
		}
		return;
	}

	if (STRCMP_EQ(key,"type")) {
		if (STRCMP_EQ(value,"STATIC")) {
			anim_data->anim_stat_base_list[stat]->type = ANIM_TYPE_STATIC;
		}
		else if (STRCMP_EQ(value,"DYNAMIC")) {
			anim_data->anim_stat_base_list[stat]->type = ANIM_TYPE_DYNAMIC;
		}
		else if (STRCMP_EQ(value,"DRAW_RECT")) {
			anim_data->anim_stat_base_list[stat]->type = ANIM_TYPE_DRAW_RECT;
		}
		else if (STRCMP_EQ(value,"DRAW_CIRCLE")) {
			anim_data->anim_stat_base_list[stat]->type = ANIM_TYPE_DRAW_CIRCLE;
		}
		else if (STRCMP_EQ(value,"DRAW_RECT_FILL")) {
			anim_data->anim_stat_base_list[stat]->type = ANIM_TYPE_DRAW_RECT_FILL;
		}
		else {
			anim_data->anim_stat_base_list[stat]->type = ANIM_TYPE_NONE;
		}
		return;
	}

	if (STRCMP_EQ(key,"layer")) {
		anim_data->anim_stat_base_list[stat]->tex_layer = atoi(value);
		return;
	}

	if (STRCMP_EQ(key,"command")) {
		int int_list[ANIM_FRAME_NUM_MAX];
		int int_list_size = game_utils_split_conmma_int(value, int_list, ANIM_FRAME_NUM_MAX);
		for (int i = 0; i < int_list_size; i++) {
			anim_data->anim_stat_base_list[stat]->frame_list[int_list[i]]->command = ANIM_FRAME_COMMAND_ON;
		}
		return;
	}

	// draw color
	if (STRCMP_EQ(key,"color_r")) {
		anim_data->anim_stat_base_list[stat]->color_r = atoi(value);
		return;
	}
	if (STRCMP_EQ(key,"color_g")) {
		anim_data->anim_stat_base_list[stat]->color_g = atoi(value);
		return;
	}
	if (STRCMP_EQ(key,"color_b")) {
		anim_data->anim_stat_base_list[stat]->color_b = atoi(value);
		return;
	}
	if (STRCMP_EQ(key,"color_a")) {
		anim_data->anim_stat_base_list[stat]->color_a = atoi(value);
		return;
	}
}

static void load_anim_img(char* line, anim_data_t* anim_data, int stat)
{
	char key[GAME_UTILS_STRING_NAME_BUF_SIZE];
	char value[GAME_UTILS_STRING_LINE_BUF_SIZE];
	char expand_str[GAME_UTILS_STRING_LINE_BUF_SIZE];
	char image_filename[GAME_FULL_PATH_MAX];
	char str_list[GAME_UTILS_STRING_CHAR_BUF_SIZE * ANIM_FRAME_NUM_MAX];
	game_utils_split_key_value(line, key, value);

	if (STRCMP_EQ(key,"layer")) {
		anim_data->anim_stat_base_list[stat]->tex_layer = atoi(value);
		return;
	}
	if (STRCMP_EQ(key,"dir_path")) {
		//dir_path = value;
		if (game_utils_string_copy(dir_path, value) != 0) {
			dir_path_size = 0;
			LOG_ERROR("Error: load_anim_img() failed copy dir_path %s\n", value);
		}
		dir_path_size = (int)strlen(dir_path);
		return;
	}
	if (STRCMP_EQ(key,"path")) {
		int expand_str_size = game_utils_expand_value(value, expand_str);

		tmp_str_list_size = 0;
		if (expand_str_size > 0) {
			tmp_str_list_size = game_utils_split_conmma(expand_str, tmp_str_list, ANIM_FRAME_NUM_MAX);
		}
		else {
			tmp_str_list_size = game_utils_split_conmma(value, tmp_str_list, ANIM_FRAME_NUM_MAX);
		}

		for (int i = 0; i < tmp_str_list_size; i++) {
			anim_frame_data_t* anim_frame_data = anim_data->anim_stat_base_list[stat]->frame_list[i];
			game_utils_string_cat(image_filename, dir_path, &tmp_str_list[GAME_UTILS_STRING_CHAR_BUF_SIZE * i]);
			anim_frame_data->res_img = resource_manager_getTextureFromPath(image_filename);

			int w, h;
			int ret = GUI_QueryTexture(anim_frame_data->res_img, NULL, NULL, &w, &h);
			anim_frame_data->src_rect = { 0, 0, w, h };
		}
		return;
	}
	if (STRCMP_EQ(key,"effect")) {
		int str_list_size = game_utils_split_conmma(value, str_list, ANIM_FRAME_NUM_MAX);

		if ((str_list_size == 1) && (str_list[0] == '*')) {
			// do nothing
		}
		else {
			char keyword_str[GAME_UTILS_STRING_CHAR_BUF_SIZE];

			for (int fi = 0; fi < str_list_size; fi++) {
				char* frame_effect_str = &str_list[GAME_UTILS_STRING_CHAR_BUF_SIZE * fi];
				int element_size = (int)strlen(&str_list[GAME_UTILS_STRING_CHAR_BUF_SIZE * fi]);

				int keyword_size = 10;
				game_utils_string_copy_n(keyword_str, frame_effect_str, keyword_size);
				if (STRCMP_EQ(keyword_str,"tile_clip(")) {
					int sep_index[4] = { 0 };
					int sep_i = 0;
					for (int i = keyword_size; i < element_size; i++) {
						if (frame_effect_str[i] == ':') {
							sep_index[sep_i] = i;
							sep_i++;
							continue;
						}
						if (frame_effect_str[i] == ')') {
							sep_index[sep_i] = i;
							sep_i++;
							break;
						}
					}
					if (sep_i != 4) {
						LOG_ERROR("load_tile_img effect title_clip error\n");
						return; // format error
					}

					//int x = atoi(str_list[fi].substr(10, sep_index[0] - 10).c_str());
					game_utils_string_copy_n(keyword_str, &frame_effect_str[keyword_size], sep_index[0] - keyword_size);
					int x = atoi(keyword_str);

					//int y = atoi(str_list[fi].substr(sep_index[0] + 1, sep_index[1] - sep_index[0]).c_str());
					game_utils_string_copy_n(keyword_str, &frame_effect_str[sep_index[0] + 1], sep_index[1] - (sep_index[0] + 1));
					int y = atoi(keyword_str);
	
					//int w = atoi(str_list[fi].substr(sep_index[1] + 1, sep_index[2] - sep_index[1]).c_str());
					game_utils_string_copy_n(keyword_str, &frame_effect_str[sep_index[1] + 1], sep_index[2] - (sep_index[1] + 1));
					int w = atoi(keyword_str);

					//int h = atoi(str_list[fi].substr(sep_index[2] + 1, sep_index[3] - sep_index[2]).c_str());
					game_utils_string_copy_n(keyword_str, &frame_effect_str[sep_index[2] + 1], sep_index[3] - (sep_index[2] + 1));
					int h = atoi(keyword_str);

					anim_frame_data_t* anim_frame_data = anim_data->anim_stat_base_list[stat]->frame_list[fi];
					anim_frame_data->src_rect = { x, y, w, h };
					continue;
				}

				keyword_size = 6;
				game_utils_string_copy_n(keyword_str, frame_effect_str, keyword_size);
				if (STRCMP_EQ(keyword_str,"color:")) {
					game_utils_string_cat(keyword_str, (char*)"{", frame_effect_str, (char*)"}");
					for (int i = 0; i < tmp_str_list_size; i++) {
						anim_frame_data_t* anim_frame_data = anim_data->anim_stat_base_list[stat]->frame_list[i];
						//image_filename = {color:S:255:255:255:D:255:255:255} + dir_path + filename
						game_utils_string_cat(image_filename, keyword_str, dir_path, &tmp_str_list[i * GAME_UTILS_STRING_CHAR_BUF_SIZE]);
						anim_frame_data->res_img = resource_manager_getTextureFromPath(image_filename);
					}
				}
			}
		}

		return;
	}
}

static void load_anim_snd(char* line, anim_data_t* anim_data, int stat)
{
	char key[GAME_UTILS_STRING_NAME_BUF_SIZE];
	char value[GAME_UTILS_STRING_LINE_BUF_SIZE];
	char expand_str[GAME_UTILS_STRING_LINE_BUF_SIZE];
	char snd_filename[GAME_FULL_PATH_MAX];
	char str_list[GAME_UTILS_STRING_NAME_BUF_SIZE * ANIM_FRAME_NUM_MAX];
	game_utils_split_key_value(line, key, value);

	if (STRCMP_EQ(key,"channel")) {
		anim_data->anim_stat_base_list[stat]->snd_channel = atoi(value);
		return;
	}
	if (STRCMP_EQ(key,"dir_path")) {
		//dir_path = value;
		if (game_utils_string_copy(dir_path, value) != 0) {
			dir_path_size = 0;
			LOG_ERROR("Error: load_anim_snd() failed copy dir_path %s\n", value);
		}
		dir_path_size = (int)strlen(dir_path);
		return;
	}
	if (STRCMP_EQ(key,"path")) {
		int expand_str_size = game_utils_expand_value(value, expand_str);

		int str_list_size = 0;
		if (expand_str_size > 0) {
			str_list_size = game_utils_split_conmma(expand_str, str_list, ANIM_FRAME_NUM_MAX, GAME_UTILS_STRING_NAME_BUF_SIZE);
		}
		else {
			str_list_size = game_utils_split_conmma(value, str_list, ANIM_FRAME_NUM_MAX, GAME_UTILS_STRING_NAME_BUF_SIZE);
		}

		//std::string snd_filename = dir_path;
		for (int i = 0; i < str_list_size; i++) {
			if (str_list[GAME_UTILS_STRING_NAME_BUF_SIZE * i] == '*') {
				anim_data->anim_stat_base_list[stat]->frame_list[i]->res_chunk = NULL;
			}
			else {
				anim_frame_data_t* anim_frame_data = anim_data->anim_stat_base_list[stat]->frame_list[i];
				game_utils_string_cat(snd_filename, dir_path, &str_list[GAME_UTILS_STRING_NAME_BUF_SIZE * i]);
				anim_frame_data->res_chunk = resource_manager_getChunkFromPath(snd_filename);
			}
		}

		// set NULL
		if (anim_data->anim_stat_base_list[stat]->frame_size > str_list_size) {
			if (str_list[GAME_UTILS_STRING_NAME_BUF_SIZE * (str_list_size - 1)] == '*') {
				for (int i = str_list_size; i < anim_data->anim_stat_base_list[stat]->frame_size; i++) {
					anim_data->anim_stat_base_list[stat]->frame_list[i]->res_chunk = NULL;
				}
			}
		}
		return;
	}

	if (STRCMP_EQ(key,"volume")) {
		int str_list_size = game_utils_split_conmma(value, str_list, ANIM_FRAME_NUM_MAX, GAME_UTILS_STRING_NAME_BUF_SIZE);
		if ((str_list_size == 1) && (str_list[0] == '*')) {
			//
			// set relative volume
			//
		}
		return;
	}
}

int animation_manager_get_base_size_index(int base_size)
{
	int base_size_index = ANIM_BASE_SIZE_32x32;
	if (base_size == 48) base_size_index = ANIM_BASE_SIZE_48x48;
	else if (base_size == 64) base_size_index = ANIM_BASE_SIZE_64x64;
	//else base_size_index = ANIM_BASE_SIZE_32x32;

	return base_size_index;
}
