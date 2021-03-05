#include <vector>
#include <map>
#include <io.h>
#include <fstream>
#include <time.h>
#include "game_common.h"
#include "game_save.h"

#include "game_utils.h"
#include "game_log.h"

#define GAME_SAVE_SLOT_NUM 8

// .ini data
typedef struct _ini_data_t ini_data_t;
struct _ini_data_t {
	std::string section;
	std::map<std::string, std::string> values;
};
#define INI_DATA_BUFFER_SIZE  (1 + GAME_SAVE_SLOT_NUM * 3)  /* settings, slot, player, stocker */
static ini_data_t ini_data_buffer[INI_DATA_BUFFER_SIZE];

// config data
typedef struct _game_config_data_t game_config_data_t;
struct _game_config_data_t {
	ini_data_t* settings;
	ini_data_t* slot[GAME_SAVE_SLOT_NUM];
	ini_data_t* player[GAME_SAVE_SLOT_NUM];
	ini_data_t* stocker[GAME_SAVE_SLOT_NUM];
};
static game_config_data_t game_config_data;

// tmp region
static char g_save_path[GAME_FULL_PATH_MAX];
static char save_template_path[GAME_FULL_PATH_MAX];
static char tmp_char_buf[GAME_UTILS_STRING_CHAR_BUF_SIZE];

// extern variables
int g_save_folder_size;
char g_save_folder[GAME_FULL_PATH_MAX];

static int game_config_data_alloc()
{
	int index = 0;

	game_config_data.settings = &ini_data_buffer[index];
	index++;

	for (int i = 0; i < GAME_SAVE_SLOT_NUM; i++) {
		game_config_data.slot[i] = &ini_data_buffer[index];
		index++;
	}

	for (int i = 0; i < GAME_SAVE_SLOT_NUM; i++) {
		game_config_data.player[i] = &ini_data_buffer[index];
		index++;
	}

	for (int i = 0; i < GAME_SAVE_SLOT_NUM; i++) {
		game_config_data.stocker[i] = &ini_data_buffer[index];
		index++;
	}

	return 0;
}

static void game_config_data_delete()
{
	game_config_data.settings = NULL;

	for (int i = 0; i < GAME_SAVE_SLOT_NUM; i++) {
		game_config_data.slot[i] = NULL;
	}

	for (int i = 0; i < GAME_SAVE_SLOT_NUM; i++) {
		game_config_data.player[i] = NULL;
	}

	for (int i = 0; i < GAME_SAVE_SLOT_NUM; i++) {
		game_config_data.stocker[i] = NULL;
	}
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
	load_ini_file_callback_data_t* data = (load_ini_file_callback_data_t*)argv;
	char tag_name[GAME_UTILS_STRING_NAME_BUF_SIZE];
	char key[GAME_UTILS_STRING_NAME_BUF_SIZE];
	char value[GAME_UTILS_STRING_CHAR_BUF_SIZE];

	if (line[0] == '\0') { return; }
	if (line[0] == '#') return;

	if (line[0] == '[') {
		if (STRCMP_EQ(line, "[settings]")) {
			// clear other flg
			memset(data->read_slot_flg,    0, sizeof(bool) * GAME_SAVE_SLOT_NUM);
			memset(data->read_player_flg,  0, sizeof(bool) * GAME_SAVE_SLOT_NUM);
			memset(data->read_stocker_flg, 0, sizeof(bool) * GAME_SAVE_SLOT_NUM);

			data->read_flg = true;
			game_config_data.settings->section = line;
			return;
		}

		int ret = game_utils_string_copy(tag_name, (char*)"[slot1]"); // [slot1...8]
		if (ret != 0) { LOG_ERROR("Error: load_ini_file_callback() [slot1] %d\n", line_num);  return; }
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
				game_config_data.slot[i]->section = line;
				break;
			}
			tag_name[5] += 1;
		}
		if (found_slot) return;

		ret = game_utils_string_copy(tag_name, (char*)"[player1]"); // [player1...8]
		if (ret != 0) { LOG_ERROR("Error: load_ini_file_callback() [player1] %d\n", line_num);  return; }
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
				game_config_data.player[i]->section = line;
				break;
			}
			tag_name[7] += 1;
		}
		if (found_slot) return;

		ret = game_utils_string_copy(tag_name, (char*)"[stocker1]"); // [stocker1...8]
		if (ret != 0) { LOG_ERROR("Error: load_ini_file_callback() [stocker1] %d\n", line_num);  return; }
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
				game_config_data.stocker[i]->section = line;
				break;
			}
			tag_name[8] += 1;
		}
		if (found_slot) return;
	}

	if (data->read_flg) {
		if (game_utils_split_key_value(line, key, value) == 0) {
			game_config_data.settings->values.insert(std::pair<std::string, std::string>(key, value));
		}
	}
	for (int i = 0; i < GAME_SAVE_SLOT_NUM; i++) {
		if (data->read_slot_flg[i]) {
			if (game_utils_split_key_value(line, key, value) == 0) {
				game_config_data.slot[i]->values.insert(std::pair<std::string, std::string>(key, value));
			}
			break;
		}
	}
	for (int i = 0; i < GAME_SAVE_SLOT_NUM; i++) {
		if (data->read_player_flg[i]) {
			if (game_utils_split_key_value(line, key, value) == 0) {
				game_config_data.player[i]->values.insert(std::pair<std::string, std::string>(key, value));
			}
			break;
		}
	}
	for (int i = 0; i < GAME_SAVE_SLOT_NUM; i++) {
		if (data->read_stocker_flg[i]) {
			if (game_utils_split_key_value(line, key, value) == 0) {
				game_config_data.stocker[i]->values.insert(std::pair<std::string, std::string>(key, value));
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

static int game_save_save_ini_file(std::string path) {
	std::ofstream outFile(path);
	if (outFile.is_open()) {
		std::string line = game_config_data.settings->section + "\n";
		outFile.write(line.c_str(), line.size());

		for (auto it = game_config_data.settings->values.begin(); it != game_config_data.settings->values.end(); it++) {
			line = it->first + "=" + it->second + "\n";
			outFile.write(line.c_str(), line.size());
		}

		for (int i = 0; i < GAME_SAVE_SLOT_NUM; i++) {
			std::string line = game_config_data.slot[i]->section + "\n";
			outFile.write(line.c_str(), line.size());

			for (auto it = game_config_data.slot[i]->values.begin(); it != game_config_data.slot[i]->values.end(); it++) {
				line = it->first + "=" + it->second + "\n";
				outFile.write(line.c_str(), line.size());
			}
		}

		for (int i = 0; i < GAME_SAVE_SLOT_NUM; i++) {
			std::string line = game_config_data.player[i]->section + "\n";
			outFile.write(line.c_str(), line.size());

			for (auto it = game_config_data.player[i]->values.begin(); it != game_config_data.player[i]->values.end(); it++) {
				line = it->first + "=" + it->second + "\n";
				outFile.write(line.c_str(), line.size());
			}
		}

		for (int i = 0; i < GAME_SAVE_SLOT_NUM; i++) {
			std::string line = game_config_data.stocker[i]->section + "\n";
			outFile.write(line.c_str(), line.size());

			for (auto it = game_config_data.stocker[i]->values.begin(); it != game_config_data.stocker[i]->values.end(); it++) {
				line = it->first + "=" + it->second + "\n";
				outFile.write(line.c_str(), line.size());
			}
		}

		outFile.close();
	}
	else {
		LOG_ERROR("game_save_save_ini_file %s error\n", path.c_str());
		return 1;
	}
	return 0;
}

int game_save_init()
{
	int ret = 0;
	char* tmp_path = NULL;
	size_t tmp_path_len;

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
		}
		else {
			//save_template_path = g_base_path + "data/_temp/save_data_template.ini";
			if (game_utils_string_copy(save_template_path, g_base_path) != 0) return 1;
			if (game_utils_string_copy(&save_template_path[g_base_path_size], "data/_temp/save_data_template.ini") != 0) return 1;
			if (game_save_load_ini_file(save_template_path) != 0) return 1;
			if (game_save_save_ini_file(g_save_path) != 0) return 1;
		}
	}
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
	if (game_config_data.settings->values["default_slot"] == "")
	{
		*default_slot_index = -1;
		return;
	}

	*default_slot_index = atoi(game_config_data.settings->values["default_slot"].c_str());
	*default_slot_index -= 1;
}
int game_save_set_config_default_slot(int default_slot_index)
{
	if (default_slot_index < 0) {
		game_config_data.settings->values["default_slot"] = "";
		return 0;
	}

	char c_buff[2] = { '\0' };
	_itoa_s((default_slot_index + 1), c_buff, 10);
	std::string new_slot_index = c_buff;
	game_config_data.settings->values["default_slot"] = new_slot_index;
	return 0;
}

void game_save_get_config_unlock_stat(int* unlock_stat)
{
	if (game_config_data.settings->values["unlock_stat"] == "")
	{
		*unlock_stat = (0x00000002); // infinity only
		return;
	}

	*unlock_stat = atoi(game_config_data.settings->values["unlock_stat"].c_str());
}
int game_save_set_config_unlock_stat(int unlock_stat)
{
	char c_buff[10] = { '\0' };
	_itoa_s(unlock_stat, c_buff, 10);
	std::string new_unlock_stat = c_buff;
	game_config_data.settings->values["unlock_stat"] = new_unlock_stat;
	return 0;
}

void game_save_get_config_resolution(int* w, int* h)
{
	*w = atoi(game_config_data.settings->values["resolution_w"].c_str());
	*h = atoi(game_config_data.settings->values["resolution_h"].c_str());
}
int game_save_set_config_resolution(int w, int h)
{
	char c_buff_w[5] = { '\0' };
	char c_buff_h[5] = { '\0' };
	_itoa_s(w, c_buff_w, 10);
	_itoa_s(h, c_buff_h, 10);
	std::string new_w = c_buff_w;
	std::string new_h = c_buff_h;

	// do not overwrite
	if ((new_w == game_config_data.settings->values["resolution_w"])
		&& (new_h == game_config_data.settings->values["resolution_h"])) return 1;

	game_config_data.settings->values["resolution_w"] = new_w;
	game_config_data.settings->values["resolution_h"] = new_h;
	return 0;
}

void game_save_get_config_music_volume(int* volume)
{
	if (game_config_data.settings->values["music_volume"] == "")
	{
		*volume = GAME_SAVE_CONFIG_MUSIC_VOLUME_DEFAULT;
		return;
	}

	*volume = atoi(game_config_data.settings->values["music_volume"].c_str());
}
int game_save_set_config_music_volume(int volume)
{
	if (volume < 0) {
		game_config_data.settings->values["music_volume"] = "";
		return 0;
	}

	char c_buff[4] = { '\0' };
	_itoa_s((volume), c_buff, 10);
	std::string new_volume = c_buff;
	if (new_volume == game_config_data.settings->values["music_volume"]) return 1;

	game_config_data.settings->values["music_volume"] = new_volume;
	return 0;
}

void game_save_get_config_sfx_volume(int* volume)
{
	if (game_config_data.settings->values["sfx_volume"] == "")
	{
		*volume = GAME_SAVE_CONFIG_SFX_VOLUME_DEFAULT;
		return;
	}

	*volume = atoi(game_config_data.settings->values["sfx_volume"].c_str());
}
int game_save_set_config_sfx_volume(int volume)
{
	if (volume < 0) {
		game_config_data.settings->values["sfx_volume"] = "";
		return 0;
	}

	char c_buff[4] = { '\0' };
	_itoa_s((volume), c_buff, 10);
	std::string new_volume = c_buff;
	if (new_volume == game_config_data.settings->values["sfx_volume"]) return 1;

	game_config_data.settings->values["sfx_volume"] = new_volume;
	return 0;
}

void game_save_get_config_player(int slot_index, unit_player_data_t* player)
{
	if (player->obj != NULL) {
		const char* c_path = game_config_data.player[slot_index]->values["path"].c_str();
		game_utils_string_copy((char *)player->obj, c_path);
	}
	player->hp = atoi(game_config_data.player[slot_index]->values["hp"].c_str());
	player->exp = atoi(game_config_data.player[slot_index]->values["exp"].c_str());
	player->attack_wait_timer = atoi(game_config_data.player[slot_index]->values["attack_wait_timer"].c_str());
	player->bullet_life_timer = atoi(game_config_data.player[slot_index]->values["bullet_life_timer"].c_str());
	player->bullet_spec = atoi(game_config_data.player[slot_index]->values["bullet_spec"].c_str());

	player->speed = atoi(game_config_data.player[slot_index]->values["speed"].c_str());
	player->weapon = atoi(game_config_data.player[slot_index]->values["weapon"].c_str());
	player->armor = atoi(game_config_data.player[slot_index]->values["armor"].c_str());
	player->spec = atoi(game_config_data.player[slot_index]->values["spec"].c_str());

	player->hp_max = atoi(game_config_data.player[slot_index]->values["hp_max"].c_str());
	player->exp_max = atoi(game_config_data.player[slot_index]->values["exp_max"].c_str());
	player->level = atoi(game_config_data.player[slot_index]->values["level"].c_str());
}

int game_save_set_config_player(int slot_index, unit_player_data_t* player)
{
	char c_buff[12] = { '\0' };
	std::string new_str;

	if (player->obj != NULL) {
		std::string player_path = (char *)player->obj;
		game_config_data.player[slot_index]->values["path"] = player_path;
	}

	_itoa_s((player->hp), c_buff, 10); new_str = c_buff;
	game_config_data.player[slot_index]->values["hp"] = new_str;
	memset(c_buff, '\0', sizeof(c_buff)); _itoa_s((player->exp), c_buff, 10); new_str = c_buff;
	game_config_data.player[slot_index]->values["exp"] = new_str;
	memset(c_buff, '\0', sizeof(c_buff)); _itoa_s((player->attack_wait_timer), c_buff, 10); new_str = c_buff;
	game_config_data.player[slot_index]->values["attack_wait_timer"] = new_str;
	memset(c_buff, '\0', sizeof(c_buff)); _itoa_s((player->bullet_life_timer), c_buff, 10); new_str = c_buff;
	game_config_data.player[slot_index]->values["bullet_life_timer"] = new_str;
	memset(c_buff, '\0', sizeof(c_buff)); _itoa_s((player->bullet_spec), c_buff, 10); new_str = c_buff;
	game_config_data.player[slot_index]->values["bullet_spec"] = new_str;

	memset(c_buff, '\0', sizeof(c_buff)); _itoa_s((player->speed), c_buff, 10); new_str = c_buff;
	game_config_data.player[slot_index]->values["speed"] = new_str;
	memset(c_buff, '\0', sizeof(c_buff)); _itoa_s((player->weapon), c_buff, 10); new_str = c_buff;
	game_config_data.player[slot_index]->values["weapon"] = new_str;
	memset(c_buff, '\0', sizeof(c_buff)); _itoa_s((player->armor), c_buff, 10); new_str = c_buff;
	game_config_data.player[slot_index]->values["armor"] = new_str;
	memset(c_buff, '\0', sizeof(c_buff)); _itoa_s((player->spec), c_buff, 10); new_str = c_buff;
	game_config_data.player[slot_index]->values["spec"] = new_str;

	memset(c_buff, '\0', sizeof(c_buff)); _itoa_s((player->hp_max), c_buff, 10); new_str = c_buff;
	game_config_data.player[slot_index]->values["hp_max"] = new_str;
	memset(c_buff, '\0', sizeof(c_buff)); _itoa_s((player->exp_max), c_buff, 10); new_str = c_buff;
	game_config_data.player[slot_index]->values["exp_max"] = new_str;
	memset(c_buff, '\0', sizeof(c_buff)); _itoa_s((player->level), c_buff, 10); new_str = c_buff;
	game_config_data.player[slot_index]->values["level"] = new_str;

	return 0;
}

void game_save_get_config_stocker(int slot_index, inventory_stocker_data_t* stocker)
{
	stocker->weapon_item->item_list_idx  = atoi(game_config_data.stocker[slot_index]->values["weapon_item_list_idx"].c_str());
	stocker->weapon_item->item_count     = atoi(game_config_data.stocker[slot_index]->values["weapon_item_count"].c_str());

	stocker->charge_item->item_list_idx  = atoi(game_config_data.stocker[slot_index]->values["charge_item_list_idx"].c_str());
	stocker->charge_item->item_count     = atoi(game_config_data.stocker[slot_index]->values["charge_item_count"].c_str());
	stocker->charge_item->charge_val     = atoi(game_config_data.stocker[slot_index]->values["charge_charge_val"].c_str());
	stocker->charge_item->charge_timer   = atoi(game_config_data.stocker[slot_index]->values["charge_charge_timer"].c_str());

	stocker->special_item->item_list_idx = atoi(game_config_data.stocker[slot_index]->values["special_item_list_idx"].c_str());
	stocker->special_item->item_count    = atoi(game_config_data.stocker[slot_index]->values["special_item_count"].c_str());
}

int game_save_set_config_stocker(int slot_index, inventory_stocker_data_t* stocker)
{
	char c_buff[12] = { '\0' };
	std::string new_str;

	_itoa_s((stocker->weapon_item->item_list_idx), c_buff, 10); new_str = c_buff;
	game_config_data.stocker[slot_index]->values["weapon_item_list_idx"] = new_str;
	memset(c_buff, '\0', sizeof(c_buff)); _itoa_s((stocker->weapon_item->item_count), c_buff, 10); new_str = c_buff;
	game_config_data.stocker[slot_index]->values["weapon_item_count"] = new_str;

	memset(c_buff, '\0', sizeof(c_buff)); _itoa_s((stocker->charge_item->item_list_idx), c_buff, 10); new_str = c_buff;
	game_config_data.stocker[slot_index]->values["charge_item_list_idx"] = new_str;
	memset(c_buff, '\0', sizeof(c_buff)); _itoa_s((stocker->charge_item->item_count), c_buff, 10); new_str = c_buff;
	game_config_data.stocker[slot_index]->values["charge_item_count"] = new_str;
	memset(c_buff, '\0', sizeof(c_buff)); _itoa_s((stocker->charge_item->charge_val), c_buff, 10); new_str = c_buff;
	game_config_data.stocker[slot_index]->values["charge_charge_val"] = new_str;
	memset(c_buff, '\0', sizeof(c_buff)); _itoa_s((stocker->charge_item->charge_timer), c_buff, 10); new_str = c_buff;
	game_config_data.stocker[slot_index]->values["charge_charge_timer"] = new_str;

	memset(c_buff, '\0', sizeof(c_buff)); _itoa_s((stocker->special_item->item_list_idx), c_buff, 10); new_str = c_buff;
	game_config_data.stocker[slot_index]->values["special_item_list_idx"] = new_str;
	memset(c_buff, '\0', sizeof(c_buff)); _itoa_s((stocker->special_item->item_count), c_buff, 10); new_str = c_buff;
	game_config_data.stocker[slot_index]->values["special_item_count"] = new_str;

	return 0;
}

void game_save_get_config_player_backup(int slot_index)
{
	unit_manager_clear_player_backup();
	game_save_get_config_player(slot_index, &g_player_backup);
	inventory_manager_clear_stocker_backup();
	game_save_get_config_stocker(slot_index, &g_stocker_backup);
}

void game_save_get_config_slot(int slot_index, std::string& player, std::string& stage, std::string& timestamp)
{
	// return values for save menu display
	player = game_config_data.slot[slot_index]->values["player"];
	stage = game_config_data.slot[slot_index]->values["stage"];
	timestamp = game_config_data.slot[slot_index]->values["timestamp"];
}

int game_save_set_config_slot(int slot_index, std::string player, std::string stage, bool init_flag)
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
	game_utils_get_localtime(tmp_char_buf, sizeof(tmp_char_buf));

	// save values for save menu display
	game_config_data.slot[slot_index]->values["player"] = player;
	game_config_data.slot[slot_index]->values["stage"] = stage;
	game_config_data.slot[slot_index]->values["timestamp"] = tmp_char_buf;

	if (init_flag) {
		// don't need to clear player & stacker
		return 0;
	}

	// save from global values
	game_save_set_config_player(slot_index, &g_player);
	game_save_set_config_stocker(slot_index, &g_stocker);

	return 0;
}
