#pragma once
#include <vector>
#include "game_common.h"
#include "resource_manager.h"

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

// node
extern void game_utils_node_init(node_buffer_info_t* node_buffer_info, node_data_t* node_data, int node_size, int buffer_size);
extern node_data_t* game_utils_node_new(node_buffer_info_t* node_buffer_info);
extern void game_utils_node_delete(node_data_t* node_data, node_buffer_info_t* node_buffer_info);

// char buf
#define STRCMP(_X1,_X2)    (game_utils_string_cmp((char*)_X1,(char*)_X2))
#define STRCMP_EQ(_X1,_X2) (STRCMP(_X1,_X2) == 0)

extern int game_utils_string_init();
extern char* game_utils_string_new();
extern void game_utils_string_delete(char* ptr);
extern int game_utils_string_copy(char* dst, const char* src);
extern int game_utils_string_copy_n(char* dst, const char* src, int str_size);
extern int game_utils_string_cmp(char* src1, char* src2);
extern int game_utils_string_itoa(int src, char* dst, int dst_size, int base);

// string
#if 0
extern std::string game_utils_replace_string(std::string src_str, const char old_c, const char new_c);
extern std::string game_utils_get_extention(std::string src_str);
extern std::string game_utils_get_filename(std::string path);
extern std::string game_utils_get_localtime();
extern int game_utils_split_key_value(std::string str, std::string& key, std::string& val);
extern int game_utils_expand_value(std::string str, std::string& expand_str);
extern int game_utils_split_conmma(std::string str, std::vector<int>& int_list);
extern int game_utils_split_conmma(std::string str, std::vector<std::string>& str_list);
extern int game_utils_split_colon(std::string str, std::vector<std::string>& str_list);
#else
#define GAME_UTILS_STRING_VALUE_LIST_SIZE_MAX  32
extern int game_utils_string_cat(char* dst_str, char* src_str1, char* src_str2, char* src_str3 = NULL);
extern void game_utils_replace_string(char* src_str, const char old_c, const char new_c);
extern void game_utils_get_extention(char* src_str, char* dst_str);
extern void game_utils_get_filename(char* path, char* dst_str);
extern void game_utils_get_localtime(char* dst_str, int dst_str_size);
extern int game_utils_split_key_value(char* str, char* key, char* val);
extern int game_utils_expand_value(char* str, char* expand_str);
extern int game_utils_split_conmma_int(char* str, int* int_list, int int_list_size);
extern int game_utils_split_keyword(char* str, char* str_list, int str_list_size, char c_key, int str_list_buf_size);
extern int game_utils_split_conmma(char* str, char* str_list, int str_list_size, int str_list_buf_size = GAME_UTILS_STRING_CHAR_BUF_SIZE);
extern int game_utils_split_colon(char* str, char* str_list, int str_list_size, int str_list_buf_size = GAME_UTILS_STRING_CHAR_BUF_SIZE);
#endif

// gui
extern bool game_utils_decision_internal(SDL_Rect* rect, int x, int y);

// resource
extern SDL_Texture* game_utils_render_img_tex(char* path, SDL_Color src_color, SDL_Color dst_color);
extern SDL_Texture* game_utils_render_font_tex(const std::string& message, char* fontFile, SDL_Color color, int fontSize);
extern ResourceImg* game_utils_render_number_font_tex(int number);
