#include "game_common.h"
#include "game_save.h"

#ifdef _WIN32
#include <io.h>
#elif _ANDROID
// not use access() function
#else
#include <sys/io.h>
#include <unistd.h>
#endif

#include "memory_manager.h"
#include "game_utils.h"
#include "game_log.h"

#define GAME_SAVE_SLOT_NUM 8

// settings
#define GAME_SAVE_SETTINGS_ID_DEFAULT_SLOT  0
#define GAME_SAVE_SETTINGS_ID_UNLOCK_STAT   1
#define GAME_SAVE_SETTINGS_ID_RESOLUTION_W  2
#define GAME_SAVE_SETTINGS_ID_RESOLUTION_H  3
#define GAME_SAVE_SETTINGS_ID_MUSIC_VOLUME  4
#define GAME_SAVE_SETTINGS_ID_SFX_VOLUME    5
#define GAME_SAVE_SETTINGS_ID_END           6

static const char* settings_key_str[GAME_SAVE_SETTINGS_ID_END] = {
	"default_slot",
	"unlock_stat",
	"resolution_w",
	"resolution_h",
	"music_volume",
	"sfx_volume",
};

// slot
#define GAME_SAVE_SLOT_ID_PLAYER     0
#define GAME_SAVE_SLOT_ID_STAGE      1
#define GAME_SAVE_SLOT_ID_TIMESTAMP  2
#define GAME_SAVE_SLOT_ID_END        3

static const char* slot_key_str[GAME_SAVE_SLOT_ID_END] = {
	"player",
	"stage",
	"timestamp",
};

// player
#define GAME_SAVE_PLAYER_ID_PATH               0
#define GAME_SAVE_PLAYER_ID_HP                 1
#define GAME_SAVE_PLAYER_ID_EXP                2
#define GAME_SAVE_PLAYER_ID_ATTACK_WAIT_TIMER  3
#define GAME_SAVE_PLAYER_ID_BULLET_LIFE_TIMER  4
#define GAME_SAVE_PLAYER_ID_BULLET_SPEC        5
#define GAME_SAVE_PLAYER_ID_SPEED              6
#define GAME_SAVE_PLAYER_ID_WEAPON             7
#define GAME_SAVE_PLAYER_ID_ARMOR              8
#define GAME_SAVE_PLAYER_ID_SPEC               9
#define GAME_SAVE_PLAYER_ID_HP_MAX            10
#define GAME_SAVE_PLAYER_ID_EXP_MAX           11
#define GAME_SAVE_PLAYER_ID_LEVEL             12
#define GAME_SAVE_PLAYER_ID_END               13

static const char* player_key_str[GAME_SAVE_PLAYER_ID_END] = {
	"path",
	"hp",
	"exp",
	"attack_wait_timer",
	"bullet_life_timer",
	"bullet_spec",
	"speed",
	"weapon",
	"armor",
	"spec",
	"hp_max",
	"exp_max",
	"level",
};

// stocker
#define GAME_SAVE_STOCKER_ID_WEAPON_ITEM_LIST_IDX   0
#define GAME_SAVE_STOCKER_ID_WEAPON_ITEM_COUNT      1
#define GAME_SAVE_STOCKER_ID_CHARGE_ITEM_LIST_IDX   2
#define GAME_SAVE_STOCKER_ID_CHARGE_ITEM_COUNT      3
#define GAME_SAVE_STOCKER_ID_CHARGE_CHARGE_VAL      4
#define GAME_SAVE_STOCKER_ID_CHARGE_CHARGE_TIMER    5
#define GAME_SAVE_STOCKER_ID_SPECIAL_ITEM_LIST_IDX  6
#define GAME_SAVE_STOCKER_ID_SPECIAL_ITEM_COUNT     7
#define GAME_SAVE_STOCKER_ID_END                    8

static const char* stocker_key_str[GAME_SAVE_STOCKER_ID_END] = {
	"weapon_item_list_idx",
	"weapon_item_count",
	"charge_item_list_idx",
	"charge_item_count",
	"charge_charge_val",
	"charge_charge_timer",
	"special_item_list_idx",
	"special_item_count",
};

// config data
typedef struct _game_config_data_t game_config_data_t;
struct _game_config_data_t {
	char* settings[GAME_SAVE_SETTINGS_ID_END];
	char* slot[GAME_SAVE_SLOT_NUM][GAME_SAVE_SLOT_ID_END];
	char* player[GAME_SAVE_SLOT_NUM][GAME_SAVE_PLAYER_ID_END];
	char* stocker[GAME_SAVE_SLOT_NUM][GAME_SAVE_STOCKER_ID_END];
};
static game_config_data_t game_config_data;

// tmp region
static char g_save_path[GAME_FULL_PATH_MAX];
static char save_template_path[GAME_FULL_PATH_MAX];

// extern variables
int g_save_folder_size;
char g_save_folder[GAME_FULL_PATH_MAX];

static int game_config_data_alloc()
{
	for (int i = 0; i < GAME_SAVE_SETTINGS_ID_END; i++) {
		game_config_data.settings[i] = memory_manager_new_char_buff(MEMORY_MANAGER_NAME_BUF_SIZE - 1);
		if (game_config_data.settings[i] == NULL) return 1;
	}

	for (int i = 0; i < GAME_SAVE_SLOT_NUM; i++) {
		// slot
		for (int s_i = 0; s_i < GAME_SAVE_SLOT_ID_END; s_i++) {
			game_config_data.slot[i][s_i] = memory_manager_new_char_buff(MEMORY_MANAGER_NAME_BUF_SIZE - 1);
			if (game_config_data.slot[i][s_i] == NULL) return 1;
		}

		// player
		for (int p_i = 0; p_i < GAME_SAVE_PLAYER_ID_END; p_i++) {
			int memory_size = MEMORY_MANAGER_NAME_BUF_SIZE - 1;
			if (p_i == GAME_SAVE_PLAYER_ID_PATH) memory_size = MEMORY_MANAGER_STRING_BUF_SIZE - 1;

			game_config_data.player[i][p_i] = memory_manager_new_char_buff(memory_size);
			if (game_config_data.player[i][p_i] == NULL) return 1;
		}

		// stocker
		for (int st_i = 0; st_i < GAME_SAVE_STOCKER_ID_END; st_i++) {
			game_config_data.stocker[i][st_i] = memory_manager_new_char_buff(MEMORY_MANAGER_NAME_BUF_SIZE - 1);
			if (game_config_data.stocker[i][st_i] == NULL) return 1;
		}
	}

	return 0;
}

static void game_config_data_delete()
{
	for (int i = 0; i < GAME_SAVE_SETTINGS_ID_END; i++) {
		if (game_config_data.settings[i] != NULL) {
			memory_manager_delete_char_buff(game_config_data.settings[i]);
			game_config_data.settings[i] = NULL;
		}
	}

	for (int i = 0; i < GAME_SAVE_SLOT_NUM; i++) {
		// slot
		for (int s_i = 0; s_i < GAME_SAVE_SLOT_ID_END; s_i++) {
			if (game_config_data.slot[i][s_i] != NULL) {
				memory_manager_delete_char_buff(game_config_data.slot[i][s_i]);
				game_config_data.slot[i][s_i] = NULL;
			}
		}

		// player
		for (int p_i = 0; p_i < GAME_SAVE_PLAYER_ID_END; p_i++) {
			if (game_config_data.player[i][p_i] != NULL) {
				memory_manager_delete_char_buff(game_config_data.player[i][p_i]);
				game_config_data.player[i][p_i] = NULL;
			}
		}

		// stocker
		for (int st_i = 0; st_i < GAME_SAVE_STOCKER_ID_END; st_i++) {
			if (game_config_data.stocker[i][st_i] != NULL) {
				memory_manager_delete_char_buff(game_config_data.stocker[i][st_i]);
				game_config_data.stocker[i][st_i] = NULL;
			}
		}
	}
}

static int get_settings_key_id(char* key)
{
	for (int i = 0; i < GAME_SAVE_SETTINGS_ID_END; i++) {
		if (STRCMP_EQ(key, settings_key_str[i])) {
			return i;
		}
	}
	LOG_ERROR_CONSOLE("Error: get_settings_key_id() not found %s", key);
	return -1;
}

static int get_slot_key_id(char* key)
{
	for (int i = 0; i < GAME_SAVE_SLOT_ID_END; i++) {
		if (STRCMP_EQ(key, slot_key_str[i])) {
			return i;
		}
	}
	LOG_ERROR_CONSOLE("Error: get_slot_key_id() not found %s", key);
	return -1;
}

static int get_player_key_id(char* key)
{
	for (int i = 0; i < GAME_SAVE_PLAYER_ID_END; i++) {
		if (STRCMP_EQ(key, player_key_str[i])) {
			return i;
		}
	}
	LOG_ERROR_CONSOLE("Error: get_player_key_id() not found %s", key);
	return -1;
}

static int get_stocker_key_id(char* key)
{
	for (int i = 0; i < GAME_SAVE_STOCKER_ID_END; i++) {
		if (STRCMP_EQ(key, stocker_key_str[i])) {
			return i;
		}
	}
	LOG_ERROR_CONSOLE("Error: get_stocker_key_id() not found %s", key);
	return -1;
}


typedef struct _load_ini_file_callback_data_t load_ini_file_callback_data_t;
struct _load_ini_file_callback_data_t {
	bool read_flg = false;
	bool read_slot_flg[GAME_SAVE_SLOT_NUM];
	bool read_player_flg[GAME_SAVE_SLOT_NUM];
	bool read_stocker_flg[GAME_SAVE_SLOT_NUM];
};
static load_ini_file_callback_data_t load_ini_file_callback_data;
static void load_ini_file_callback(char* line, int line_size, int line_num, void* argv)
{
	int ret = 0;

	load_ini_file_callback_data_t* data = (load_ini_file_callback_data_t*)argv;
	char tag_name[MEMORY_MANAGER_NAME_BUF_SIZE];
	char key[MEMORY_MANAGER_NAME_BUF_SIZE];
	char value[MEMORY_MANAGER_STRING_BUF_SIZE];

	if (line[0] == '\0') { return; }
	if (line[0] == '#') return;

	if (line[0] == '[') {
		if (STRCMP_EQ(line, "[settings]")) {
			// clear other flg
			memset(data->read_slot_flg,    0, sizeof(bool) * GAME_SAVE_SLOT_NUM);
			memset(data->read_player_flg,  0, sizeof(bool) * GAME_SAVE_SLOT_NUM);
			memset(data->read_stocker_flg, 0, sizeof(bool) * GAME_SAVE_SLOT_NUM);

			data->read_flg = true;
			return;
		}

		int ret = game_utils_string_copy(tag_name, (char*)"[slot1]"); // [slot1...8]
		if (ret != 0) { LOG_ERROR_CONSOLE("Error: load_ini_file_callback() [slot1] %d\n", line_num);  return; }
		bool found_slot = false;
		for (int i = 0; i < GAME_SAVE_SLOT_NUM; i++) {
			if (STRCMP_EQ(line,tag_name)) {
				// clear other flg
				data->read_flg = false;
				memset(data->read_slot_flg,    0, sizeof(bool) * GAME_SAVE_SLOT_NUM);
				memset(data->read_player_flg,  0, sizeof(bool) * GAME_SAVE_SLOT_NUM);
				memset(data->read_stocker_flg, 0, sizeof(bool) * GAME_SAVE_SLOT_NUM);

				data->read_slot_flg[i] = true;
				found_slot = true;
				break;
			}
			tag_name[5] += 1;
		}
		if (found_slot) return;

		ret = game_utils_string_copy(tag_name, (char*)"[player1]"); // [player1...8]
		if (ret != 0) { LOG_ERROR_CONSOLE("Error: load_ini_file_callback() [player1] %d\n", line_num);  return; }
		found_slot = false;
		for (int i = 0; i < GAME_SAVE_SLOT_NUM; i++) {
			if (STRCMP_EQ(line,tag_name)) {
				// clear other flg
				data->read_flg = false;
				memset(data->read_slot_flg,    0, sizeof(bool) * GAME_SAVE_SLOT_NUM);
				memset(data->read_player_flg,  0, sizeof(bool) * GAME_SAVE_SLOT_NUM);
				memset(data->read_stocker_flg, 0, sizeof(bool) * GAME_SAVE_SLOT_NUM);

				data->read_player_flg[i] = true;
				found_slot = true;
				break;
			}
			tag_name[7] += 1;
		}
		if (found_slot) return;

		ret = game_utils_string_copy(tag_name, (char*)"[stocker1]"); // [stocker1...8]
		if (ret != 0) { LOG_ERROR_CONSOLE("Error: load_ini_file_callback() [stocker1] %d\n", line_num);  return; }
		found_slot = false;
		for (int i = 0; i < GAME_SAVE_SLOT_NUM; i++) {
			if (STRCMP_EQ(line,tag_name)) {
				// clear other flg
				data->read_flg = false;
				memset(data->read_slot_flg,    0, sizeof(bool) * GAME_SAVE_SLOT_NUM);
				memset(data->read_player_flg,  0, sizeof(bool) * GAME_SAVE_SLOT_NUM);
				memset(data->read_stocker_flg, 0, sizeof(bool) * GAME_SAVE_SLOT_NUM);

				data->read_stocker_flg[i] = true;
				found_slot = true;
				break;
			}
			tag_name[8] += 1;
		}
		if (found_slot) return;
	}

	if (data->read_flg) {
		if (game_utils_split_key_value(line, key, value) == 0) {
			int id = get_settings_key_id(key);
			ret = game_utils_string_copy(game_config_data.settings[id], value);
			if (ret != 0) { LOG_ERROR_CONSOLE("Error: load_ini_file_callback() copy settings %d", id); }
		}
	}
	for (int i = 0; i < GAME_SAVE_SLOT_NUM; i++) {
		if (data->read_slot_flg[i]) {
			if (game_utils_split_key_value(line, key, value) == 0) {
				int id = get_slot_key_id(key);
				ret = game_utils_string_copy(game_config_data.slot[i][id], value);
				if (ret != 0) { LOG_ERROR_CONSOLE("Error: load_ini_file_callback() copy slot %d %d", (i+1), id); }
			}
			break;
		}
	}
	for (int i = 0; i < GAME_SAVE_SLOT_NUM; i++) {
		if (data->read_player_flg[i]) {
			if (game_utils_split_key_value(line, key, value) == 0) {
				int id = get_player_key_id(key);
				ret = game_utils_string_copy(game_config_data.player[i][id], value);
				if (ret != 0) { LOG_ERROR_CONSOLE("Error: load_ini_file_callback() copy player %d %d", (i + 1), id); }
			}
			break;
		}
	}
	for (int i = 0; i < GAME_SAVE_SLOT_NUM; i++) {
		if (data->read_stocker_flg[i]) {
			if (game_utils_split_key_value(line, key, value) == 0) {
				int id = get_stocker_key_id(key);
				ret = game_utils_string_copy(game_config_data.stocker[i][id], value);
				if (ret != 0) { LOG_ERROR_CONSOLE("Error: load_ini_file_callback() copy stocker %d %d", (i + 1), id); }
			}
			break;
		}
	}
}

static int game_save_load_ini_file(char* path)
{
	if (game_config_data_alloc()) {
		LOG_ERROR("game_config_data_alloc error\n");
		return 1;
	}

	// read file
	load_ini_file_callback_data.read_flg = false;
	memset(load_ini_file_callback_data.read_slot_flg,    0, sizeof(bool)* GAME_SAVE_SLOT_NUM);
	memset(load_ini_file_callback_data.read_player_flg,  0, sizeof(bool)* GAME_SAVE_SLOT_NUM);
	memset(load_ini_file_callback_data.read_stocker_flg, 0, sizeof(bool)* GAME_SAVE_SLOT_NUM);
	int ret = game_utils_files_read_line(path, load_ini_file_callback, (void*)&load_ini_file_callback_data);
	if (ret != 0) { LOG_ERROR("game_save_load_ini_file %s error\n", path); return 1; }

	return 0;
}

static int game_save_save_ini_file(char* path) {
	SDL_RWops* out_file = SDL_RWFromFile(path, "w");
	if (out_file == NULL) {
		LOG_ERROR("Error: game_save_save_ini_file() SDL_RWFromFile() failed\n");
		return 1;
	}

	int ret = 0;
	size_t ret_size = 0;
	char tmp_char_buf[MEMORY_MANAGER_LINE_BUF_SIZE];
	char* new_line_str = (char*)"\n";
	int new_line_str_size = (int)strlen(new_line_str);

	{
		const char* line = "[settings]\n";
		ret_size = SDL_RWwrite(out_file, line, strlen(line), 1);
		if (ret_size <= 0) { LOG_ERROR_CONSOLE("Error: game_save_save_ini_file() write %s\n", line); }

		for (int i = 0; i < GAME_SAVE_SETTINGS_ID_END; i++) {
			ret = game_utils_string_cat(tmp_char_buf, (char*)settings_key_str[i], (char*)"=", game_config_data.settings[i]);
			if (ret <= 0) { LOG_ERROR_CONSOLE("Error: game_save_save_ini_file() get settings %d\n", i); continue; }

			ret_size = SDL_RWwrite(out_file, tmp_char_buf, strlen(tmp_char_buf), 1);
			if (ret_size <= 0) { LOG_ERROR_CONSOLE("Error: game_save_save_ini_file() write %s\n", tmp_char_buf); }

			ret_size = SDL_RWwrite(out_file, new_line_str, new_line_str_size, 1);
			if (ret_size <= 0) { LOG_ERROR_CONSOLE("Error: game_save_save_ini_file() write new_line_str\n"); }
		}
	}

	for (int i = 0; i < GAME_SAVE_SLOT_NUM; i++) {
		char slot_num_str[4] = { '\0' };
		game_utils_string_itoa((i+1), slot_num_str, (4-1), 10);
		ret = game_utils_string_cat(tmp_char_buf, (char*)"[slot", slot_num_str, (char*)"]\n");
		if (ret <= 0) { LOG_ERROR_CONSOLE("Error: game_save_save_ini_file() get [slot%d]", i); continue; }

		ret_size = SDL_RWwrite(out_file, tmp_char_buf, strlen(tmp_char_buf), 1);
		if (ret_size <= 0) { LOG_ERROR_CONSOLE("Error: game_save_save_ini_file() write %s\n", tmp_char_buf); }

		for (int s_i = 0; s_i < GAME_SAVE_SLOT_ID_END; s_i++) {
			ret = game_utils_string_cat(tmp_char_buf, (char*)slot_key_str[s_i], (char*)"=", game_config_data.slot[i][s_i]);
			if (ret <= 0) { LOG_ERROR_CONSOLE("Error: game_save_save_ini_file() get slot %d %d", i, s_i); continue; }

			ret_size = SDL_RWwrite(out_file, tmp_char_buf, strlen(tmp_char_buf), 1);
			if (ret_size <= 0) { LOG_ERROR_CONSOLE("Error: game_save_save_ini_file() write %s\n", tmp_char_buf); }

			ret_size = SDL_RWwrite(out_file, new_line_str, new_line_str_size, 1);
			if (ret_size <= 0) { LOG_ERROR_CONSOLE("Error: game_save_save_ini_file() write new_line_str\n"); }
		}
	}

	for (int i = 0; i < GAME_SAVE_SLOT_NUM; i++) {
		char player_num_str[4] = { '\0' };
		game_utils_string_itoa((i + 1), player_num_str, (4 - 1), 10);
		ret = game_utils_string_cat(tmp_char_buf, (char*)"[player", player_num_str, (char*)"]\n");
		if (ret <= 0) { LOG_ERROR_CONSOLE("Error: game_save_save_ini_file() get [player%d]", i); continue; }

		ret_size = SDL_RWwrite(out_file, tmp_char_buf, strlen(tmp_char_buf), 1);
		if (ret_size <= 0) { LOG_ERROR_CONSOLE("Error: game_save_save_ini_file() write %s\n", tmp_char_buf); }

		for (int p_i = 0; p_i < GAME_SAVE_PLAYER_ID_END; p_i++) {
			ret = game_utils_string_cat(tmp_char_buf, (char*)player_key_str[p_i], (char*)"=", game_config_data.player[i][p_i]);
			if (ret <= 0) { LOG_ERROR_CONSOLE("Error: game_save_save_ini_file() get player %d %d", i, p_i); continue; }

			ret_size = SDL_RWwrite(out_file, tmp_char_buf, strlen(tmp_char_buf), 1);
			if (ret_size <= 0) { LOG_ERROR_CONSOLE("Error: game_save_save_ini_file() write %s\n", tmp_char_buf); }

			ret_size = SDL_RWwrite(out_file, new_line_str, new_line_str_size, 1);
			if (ret_size <= 0) { LOG_ERROR_CONSOLE("Error: game_save_save_ini_file() write new_line_str\n"); }
		}
	}

	for (int i = 0; i < GAME_SAVE_SLOT_NUM; i++) {
		char stocker_num_str[4] = { '\0' };
		game_utils_string_itoa((i + 1), stocker_num_str, (4 - 1), 10);
		ret = game_utils_string_cat(tmp_char_buf, (char*)"[stocker", stocker_num_str, (char*)"]\n");
		if (ret <= 0) { LOG_ERROR_CONSOLE("Error: game_save_save_ini_file() get [stocker%d]", i); continue; }

		ret_size = SDL_RWwrite(out_file, tmp_char_buf, strlen(tmp_char_buf), 1);
		if (ret_size <= 0) { LOG_ERROR_CONSOLE("Error: game_save_save_ini_file() write %s\n", tmp_char_buf); }

		for (int st_i = 0; st_i < GAME_SAVE_STOCKER_ID_END; st_i++) {
			ret = game_utils_string_cat(tmp_char_buf, (char*)stocker_key_str[st_i], (char*)"=", game_config_data.stocker[i][st_i]);
			if (ret <= 0) { LOG_ERROR_CONSOLE("Error: game_save_save_ini_file() get stocker %d %d", i, st_i); continue; }

			ret_size = SDL_RWwrite(out_file, tmp_char_buf, strlen(tmp_char_buf), 1);
			if (ret_size <= 0) { LOG_ERROR_CONSOLE("Error: game_save_save_ini_file() write %s\n", tmp_char_buf); }

			ret_size = SDL_RWwrite(out_file, new_line_str, new_line_str_size, 1);
			if (ret_size <= 0) { LOG_ERROR_CONSOLE("Error: game_save_save_ini_file() write new_line_str\n"); }
		}
	}

	ret = SDL_RWclose(out_file);
	if (ret != 0) {
		LOG_ERROR("Error: game_save_save_ini_file() SDL_RWclose() failed\n");
		return 1;
	}

	return 0;
}

int game_save_init()
{
	int ret = 0;
	char* tmp_path = NULL;
	size_t tmp_path_len;

#ifdef _WIN32
	errno_t std_error = _dupenv_s(&tmp_path, &tmp_path_len, "USERPROFILE");
	if ((!std_error) && (tmp_path_len > 0)) {
		// g_save_folder = tmp_path + "/Documents/My Games/nummc/"
		if (game_utils_string_copy(g_save_folder, tmp_path) != 0) return 1;
		g_save_folder_size = (int)strlen(g_save_folder);
		if (game_utils_string_copy(&g_save_folder[g_save_folder_size], "/Documents/My Games/nummc/") != 0) return 1;
		game_utils_replace_string(g_save_folder, '\\', '/');
		g_save_folder_size = (int)strlen(g_save_folder);

		//g_save_path = g_save_folder + "save_data.ini";
		if (game_utils_string_copy(g_save_path, g_save_folder) != 0) return 1;
		if (game_utils_string_copy(&g_save_path[g_save_folder_size], "save_data.ini") != 0) return 1;
		game_utils_replace_string(g_save_path, '\\', '/');
		//LOG_DEBUG("SavePath: %s\n", g_save_path.c_str());
		free(tmp_path);
		tmp_path = NULL;

		// check folder exist & create
		if (game_utils_create_folder(g_save_folder)) {
			return 1;
		}

		// check file exist & load
		if (_access(g_save_path, 6) != -1) {
			if (game_save_load_ini_file(g_save_path) != 0) return 1;

			// save backup
			char backup_save_path[GAME_FULL_PATH_MAX];
			ret = game_utils_string_cat(backup_save_path, g_save_path, (char*)".backup");
			if (ret <= 0) { LOG_ERROR_CONSOLE("Error: game_save_init() get backup_save_path\n"); return 1; }
			if (game_save_save_ini_file(backup_save_path) != 0) return 1;
		}
		else {
			//save_template_path = g_base_path + "data/_temp/save_data_template.ini";
			if (game_utils_string_copy(save_template_path, g_base_path) != 0) return 1;
			if (game_utils_string_copy(&save_template_path[g_base_path_size], "data/_temp/save_data_template.ini") != 0) return 1;
			if (game_save_load_ini_file(save_template_path) != 0) return 1;
			if (game_save_save_ini_file(g_save_path) != 0) return 1;
		}
	}
#elif _ANDROID
	if (game_utils_string_copy(g_save_path, (char*)"save_data.ini") == 0) {
		//if (1) { // enable, if overwrite by default values
		if (game_save_load_ini_file(g_save_path) != 0) {
			// create new file, copy template ini file:
			if (game_utils_string_copy(save_template_path, "data/temp/save_data_template.ini") != 0) return 1;
			if (game_save_load_ini_file(save_template_path) != 0) return 1;
			if (game_save_save_ini_file(g_save_path) != 0) return 1;
		}
	}
#else
	tmp_path = getenv("HOME");
	if (tmp_path) {
		//g_save_folder = tmp_path;
		//g_save_folder = g_save_folder + "/.config/nummc/";
		if (game_utils_string_cat(g_save_folder, tmp_path, (char*)"/.config/nummc/") <= 0) return 1;
		g_save_folder_size = (int)strlen(g_save_folder);

		//g_save_path = g_save_folder + "save_data.ini";
		if (game_utils_string_cat(g_save_path, g_save_folder, (char*)"save_data.ini") <= 0) return 1;

		//free(tmp_path);
		tmp_path = NULL;

		// check folder exist & create
		if (game_utils_create_folder(g_save_folder)) {
			return 1;
		}

		// check file exist & load
		if (access(g_save_path, R_OK | W_OK) != -1) {
			if (game_save_load_ini_file(g_save_path) != 0) return 1;
			LOG_DEBUG_CONSOLE("g_save_path: %s\n", g_save_path);

			// save backup
			char backup_save_path[GAME_FULL_PATH_MAX];
			ret = game_utils_string_cat(backup_save_path, g_save_path, (char*)".backup");
			if (ret <= 0) { LOG_ERROR_CONSOLE("Error: game_save_init() get backup_save_path\n"); return 1; }
			if (game_save_save_ini_file(backup_save_path) != 0) return 1;
		}
		else {
			//save_template_path = g_base_path + "data/_temp/save_data_template.ini";
			if (game_utils_string_copy(save_template_path, g_base_path) != 0) return 1;
			if (game_utils_string_copy(&save_template_path[g_base_path_size], "data/_temp/save_data_template.ini") != 0) return 1;
			if (game_save_load_ini_file(save_template_path) != 0) return 1;
			if (game_save_save_ini_file(g_save_path) != 0) return 1;
		}
	}
#endif
	else {
		LOG_ERROR("Error: Can't get USERPROFILE\n");
		return 1;
	}
	return 0;
}

int game_save_config_save()
{
	if (game_save_save_ini_file(g_save_path) != 0) return 1;
	return 0;
}

void game_save_close()
{
	game_config_data_delete();
}

void game_save_get_config_default_slot(int* default_slot_index)
{
	if (game_config_data.settings[GAME_SAVE_SETTINGS_ID_DEFAULT_SLOT][0] == '\0')
	{
		*default_slot_index = -1;
		return;
	}

	*default_slot_index = atoi(game_config_data.settings[GAME_SAVE_SETTINGS_ID_DEFAULT_SLOT]);
	*default_slot_index -= 1;
}
int game_save_set_config_default_slot(int default_slot_index)
{
	if (default_slot_index < 0) {
		game_config_data.settings[GAME_SAVE_SETTINGS_ID_DEFAULT_SLOT][0] = '\0';
		return 0;
	}

	char c_buff[4] = { '\0' };
	game_utils_string_itoa((default_slot_index + 1), c_buff, (4-1), 10);
	int ret = game_utils_string_copy(game_config_data.settings[GAME_SAVE_SETTINGS_ID_DEFAULT_SLOT], c_buff);
	if (ret != 0) { LOG_ERROR_CONSOLE("Error: game_save_set_config_default_slot() copy slot\n"); return 1; }

	return 0;
}

void game_save_get_config_unlock_stat(int* unlock_stat)
{
	if (game_config_data.settings[GAME_SAVE_SETTINGS_ID_UNLOCK_STAT][0] == '\0')
	{
		*unlock_stat = (0x00000002); // infinity only
		return;
	}

	*unlock_stat = atoi(game_config_data.settings[GAME_SAVE_SETTINGS_ID_UNLOCK_STAT]);
}
int game_save_set_config_unlock_stat(int unlock_stat)
{
	char c_buff[10] = { '\0' };
	game_utils_string_itoa(unlock_stat, c_buff, (10 - 1), 10);
	int ret = game_utils_string_copy(game_config_data.settings[GAME_SAVE_SETTINGS_ID_UNLOCK_STAT], c_buff);
	if (ret != 0) { LOG_ERROR_CONSOLE("Error: game_save_set_config_unlock_stat() copy unlock_stat\n"); return 1; }

	return 0;
}

void game_save_get_config_resolution(int* w, int* h)
{
	*w = atoi(game_config_data.settings[GAME_SAVE_SETTINGS_ID_RESOLUTION_W]);
	*h = atoi(game_config_data.settings[GAME_SAVE_SETTINGS_ID_RESOLUTION_H]);
}
int game_save_set_config_resolution(int w, int h)
{
	int ret = 0;
	char c_buff_w[8] = { '\0' };
	char c_buff_h[8] = { '\0' };
	game_utils_string_itoa(w, c_buff_w, (8 - 1), 10);
	game_utils_string_itoa(h, c_buff_h, (8 - 1), 10);

	// do not overwrite
	if ((STRCMP_EQ(c_buff_w,game_config_data.settings[GAME_SAVE_SETTINGS_ID_RESOLUTION_W]))
		&& (STRCMP_EQ(c_buff_h,game_config_data.settings[GAME_SAVE_SETTINGS_ID_RESOLUTION_H]))) return 1;

	ret = game_utils_string_copy(game_config_data.settings[GAME_SAVE_SETTINGS_ID_RESOLUTION_W], c_buff_w);
	if (ret != 0) { LOG_ERROR_CONSOLE("Error: game_save_set_config_resolution() copy resolution_w\n"); return 1; }

	ret = game_utils_string_copy(game_config_data.settings[GAME_SAVE_SETTINGS_ID_RESOLUTION_H], c_buff_h);
	if (ret != 0) { LOG_ERROR_CONSOLE("Error: game_save_set_config_resolution() copy resolution_h\n"); return 1; }

	return 0;
}

void game_save_get_config_music_volume(int* volume)
{
	if (game_config_data.settings[GAME_SAVE_SETTINGS_ID_MUSIC_VOLUME][0] == '\0')
	{
		*volume = GAME_SAVE_CONFIG_MUSIC_VOLUME_DEFAULT;
		return;
	}

	*volume = atoi(game_config_data.settings[GAME_SAVE_SETTINGS_ID_MUSIC_VOLUME]);
}
int game_save_set_config_music_volume(int volume)
{
	if (volume < 0) {
		game_config_data.settings[GAME_SAVE_SETTINGS_ID_MUSIC_VOLUME][0] = '\0';
		return 0;
	}

	char c_buff[4] = { '\0' };
	game_utils_string_itoa(volume, c_buff, 4, 10);
	if (STRCMP_EQ(c_buff, game_config_data.settings[GAME_SAVE_SETTINGS_ID_MUSIC_VOLUME])) return 1;

	int ret = game_utils_string_copy(game_config_data.settings[GAME_SAVE_SETTINGS_ID_MUSIC_VOLUME], c_buff);
	if (ret != 0) { LOG_ERROR_CONSOLE("Error: game_save_set_config_music_volume() copy music_volume\n"); return 1; }

	return 0;
}

void game_save_get_config_sfx_volume(int* volume)
{
	if (game_config_data.settings[GAME_SAVE_SETTINGS_ID_SFX_VOLUME][0] == '\0')
	{
		*volume = GAME_SAVE_CONFIG_SFX_VOLUME_DEFAULT;
		return;
	}

	*volume = atoi(game_config_data.settings[GAME_SAVE_SETTINGS_ID_SFX_VOLUME]);
}
int game_save_set_config_sfx_volume(int volume)
{
	if (volume < 0) {
		game_config_data.settings[GAME_SAVE_SETTINGS_ID_SFX_VOLUME][0] = '\0';
		return 0;
	}

	char c_buff[4] = { '\0' };
	game_utils_string_itoa(volume, c_buff, 4, 10);
	if (STRCMP_EQ(c_buff, game_config_data.settings[GAME_SAVE_SETTINGS_ID_SFX_VOLUME])) return 1;

	int ret = game_utils_string_copy(game_config_data.settings[GAME_SAVE_SETTINGS_ID_SFX_VOLUME], c_buff);
	if (ret != 0) { LOG_ERROR_CONSOLE("Error: game_save_set_config_sfx_volume() copy sfx_volume\n"); return 1; }

	return 0;
}

void game_save_get_config_player(int slot_index, unit_player_data_t* player)
{
	if (player->obj != NULL) {
		const char* c_path = game_config_data.player[slot_index][GAME_SAVE_PLAYER_ID_PATH];
		game_utils_string_copy((char *)player->obj, c_path);
	}
	player->hp                = atoi(game_config_data.player[slot_index][GAME_SAVE_PLAYER_ID_HP]);
	player->exp               = atoi(game_config_data.player[slot_index][GAME_SAVE_PLAYER_ID_EXP]);
	player->attack_wait_timer = atoi(game_config_data.player[slot_index][GAME_SAVE_PLAYER_ID_ATTACK_WAIT_TIMER]);
	player->bullet_life_timer = atoi(game_config_data.player[slot_index][GAME_SAVE_PLAYER_ID_BULLET_LIFE_TIMER]);
	player->bullet_spec       = atoi(game_config_data.player[slot_index][GAME_SAVE_PLAYER_ID_BULLET_SPEC]);

	player->speed             = atoi(game_config_data.player[slot_index][GAME_SAVE_PLAYER_ID_SPEED]);
	player->weapon            = atoi(game_config_data.player[slot_index][GAME_SAVE_PLAYER_ID_WEAPON]);
	player->armor             = atoi(game_config_data.player[slot_index][GAME_SAVE_PLAYER_ID_ARMOR]);
	player->spec              = atoi(game_config_data.player[slot_index][GAME_SAVE_PLAYER_ID_SPEC]);

	player->hp_max            = atoi(game_config_data.player[slot_index][GAME_SAVE_PLAYER_ID_HP_MAX]);
	player->exp_max           = atoi(game_config_data.player[slot_index][GAME_SAVE_PLAYER_ID_EXP_MAX]);
	player->level             = atoi(game_config_data.player[slot_index][GAME_SAVE_PLAYER_ID_LEVEL]);
}

static int set_config_player_int(int slot_index, int player_id, int value)
{
	int ret = 0;

	char c_buff[MEMORY_MANAGER_NAME_BUF_SIZE] = { '\0' };
	int c_buff_size = MEMORY_MANAGER_NAME_BUF_SIZE;

	game_utils_string_itoa(value, c_buff, (c_buff_size - 1), 10);
	ret = game_utils_string_copy(game_config_data.player[slot_index][player_id], c_buff);
	if (ret != 0) { LOG_ERROR_CONSOLE("Error: set_config_player_int() copy %d %d\n", slot_index, player_id); return 1; }

	return 0;
}

int game_save_set_config_player(int slot_index, unit_player_data_t* player)
{
	if (player->obj != NULL) {
		int ret = game_utils_string_copy(game_config_data.player[slot_index][GAME_SAVE_PLAYER_ID_PATH], (char*)player->obj);
		if (ret != 0) { LOG_ERROR_CONSOLE("Error: game_save_set_config_player() copy path\n"); return 1; }
	}

	if (set_config_player_int(slot_index, GAME_SAVE_PLAYER_ID_HP, player->hp) != 0) return 1;
	if (set_config_player_int(slot_index, GAME_SAVE_PLAYER_ID_EXP, player->exp) != 0) return 1;
	if (set_config_player_int(slot_index, GAME_SAVE_PLAYER_ID_ATTACK_WAIT_TIMER, player->attack_wait_timer) != 0) return 1;
	if (set_config_player_int(slot_index, GAME_SAVE_PLAYER_ID_BULLET_LIFE_TIMER, player->bullet_life_timer) != 0) return 1;
	if (set_config_player_int(slot_index, GAME_SAVE_PLAYER_ID_BULLET_SPEC, player->bullet_spec) != 0) return 1;
	if (set_config_player_int(slot_index, GAME_SAVE_PLAYER_ID_SPEED, player->speed) != 0) return 1;
	if (set_config_player_int(slot_index, GAME_SAVE_PLAYER_ID_WEAPON, player->weapon) != 0) return 1;
	if (set_config_player_int(slot_index, GAME_SAVE_PLAYER_ID_ARMOR, player->armor) != 0) return 1;
	if (set_config_player_int(slot_index, GAME_SAVE_PLAYER_ID_SPEC, player->spec) != 0) return 1;
	if (set_config_player_int(slot_index, GAME_SAVE_PLAYER_ID_HP_MAX, player->hp_max) != 0) return 1;
	if (set_config_player_int(slot_index, GAME_SAVE_PLAYER_ID_EXP_MAX, player->exp_max) != 0) return 1;
	if (set_config_player_int(slot_index, GAME_SAVE_PLAYER_ID_LEVEL, player->level) != 0) return 1;

	return 0;
}

void game_save_get_config_stocker(int slot_index, inventory_stocker_data_t* stocker)
{
	stocker->weapon_item->item_list_idx  = atoi(game_config_data.stocker[slot_index][GAME_SAVE_STOCKER_ID_WEAPON_ITEM_LIST_IDX]);
	stocker->weapon_item->item_count     = atoi(game_config_data.stocker[slot_index][GAME_SAVE_STOCKER_ID_WEAPON_ITEM_COUNT]);

	stocker->charge_item->item_list_idx  = atoi(game_config_data.stocker[slot_index][GAME_SAVE_STOCKER_ID_CHARGE_ITEM_LIST_IDX]);
	stocker->charge_item->item_count     = atoi(game_config_data.stocker[slot_index][GAME_SAVE_STOCKER_ID_CHARGE_ITEM_COUNT]);
	stocker->charge_item->charge_val     = atoi(game_config_data.stocker[slot_index][GAME_SAVE_STOCKER_ID_CHARGE_CHARGE_VAL]);
	stocker->charge_item->charge_timer   = atoi(game_config_data.stocker[slot_index][GAME_SAVE_STOCKER_ID_CHARGE_CHARGE_TIMER]);

	stocker->special_item->item_list_idx = atoi(game_config_data.stocker[slot_index][GAME_SAVE_STOCKER_ID_SPECIAL_ITEM_LIST_IDX]);
	stocker->special_item->item_count    = atoi(game_config_data.stocker[slot_index][GAME_SAVE_STOCKER_ID_SPECIAL_ITEM_COUNT]);
}

static int set_config_stocker_int(int slot_index, int stocker_id, int value)
{
	int ret = 0;

	char c_buff[MEMORY_MANAGER_NAME_BUF_SIZE] = { '\0' };
	int c_buff_size = MEMORY_MANAGER_NAME_BUF_SIZE;

	game_utils_string_itoa(value, c_buff, (c_buff_size - 1), 10);
	ret = game_utils_string_copy(game_config_data.stocker[slot_index][stocker_id], c_buff);
	if (ret != 0) { LOG_ERROR_CONSOLE("Error: set_config_stocker_int() copy %d %d\n", slot_index, stocker_id); return 1; }

	return 0;
}

int game_save_set_config_stocker(int slot_index, inventory_stocker_data_t* stocker)
{
	if (set_config_stocker_int(slot_index, GAME_SAVE_STOCKER_ID_WEAPON_ITEM_LIST_IDX, stocker->weapon_item->item_list_idx) != 0) return 1;
	if (set_config_stocker_int(slot_index, GAME_SAVE_STOCKER_ID_WEAPON_ITEM_COUNT, stocker->weapon_item->item_count) != 0) return 1;

	if (set_config_stocker_int(slot_index, GAME_SAVE_STOCKER_ID_CHARGE_ITEM_LIST_IDX, stocker->charge_item->item_list_idx) != 0) return 1;
	if (set_config_stocker_int(slot_index, GAME_SAVE_STOCKER_ID_CHARGE_ITEM_COUNT, stocker->charge_item->item_count) != 0) return 1;
	if (set_config_stocker_int(slot_index, GAME_SAVE_STOCKER_ID_CHARGE_CHARGE_VAL, stocker->charge_item->charge_val) != 0) return 1;
	if (set_config_stocker_int(slot_index, GAME_SAVE_STOCKER_ID_CHARGE_CHARGE_TIMER, stocker->charge_item->charge_timer) != 0) return 1;

	if (set_config_stocker_int(slot_index, GAME_SAVE_STOCKER_ID_SPECIAL_ITEM_LIST_IDX, stocker->special_item->item_list_idx) != 0) return 1;
	if (set_config_stocker_int(slot_index, GAME_SAVE_STOCKER_ID_SPECIAL_ITEM_COUNT, stocker->special_item->item_count) != 0) return 1;

	return 0;
}

void game_save_get_config_player_backup(int slot_index)
{
	unit_manager_clear_player_backup();
	game_save_get_config_player(slot_index, &g_player_backup);
	inventory_manager_clear_stocker_backup();
	game_save_get_config_stocker(slot_index, &g_stocker_backup);
}

void game_save_get_config_slot(int slot_index, char* player, char* stage, char* timestamp)
{
	int ret = 0;

	// return values for save menu display
	if (player) {
		ret = game_utils_string_copy(player, game_config_data.slot[slot_index][GAME_SAVE_SLOT_ID_PLAYER]);
		if (ret != 0) { LOG_ERROR("Error: game_save_get_config_slot() copy player\n"); }
	}
	if (stage) {
		ret = game_utils_string_copy(stage, game_config_data.slot[slot_index][GAME_SAVE_SLOT_ID_STAGE]);
		if (ret != 0) { LOG_ERROR("Error: game_save_get_config_slot() copy stage\n"); }
	}
	if (timestamp) {
		ret = game_utils_string_copy(timestamp, game_config_data.slot[slot_index][GAME_SAVE_SLOT_ID_TIMESTAMP]);
		if (ret != 0) { LOG_ERROR("Error: game_save_get_config_slot() copy timestamp\n"); }
	}
}

static int set_config_slot_str(int slot_index, int slot_id, char* value)
{
	int ret = 0;

	ret = game_utils_string_copy(game_config_data.slot[slot_index][slot_id], value);
	if (ret != 0) { LOG_ERROR_CONSOLE("Error: set_config_slot_str() copy %d %d\n", slot_index, slot_id); return 1; }

	return 0;
}

int game_save_set_config_slot(int slot_index, char* player, char* stage, bool init_flag)
{
	// clear default slot
	if (init_flag) {
		int default_slot_index;
		game_save_get_config_default_slot(&default_slot_index);
		if (default_slot_index == slot_index) {
			game_save_set_config_default_slot(-1);
		}
	}

	// set current timestamp
	char tmp_char_buf[MEMORY_MANAGER_STRING_BUF_SIZE];
	game_utils_get_localtime(tmp_char_buf, sizeof(tmp_char_buf));

	// save values for save menu display
	if (set_config_slot_str(slot_index, GAME_SAVE_SLOT_ID_PLAYER, player) != 0) {
		LOG_ERROR("game_save_set_config_slot() player=%s\n", player);
		return 1;
	}
	if (set_config_slot_str(slot_index, GAME_SAVE_SLOT_ID_STAGE, stage) != 0) {
		LOG_ERROR("game_save_set_config_slot() stage=%s\n", stage);
		return 1;
	}
	if (set_config_slot_str(slot_index, GAME_SAVE_SLOT_ID_TIMESTAMP, tmp_char_buf) != 0) {
		LOG_ERROR("game_save_set_config_slot() timestamp=%s\n", tmp_char_buf);
		return 1;
	}

	if (init_flag) {
		// don't need to clear player & stacker
		return 0;
	}

	// save from global values
	game_save_set_config_player(slot_index, &g_player);
	game_save_set_config_stocker(slot_index, &g_stocker);

	return 0;
}
