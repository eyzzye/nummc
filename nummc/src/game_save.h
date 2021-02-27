#pragma once
#include "game_common.h"
#include "unit_manager.h"
#include "inventory_manager.h"

#define GAME_SAVE_CONFIG_RESOLUTION_W_DEFAULT  1280
#define GAME_SAVE_CONFIG_RESOLUTION_H_DEFAULT   720
#define GAME_SAVE_CONFIG_MUSIC_VOLUME_DEFAULT   100
#define GAME_SAVE_CONFIG_SFX_VOLUME_DEFAULT     100

extern int g_save_folder_size;
extern char g_save_folder[GAME_FULL_PATH_MAX];

extern int game_save_init();
extern int game_save_config_save();
extern void game_save_close();
extern void game_save_get_config_default_slot(int* default_slot_index);
extern int  game_save_set_config_default_slot(int default_slot_index);
extern void game_save_get_config_unlock_stat(int* unlock_stat);
extern int game_save_set_config_unlock_stat(int unlock_stat);
extern void game_save_get_config_resolution(int* w, int* h);
extern int  game_save_set_config_resolution(int w, int h);
extern void game_save_get_config_music_volume(int* volume);
extern int  game_save_set_config_music_volume(int volume);
extern void game_save_get_config_sfx_volume(int* volume);
extern int  game_save_set_config_sfx_volume(int volume);
extern void game_save_get_config_player(int slot_index, unit_player_data_t* player);
extern int  game_save_set_config_player(int slot_index, unit_player_data_t* player);
extern void game_save_get_config_stocker(int slot_index, inventory_stocker_data_t* stocker);
extern int  game_save_set_config_stocker(int slot_index, inventory_stocker_data_t* stocker);
extern void game_save_get_config_player_backup(int slot_index);
extern void game_save_get_config_slot(int slot_index, std::string& player, std::string& stage, std::string& timestamp);
extern int  game_save_set_config_slot(int slot_index, std::string player, std::string stage, bool init_flag = false);
