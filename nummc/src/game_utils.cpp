#include <string.h>
#include <random>
#include <io.h>
#include <time.h>
#include <fstream>
#include "game_common.h"
#include "game_utils.h"
#include "game_window.h"
#include "game_log.h"
#include "resource_manager.h"

//
// random utils
//
int game_utils_random_init(unsigned int seed)
{
	srand(seed);
	return 0;
}

int game_utils_random_gen(int max, int min)
{
	int rand_val = rand();
	int val = rand_val % (max + 1 - min) + min;
	return val;
}

//
// math utils
//
float game_utils_sin(float angle)
{
	float sin_val = sin(angle);
	if (ABS(sin_val) < FLOAT_NEAR_ZERO) {
		sin_val = 0;
	}
	else if ((sin_val > 0) && ((1.0f - FLOAT_NEAR_ZERO) < sin_val) && (sin_val < 1.0f + FLOAT_NEAR_ZERO)) {
		sin_val = 1.0f;
	}
	else if ((sin_val < 0) && ((-1.0f - FLOAT_NEAR_ZERO) < sin_val) && (sin_val < -1.0f + FLOAT_NEAR_ZERO)) {
		sin_val = -1.0f;
	}
	return sin_val;
}

float game_utils_cos(float angle)
{
	float cos_val = cos(angle);
	if (ABS(cos_val) < FLOAT_NEAR_ZERO) {
		cos_val = 0;
	}
	else if ((cos_val > 0) && ((1.0f - FLOAT_NEAR_ZERO) < cos_val) && (cos_val < 1.0f + FLOAT_NEAR_ZERO)) {
		cos_val = 1.0f;
	}
	else if ((cos_val < 0) && ((-1.0f - FLOAT_NEAR_ZERO) < cos_val) && (cos_val < -1.0f + FLOAT_NEAR_ZERO)) {
		cos_val = -1.0f;
	}
	return cos_val;
}

//
// file utils
//
std::string game_utils_upper_folder(std::string path)
{
	bool found_flg = false;
	size_t split_index = 0;

	int start_index = (path[path.size()-1] == '/') ? ((int)path.size() - 2) : ((int)path.size() - 1);
	for (int i = start_index; i >= 0; i--) {
		if (path[i] == '/') {
			split_index = (size_t)i;
			found_flg = true;
			break;
		}
	}

	// error return
	if (!found_flg) return "";
	if (split_index < 3) return "";

	return path.substr(0, split_index);
}

int game_utils_create_folder(std::string path)
{
	int ret = 0;
	if (_access(path.c_str(), 6) == -1) {
		std::string upper = game_utils_upper_folder(path);
		if (upper == "") {
			return 1;
		}
		else {
			if (game_utils_create_folder(upper)) {
				return 1;
			}
			if (!CreateDirectoryA(path.c_str(), NULL)) {
				return 1;
			}
		}
	}
	return 0;
}

int game_utils_backup_file(std::string path, int max_size)
{
	HANDLE logFile = CreateFileA(path.c_str(),
		GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL | FILE_FLAG_NO_BUFFERING, NULL);
	if (logFile == INVALID_HANDLE_VALUE) {
		if (GetLastError() == ERROR_FILE_NOT_FOUND) {
			// new create
			std::ofstream newFile(path, std::ostream::out);
			newFile.close();
			return 0;
		}
		else {
			LOG_ERROR_WINAPI_CONSOLE("CreateFileA Error: %s\n");
			return 1;
		}
	}
	else {
		LARGE_INTEGER file_size;
		if (!GetFileSizeEx(logFile, &file_size)) {
			LOG_ERROR_WINAPI_CONSOLE("GetFileSizeEx Error: %s\n");
			CloseHandle(logFile);
			return 1;
		}
		else if (file_size.QuadPart > max_size) {
			std::string backup_file_name = path + ".1";
			if (!CopyFileExA(path.c_str(), backup_file_name.c_str(), NULL, NULL, NULL, COPY_FILE_NO_BUFFERING))
			{
				LOG_ERROR_WINAPI_CONSOLE("CopyFileExA Error: %s\n");
				CloseHandle(logFile);
				return 1;
			}
			else {
				CloseHandle(logFile);

				// new create
				std::ofstream newFile(path, std::ostream::out);
				newFile.close();
				return 0;
			}
		}
		else {
			CloseHandle(logFile);
			return 0;
		}
	}
}

//
// char buffer utils
//
#define GAME_UTILS_STRING_SIZE  (256 * 8)  /* *.unit, SPAWN.anim, IDLE.anim, MOVE.anim, DIE.anim, ATTACK1.anim, ATTACK2.anim, *.unit->next_level */
static game_utils_string_t game_utils_string[GAME_UTILS_STRING_SIZE];
static int game_utils_string_index_end;

int game_utils_string_init()
{
	memset(game_utils_string, 0, sizeof(game_utils_string));
	game_utils_string_index_end = 0;
	return 0;
}

char* game_utils_string_new()
{
	int ret = -1;

	int index = game_utils_string_index_end;
	for (int i = 0; i < GAME_UTILS_STRING_SIZE; i++) {
		if (index >= GAME_UTILS_STRING_SIZE) index -= GAME_UTILS_STRING_SIZE;
		if (game_utils_string[index].type == GAME_UTILS_STRING_TYPE_NONE) {
			ret = index;
			break;
		}
		index++;
	}

	if (ret == -1) {
		LOG_ERROR("Error: game_utils_string_new() overflow.");
		return NULL;
	}

	game_utils_string[index].type = GAME_UTILS_STRING_TYPE_CHAR_BUF;
	memset(game_utils_string[index].buffer, 0, sizeof(char) * GAME_UTILS_STRING_CHAR_BUF_SIZE);
	game_utils_string_index_end = index + 1;

	if (game_utils_string_index_end >= GAME_UTILS_STRING_SIZE) {
		game_utils_string_index_end = 0;
	}
	return game_utils_string[index].buffer;
}

void game_utils_string_delete(char* ptr)
{
	size_t head_index = (size_t)game_utils_string[0].buffer;
	size_t delete_index = (size_t)ptr;
	int index = (int)((delete_index - head_index) / sizeof(game_utils_string_t));

	if (game_utils_string[index].buffer != ptr) {
		LOG_ERROR("ERROR: game_utils_string_delete() address is invalid\n");
		return;
	}
	game_utils_string[index].type = GAME_UTILS_STRING_TYPE_NONE;
}

int game_utils_string_copy(char* dst, const char* src)
{
	int ret = strcpy_s(dst, GAME_UTILS_STRING_CHAR_BUF_SIZE - 1, src);
	if (ret != 0) {
		LOG_ERROR("ERROR: game_utils_string_copy() failed\n");
		return 1;
	}
	return 0;
}

//
// string utils
//
std::string game_utils_replace_string(std::string src_str, const char old_c, const char new_c)
{
	std::string new_str;
	new_str.resize(src_str.size(), '\0');
	for (int i = 0; i < src_str.size(); i++) {
		new_str[i] = (src_str[i] == old_c) ? new_c : src_str[i];
	}
	return new_str;
}

std::string game_utils_get_extention(std::string src_str)
{
	std::string ext_str = "";
	bool found_flg = false;
	size_t head_index = 0;
	for (int i = (int)src_str.size() - 1; i >= 0; i--) {
		if (src_str[i] == '.') {
			head_index = (size_t)i;
			found_flg = true;
			break;
		} else if (src_str[i] == '/') {
			break;
		}
	}

	if (found_flg) {
		size_t new_size = src_str.size() - head_index;
		ext_str.resize(new_size, '\0');
		size_t ext_i = 0;
		for (size_t i = head_index; i < src_str.size(); i++, ext_i++) {
			ext_str[ext_i] = src_str[i];
		}
	}
	return ext_str;
}
std::string game_utils_get_filename(std::string path)
{
	bool found_flg = false;
	int start_index = (int)path.size() - 1;
	size_t split_index = 0;
	size_t extention_index = start_index;

	for (int i = start_index; i >= 0; i--) {
		if (path[i] == '.') {
			extention_index = (size_t)i;
		}
		else if (path[i] == '/') {
			split_index = (size_t)i;
			found_flg = true;
			break;
		}
	}

	// error return
	if (!found_flg) return "";
	if (split_index < 3) return "";

	return path.substr(split_index + 1, extention_index - (split_index + 1));
}
std::string game_utils_get_localtime()
{
	struct tm current_localtime;
	char time_str[20] = { '\0' };
	time_t current_time = time(NULL);

	localtime_s(&current_localtime, &current_time);
	if (strftime(time_str, sizeof(time_str), "%Y/%m/%d_%H:%M:%S", &current_localtime) == 0) return GAME_UTILS_LOCALTIME_FORMAT;
	return time_str;
}

int game_utils_split_key_value(std::string str, std::string& key, std::string& val)
{
	if (str.length() == 0) return 1;

	// search =
	int head_index = -1;
	for (int i = 0; i < str.length(); i++) {
		if (str[i] == '=') {
			head_index = i;
			break;
		}
	}

	// not found
	if (head_index == -1) {
		key = "";
		val = "";
		return 1;
	}

	size_t length = str.length() - ((size_t)head_index + 1);
	key = (head_index == 0) ? "" : str.substr(0, head_index);
	val = (length <= 0) ? "" : str.substr((size_t)head_index + 1, length);
	return 0;
}

int game_utils_expand_value(std::string str, std::string& expand_str)
{
	if (str.length() == 0) return 0;

	// search ,.*[.+].*,
	std::vector<int> comma_list;
	std::vector<int> head_list;
	std::vector<int> end_list;
	std::vector<int> hyphen_list;
	for (int i = 0; i < str.length(); i++) {
		if (str[i] == ',') comma_list.push_back(i);
		else if (str[i] == '[') head_list.push_back(i);
		else if (str[i] == '-') hyphen_list.push_back(i);
		else if (str[i] == ']') end_list.push_back(i);
	}

	// not found bracket
	if (head_list.size() == 0) {
		expand_str = str;
		return 0;
	}

	// exist irregular set
	if (head_list.size() != end_list.size()) {
		printf("expand_value Error: not match bracket count \n");
		return -1;
	}

	// expand [] set
	expand_str = "";
	int start_index, end_index;
	int hyphen_cursor = 0;
	int latest_end_index = 0;
	int length = 0;
	for (int i = 0; i < head_list.size(); i++) {
		// error check
		if (head_list[i] >= end_list[i]) {
			printf("expand_value Error: not match bracket position \n");
			return -1;
		}

		// split [n1-n2]
		bool hyphen_found = false;
		int start_num = 0;
		int end_num = 0;
		for (; hyphen_cursor < hyphen_list.size(); hyphen_cursor++) {
			if ((hyphen_list[hyphen_cursor] > head_list[i]) && (hyphen_list[hyphen_cursor] < end_list[i])) {
				// get first number
				int length = (hyphen_list[hyphen_cursor] - 1) - (head_list[i] + 1) + 1;
				std::string first = str.substr(head_list[i] + 1, length);
				start_num = atoi(first.c_str());

				// get second number
				length = (end_list[i] - 1) - (hyphen_list[hyphen_cursor] + 1) + 1;
				std::string second = str.substr(hyphen_list[hyphen_cursor] + 1, length);
				end_num = atoi(second.c_str());

				hyphen_found = true;
				break;
			}
		}

		// not found hyphen
		if (!hyphen_found) {
			printf("expand_value Error: not found hyphen \n");
			return -1;
		}

		// get comma set
		start_index = 0; end_index = 0;
		for (int comma_index : comma_list) {
			if (comma_index > end_list[i]) {
				end_index = comma_index;
				break;
			}
			start_index = comma_index;
		}

		bool start_comma_none = false;
		bool end_comma_none = false;
		// start comma
		if (str.substr(0, 1) != ",") {
			start_comma_none = true;
		}
		// end comma
		if (end_index == 0) {
			end_index = (int)str.length() - 1;
			end_comma_none = true;
		}

		// copy comma before  ("a.png,b[1-4].png,c.png" => ""a.png")
		int length = start_index - latest_end_index;
		if (length > 0) {
			expand_str = expand_str + str.substr(latest_end_index, length);
		}

		// get prefix "(,.*)["
		std::string prefix = "";
		if (start_comma_none) {
			length = (head_list[i] - 1) - start_index + 1;
			if (length > 0) prefix = str.substr(start_index, length);
		}
		else {
			length = (head_list[i] - 1) - (start_index + 1) + 1;
			if (length > 0) prefix = str.substr(start_index + 1, length);
		}

		// get extention "](.*),"
		std::string extention = "";
		if (end_comma_none) {
			length = end_index - (end_list[i] + 1) + 1;
			if (length > 0) extention = str.substr(end_list[i] + 1, length);
		}
		else {
			length = (end_index - 1) - (end_list[i] + 1) + 1;
			if (length > 0) extention = str.substr(end_list[i] + 1, length);
		}

		// copy expand section (",b[1-4].png," => ",b1.png,b2png,b3.png,b4.png")
		for (int e_i = start_num; e_i <= end_num; e_i++) {
			char buff_c[4] = { '\0', '\0', '\0', '\0' }; // 3Œ…+NULL•¶Žš
			if (_itoa_s(e_i, buff_c, 10) == 0) {
				std::string expand_num_str = buff_c;
				if ((e_i == start_num) && start_comma_none) {
					expand_str += prefix + expand_num_str + extention;
				}
				else {
					expand_str += "," + prefix + expand_num_str + extention;
				}
			}
		}

		latest_end_index = end_index;
	}

	// copy remain section (",c.png" => ",c.png")
	length = ((int)str.length() - 1) - latest_end_index;
	if (length > 0) {
		expand_str += str.substr(latest_end_index, length + 1);
	}

	return (int)expand_str.length();
}

int game_utils_split_conmma(std::string str, std::vector<int>& int_list)
{
	if (str.length() == 0) return 1;

	size_t pre_end = 0;
	for (size_t i = 0; i < str.size(); i++) {
		if (str[i] == ',') {
			std::string tmp = str.substr(pre_end, i - pre_end);
			pre_end = i + 1;
			int_list.push_back(atoi(tmp.c_str()));
		}
		else if (i == (str.size() - 1)) {
			std::string tmp = str.substr(pre_end, (i + 1) - pre_end);
			pre_end = i + 1;
			int_list.push_back(atoi(tmp.c_str()));
		}
	}
	return 0;
}

int game_utils_split_keyword(std::string str, std::vector<std::string>& str_list, char c_key)
{
	if (str.length() == 0) return 1;

	size_t pre_end = 0;
	for (size_t i = 0; i < str.size(); i++) {
		if (str[i] == c_key) {
			std::string tmp = str.substr(pre_end, i - pre_end);
			pre_end = i + 1;
			str_list.push_back(tmp);
		}
		else if (i == (str.size() - 1)) {
			std::string tmp = str.substr(pre_end, (i + 1) - pre_end);
			pre_end = i + 1;
			str_list.push_back(tmp);
		}
	}
	return 0;
}

int game_utils_split_conmma(std::string str, std::vector<std::string>& str_list)
{
	return game_utils_split_keyword(str, str_list, ',');
}

int game_utils_split_colon(std::string str, std::vector<std::string>& str_list)
{
	return game_utils_split_keyword(str, str_list, ':');
}

//
// gui utils
//
bool game_utils_decision_internal(SDL_Rect* rect, int x, int y)
{
	bool internal_x = false;
	bool internal_y = false;

	if ((rect->x <= x) && (x <= rect->x + rect->w)) {
		internal_x = true;
	}
	if ((rect->y <= y) && (y <= rect->y + rect->h)) {
		internal_y = true;
	}
	return internal_x && internal_y;
}

//
// image render utils
//
SDL_Texture* game_utils_render_img_tex(const std::string& path, SDL_Color src_color, SDL_Color dst_color)
{
	SDL_Surface* surf = IMG_Load(path.c_str());

	if (SDL_LockSurface(surf) == 0) {
		bool format_check = true;
		int r_idx, g_idx, b_idx;
		if (surf->format->format == SDL_PIXELFORMAT_RGBA32) {
			r_idx = 0;
			g_idx = 1;
			b_idx = 2;
		}
		else if (surf->format->format == SDL_PIXELFORMAT_ABGR32) {
			r_idx = 3;
			g_idx = 2;
			b_idx = 1;
		}
		else if (surf->format->format == SDL_PIXELFORMAT_BGRA32) {
			r_idx = 2;
			g_idx = 1;
			b_idx = 0;
		}
		else if (surf->format->format == SDL_PIXELFORMAT_ARGB32) {
			r_idx = 1;
			g_idx = 2;
			b_idx = 3;
		}
		else {
			LOG_ERROR("Error: game_utils_render_img_tex non-support format\n");
			format_check = false;
		}

		if (format_check) {
			char* pixel = (char*)surf->pixels;
			for (int h = 0; h < surf->h; h++) {
				for (int w = 0; w < surf->w; w++) {
					if (*(pixel + r_idx) == src_color.r && *(pixel + g_idx) == src_color.g && *(pixel + b_idx) == src_color.b) {
						*(pixel + r_idx) = dst_color.r;
						*(pixel + g_idx) = dst_color.g;
						*(pixel + b_idx) = dst_color.b;
					}
					pixel += 4;
				}
			}
		}
		SDL_UnlockSurface(surf);
	}

	SDL_Texture* texture = SDL_CreateTextureFromSurface(g_ren, surf);
	if (texture == NULL) {
		LOG_ERROR("SDL_CreateTextureFromSurface Error: %s", SDL_GetError());
	}

	SDL_FreeSurface(surf);
	return texture;
}

//
// font render utils
//
SDL_Texture* game_utils_render_font_tex(const std::string& message, const std::string& fontFile, SDL_Color color, int fontSize)
{
	TTF_Font* font = TTF_OpenFont(fontFile.c_str(), fontSize);
	if (font == NULL) {
		LOG_ERROR("TTF_OpenFont Error: %s", SDL_GetError());
		return NULL;
	}

	//SDL_Surface* surf = TTF_RenderText_Blended(font, message.c_str(), color);
	SDL_Surface* surf = TTF_RenderUTF8_Blended(font, message.c_str(), color);
	if (surf == NULL) {
		TTF_CloseFont(font);
		LOG_ERROR("TTF_RenderText_Blended Error: %s", SDL_GetError());
		return NULL;
	}

	SDL_Texture* texture = SDL_CreateTextureFromSurface(g_ren, surf);
	if (texture == NULL) {
		LOG_ERROR("SDL_CreateTextureFromSurface Error: %s", SDL_GetError());
	}

	SDL_FreeSurface(surf);
	TTF_CloseFont(font);
	return texture;
}

SDL_Texture* game_utils_render_number_font_tex(int number)
{
	SDL_Texture* texture = NULL;
	switch (number) {
		case 0:
			texture = resource_manager_getTextureFromPath("images/gui/font/0.png");
			break;
		case 1:
			texture = resource_manager_getTextureFromPath("images/gui/font/1.png");
			break;
		case 2:
			texture = resource_manager_getTextureFromPath("images/gui/font/2.png");
			break;
		case 3:
			texture = resource_manager_getTextureFromPath("images/gui/font/3.png");
			break;
		case 4:
			texture = resource_manager_getTextureFromPath("images/gui/font/4.png");
			break;
		case 5:
			texture = resource_manager_getTextureFromPath("images/gui/font/5.png");
			break;
		case 6:
			texture = resource_manager_getTextureFromPath("images/gui/font/6.png");
			break;
		case 7:
			texture = resource_manager_getTextureFromPath("images/gui/font/7.png");
			break;
		case 8:
			texture = resource_manager_getTextureFromPath("images/gui/font/8.png");
			break;
		case 9:
			texture = resource_manager_getTextureFromPath("images/gui/font/9.png");
			break;
		default:
			break;
	}
	return texture;
}
