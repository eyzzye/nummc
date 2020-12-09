#pragma once
#include <vector>
#include "game_common.h"

#define GAME_UTILS_BACKUP_FILE_SIZE 2*1024*1024  // 2MB
#define GAME_UTILS_LOCALTIME_FORMAT "----/--/--_--:--:--"

// random
extern int game_utils_random_init(unsigned int seed);
extern int game_utils_random_gen(int max, int min);

// math
extern float game_utils_sin(float angle);
extern float game_utils_cos(float angle);

// file
extern std::string game_utils_upper_folder(std::string path);
extern int game_utils_create_folder(std::string path);
extern int game_utils_backup_file(std::string path, int max_size = GAME_UTILS_BACKUP_FILE_SIZE);

// string
extern std::string game_utils_replace_string(std::string src_str, const char old_c, const char new_c);
extern std::string game_utils_get_extention(std::string src_str);
extern std::string game_utils_get_filename(std::string path);
extern std::string game_utils_get_localtime();
extern int game_utils_split_key_value(std::string str, std::string& key, std::string& val);
extern int game_utils_expand_value(std::string str, std::string& expand_str);
extern int game_utils_split_conmma(std::string str, std::vector<int>& int_list);
extern int game_utils_split_conmma(std::string str, std::vector<std::string>& str_list);
extern int game_utils_split_colon(std::string str, std::vector<std::string>& str_list);

// gui
extern bool game_utils_decision_internal(SDL_Rect* rect, int x, int y);

// resource
extern SDL_Texture* game_utils_render_font_tex(const std::string& message, const std::string& fontFile, SDL_Color color, int fontSize);
extern SDL_Texture* game_utils_render_number_font_tex(int number);
