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

typedef struct _ini_data_t ini_data_t;
struct _ini_data_t {
	std::string section;
	std::map<std::string, std::string> values;
};

typedef struct _game_config_data_t game_config_data_t;
struct _game_config_data_t {
	ini_data_t* settings;
	std::vector<ini_data_t*> slot;
	std::vector<ini_data_t*> player;
	std::vector<ini_data_t*> stocker;
};

static game_config_data_t game_config_data;
std::string g_save_folder;
std::string g_save_path;
std::string g_save_player_path[GAME_SAVE_SLOT_NUM];

static int game_config_data_alloc()
{
	game_config_data.settings = new ini_data_t;
	if (!game_config_data.settings) return -1;

	game_config_data.slot.resize(GAME_SAVE_SLOT_NUM);
	for (int i = 0; i < GAME_SAVE_SLOT_NUM; i++) {
		game_config_data.slot[i] = new ini_data_t;
		if (!game_config_data.slot[i]) return -1;
	}

	game_config_data.player.resize(GAME_SAVE_SLOT_NUM);
	for (int i = 0; i < GAME_SAVE_SLOT_NUM; i++) {
		game_config_data.player[i] = new ini_data_t;
		if (!game_config_data.player[i]) return -1;
	}

	game_config_data.stocker.resize(GAME_SAVE_SLOT_NUM);
	for (int i = 0; i < GAME_SAVE_SLOT_NUM; i++) {
		game_config_data.stocker[i] = new ini_data_t;
		if (!game_config_data.stocker[i]) return -1;
	}

	return 0;
}

static void game_config_data_delete()
{
	delete game_config_data.settings;
	game_config_data.settings = NULL;

	for (int i = 0; i < GAME_SAVE_SLOT_NUM; i++) {
		delete game_config_data.slot[i];
		game_config_data.slot[i] = NULL;
	}
	game_config_data.slot.clear();

	for (int i = 0; i < GAME_SAVE_SLOT_NUM; i++) {
		delete game_config_data.player[i];
		game_config_data.player[i] = NULL;
	}
	game_config_data.player.clear();

	for (int i = 0; i < GAME_SAVE_SLOT_NUM; i++) {
		delete game_config_data.stocker[i];
		game_config_data.stocker[i] = NULL;
	}
	game_config_data.stocker.clear();
}

static int game_save_load_ini_file(std::string path)
{
	if (game_config_data_alloc()) {
		LOG_ERROR("game_config_data_alloc error\n");
		return 1;
	}

	bool read_flg = false;
	bool read_slot_flg[GAME_SAVE_SLOT_NUM] = { false };
	bool read_player_flg[GAME_SAVE_SLOT_NUM] = { false };
	bool read_stocker_flg[GAME_SAVE_SLOT_NUM] = { false };
	std::ifstream inFile(path);
	if (inFile.is_open()) {
		std::string line;
		while (std::getline(inFile, line)) {
			if (line == "") { continue; }

			if (line[0] == '[') {
				if (line == "[settings]") {
					// clear other flg
					memset(read_slot_flg, 0, sizeof(read_slot_flg));
					memset(read_player_flg, 0, sizeof(read_player_flg));
					memset(read_stocker_flg, 0, sizeof(read_stocker_flg));

					read_flg = true;
					game_config_data.settings->section = line;
					continue;
				}

				std::string tag_name = "[slot1]"; // [slot1...8]
				bool found_slot = false;
				for (int i = 0; i < GAME_SAVE_SLOT_NUM; i++) {
					if (line == tag_name) {
						// clear other flg
						read_flg = false;
						memset(read_slot_flg, 0, sizeof(read_slot_flg));
						memset(read_player_flg, 0, sizeof(read_player_flg));
						memset(read_stocker_flg, 0, sizeof(read_stocker_flg));

						read_slot_flg[i] = true;
						found_slot = true;
						game_config_data.slot[i]->section = line;
						break;
					}
					tag_name[5] += 1;
				}
				if (found_slot) continue;

				tag_name = "[player1]"; // [player1...8]
				found_slot = false;
				for (int i = 0; i < GAME_SAVE_SLOT_NUM; i++) {
					if (line == tag_name) {
						// clear other flg
						read_flg = false;
						memset(read_slot_flg, 0, sizeof(read_slot_flg));
						memset(read_player_flg, 0, sizeof(read_player_flg));
						memset(read_stocker_flg, 0, sizeof(read_stocker_flg));

						read_player_flg[i] = true;
						found_slot = true;
						game_config_data.player[i]->section = line;
						break;
					}
					tag_name[7] += 1;
				}
				if (found_slot) continue;

				tag_name = "[stocker1]"; // [stocker1...8]
				found_slot = false;
				for (int i = 0; i < GAME_SAVE_SLOT_NUM; i++) {
					if (line == tag_name) {
						// clear other flg
						read_flg = false;
						memset(read_slot_flg, 0, sizeof(read_slot_flg));
						memset(read_player_flg, 0, sizeof(read_player_flg));
						memset(read_stocker_flg, 0, sizeof(read_stocker_flg));

						read_stocker_flg[i] = true;
						found_slot = true;
						game_config_data.stocker[i]->section = line;
						break;
					}
					tag_name[8] += 1;
				}
				if (found_slot) continue;
			}

			if (read_flg) {
				std::string key;
				std::string value;
				if (game_utils_split_key_value(line, key, value) == 0) {
					game_config_data.settings->values.insert(std::pair<std::string, std::string>(key, value));
				}
			}
			for (int i = 0; i < GAME_SAVE_SLOT_NUM; i++) {
				if (read_slot_flg[i]) {
					std::string key;
					std::string value;
					if (game_utils_split_key_value(line, key, value) == 0) {
						game_config_data.slot[i]->values.insert(std::pair<std::string, std::string>(key, value));
					}
					break;
				}
			}
			for (int i = 0; i < GAME_SAVE_SLOT_NUM; i++) {
				if (read_player_flg[i]) {
					std::string key;
					std::string value;
					if (game_utils_split_key_value(line, key, value) == 0) {
						game_config_data.player[i]->values.insert(std::pair<std::string, std::string>(key, value));
					}
					break;
				}
			}
			for (int i = 0; i < GAME_SAVE_SLOT_NUM; i++) {
				if (read_stocker_flg[i]) {
					std::string key;
					std::string value;
					if (game_utils_split_key_value(line, key, value) == 0) {
						game_config_data.stocker[i]->values.insert(std::pair<std::string, std::string>(key, value));
					}
					break;
				}
			}
		}
		inFile.close();
	}
	else {
		LOG_ERROR("game_save_load_ini_file %s error\n", path.c_str());
		return 1;
	}
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
	char* tmp_path = SDL_GetBasePath();
	size_t tmp_path_len;

	errno_t std_error = _dupenv_s(&tmp_path, &tmp_path_len, "USERPROFILE");
	if ((!std_error) && (tmp_path_len > 0)) {
		g_save_folder = tmp_path;
		g_save_folder = g_save_folder + "/Documents/My Games/nummc/";
		g_save_folder = game_utils_replace_string(g_save_folder, '\\', '/');

		g_save_path = g_save_folder + "save_data.ini";
		g_save_path = game_utils_replace_string(g_save_path, '\\', '/');
		//LOG_DEBUG("SavePath: %s\n", g_save_path.c_str());
		free(tmp_path);
		tmp_path = NULL;

		// check folder exist & create
		if (game_utils_create_folder(g_save_folder)) {
			return 1;
		}

		// check file exist & load
		if (_access(g_save_path.c_str(), 6) != -1) {
			if (game_save_load_ini_file(g_save_path) != 0) return 1;
		}
		else {
			std::string save_template_path = g_base_path + "data/_temp/save_data_template.ini";
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
		strcpy_s((char *)player->obj, MAX_PATH, c_path);
	}
	player->hp = atoi(game_config_data.player[slot_index]->values["hp"].c_str());
	player->exp = atoi(game_config_data.player[slot_index]->values["exp"].c_str());
	player->attack_wait_timer = atoi(game_config_data.player[slot_index]->values["attack_wait_timer"].c_str());
	player->bullet_life_timer = atoi(game_config_data.player[slot_index]->values["bullet_life_timer"].c_str());
	player->bullet_spec = atoi(game_config_data.player[slot_index]->values["bullet_spec"].c_str());

	player->speed = atoi(game_config_data.player[slot_index]->values["speed"].c_str());
	player->strength = atoi(game_config_data.player[slot_index]->values["strength"].c_str());
	player->weapon = atoi(game_config_data.player[slot_index]->values["weapon"].c_str());
	player->armor = atoi(game_config_data.player[slot_index]->values["armor"].c_str());

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
	memset(c_buff, '\0', sizeof(c_buff)); _itoa_s((player->strength), c_buff, 10); new_str = c_buff;
	game_config_data.player[slot_index]->values["strength"] = new_str;
	memset(c_buff, '\0', sizeof(c_buff)); _itoa_s((player->weapon), c_buff, 10); new_str = c_buff;
	game_config_data.player[slot_index]->values["weapon"] = new_str;
	memset(c_buff, '\0', sizeof(c_buff)); _itoa_s((player->armor), c_buff, 10); new_str = c_buff;
	game_config_data.player[slot_index]->values["armor"] = new_str;

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

int game_save_set_config_slot(int slot_index, std::string player, std::string stage, std::string timestamp)
{
	// set current timestamp
	if ((player != "") && (stage != "") && (timestamp == "")) {
		timestamp = game_utils_get_localtime();
	}

	// clear default slot
	if ((player == "") && (stage == "") && (timestamp == "")) {
		int default_slot_index;
		game_save_get_config_default_slot(&default_slot_index);
		if (default_slot_index == slot_index) {
			game_save_set_config_default_slot(-1);
		}
	}

	// save values for save menu display
	game_config_data.slot[slot_index]->values["player"] = player;
	game_config_data.slot[slot_index]->values["stage"] = stage;
	game_config_data.slot[slot_index]->values["timestamp"] = timestamp;

	if ((player == "") && (stage == "") && (timestamp == "")) {
		// don't need to clear player & stacker
		return 0;
	}

	// save from global values
	game_save_set_config_player(slot_index, &g_player);
	game_save_set_config_stocker(slot_index, &g_stocker);

	return 0;
}
