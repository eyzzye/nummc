#include "game_common.h"
#include "game_utils.h"

#include <string.h>
#include <random>
#include <time.h>
#ifdef _WIN32
#include <io.h>
#endif

#include "game_window.h"
#include "game_log.h"
#include "memory_manager.h"
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
// file helper utils
//
int game_utils_upper_folder(char* path, char* dst_str)
{
	int dst_str_size = 0;
	int path_size = (int)strlen(path);

	bool found_flg = false;
	size_t split_index = 0;

	int start_index = (path[path_size - 1] == '/') ? (path_size - 2) : (path_size - 1);
	for (int i = start_index; i >= 0; i--) {
		if (path[i] == '/') {
			split_index = (size_t)i;
			found_flg = true;
			break;
		}
	}

	// error return
	if (!found_flg) return 0;
	if (split_index < 3) return 0;

	//path.substr(0, split_index);
	int ret = game_utils_string_copy_n(dst_str, &path[0], (int)split_index);
	if (ret == 0) dst_str_size = (int)strlen(dst_str);

	return dst_str_size;
}

// path: root_folder/new_folder_path
//       root_folder ... already created, can accsess
//       new_folder_path ... already created or not
// return: create all folder(0), error(1)
int game_utils_create_folder(char* path)
{
#ifdef _WIN32
	int ret = 0;
	if (_access(path, 6) == -1) {
		char upper[256];
		int upper_size = game_utils_upper_folder(path, upper);
		if (upper_size <= 0) {
			return 1;
		}
		else {
			if (game_utils_create_folder(upper)) {
				return 1;
			}
			if (!CreateDirectoryA(path, NULL)) {
				return 1;
			}
		}
	}
#else
	int ret = 0;
	if (access(path, R_OK | W_OK) == -1) {
		char upper[256];
		int upper_size = game_utils_upper_folder(path, upper);
		if (upper_size <= 0) {
			return 1;
		}
		else {
			if (game_utils_create_folder(upper)) {
				return 1;
			}
			if (mkdir(path, 0700) != 0 && errno != EEXIST) {
				return 1;
			}
		}
	}
#endif
	return 0;
}

int game_utils_backup_file(char* path, int max_size)
{
#ifdef _WIN32
	HANDLE logFile = CreateFileA(path,
		GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL | FILE_FLAG_NO_BUFFERING, NULL);
	if (logFile == INVALID_HANDLE_VALUE) {
		if (GetLastError() == ERROR_FILE_NOT_FOUND) {
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
			//std::string backup_file_name = path + ".1";
			char backup_file_name[GAME_FULL_PATH_MAX];
			int backup_file_name_size = game_utils_string_cat(backup_file_name, path, (char*)".1");
			if ((backup_file_name_size <= 0) || (!CopyFileExA(path, backup_file_name, NULL, NULL, NULL, COPY_FILE_NO_BUFFERING)))
			{
				LOG_ERROR_WINAPI_CONSOLE("CopyFileExA Error: %s\n");
				CloseHandle(logFile);
				return 1;
			}
			else {
				CloseHandle(logFile);
				return 0;
			}
		}
		else {
			CloseHandle(logFile);
			return 0;
		}
	}
#endif
}

int game_utils_files_get_file_list(char* path, char* filter, char* file_list, int file_list_size, int file_list_line_size)
{
	int list_size = 0;

#ifdef _WIN32
	char full_filename[256];
	int flie_size = game_utils_string_cat(full_filename, path, filter);
	if (flie_size <= 0) { LOG_ERROR("Error: game_utils_files_get_file_list() failed string_cat\n"); return 0; }

	WIN32_FIND_DATAA find_file_data;
	HANDLE h_find = FindFirstFileA(full_filename, &find_file_data);
	if (h_find == INVALID_HANDLE_VALUE) {
		LOG_ERROR("game_utils_files_get_file_list FindFirstFileA() error %s\n", full_filename);
		return 0;
	}

	do {
		if (!(find_file_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
			int flie_size = game_utils_string_cat(full_filename, path, find_file_data.cFileName);
			if (flie_size > 0) {
				game_utils_string_copy(&file_list[list_size * file_list_line_size], full_filename);
				list_size++;
			}
		}
	} while (FindNextFileA(h_find, &find_file_data) != 0);
	FindClose(h_find);
#else
	char* extention = NULL;
	int extention_size = (int)strlen(filter);
	if ((extention_size >= 3) && (filter[0] == '*') && (filter[1] == '.')) {
		extention = &filter[1];
	}
	else if ((extention_size >= 2) && (filter[0] == '.')) {
		extention = &filter[0];
	}
	else {
		//extention = NULL;
	}

	char full_filename[256];
	DIR* top_dir = opendir(path);
	struct dirent* dir;
	if (top_dir != NULL) {
		while ((dir = readdir(top_dir)) != NULL) {
			if (dir->d_type != DT_DIR) {
				//realpath(dir->d_name, full_filename);

				bool register_flag = false;
				int current_ext_size = 0;
				if (extention != NULL) {
					char current_ext[32];
					current_ext_size = game_utils_get_extention(dir->d_name, current_ext);
					if ((current_ext_size > 0) && (strcmp(current_ext, extention) == 0)) {
						register_flag = true;
					}
				}
				else {
					register_flag = true;
				}

				if (register_flag) {
					int flie_size = game_utils_string_cat(full_filename, path, dir->d_name);
					if (flie_size > 0) {
						game_utils_string_copy(&file_list[list_size * file_list_line_size], full_filename);
						list_size++;
					}
				}
			}
		}
	}
	closedir(top_dir);
#endif
	return list_size;
}

//
// file write/read utils
//
static SDL_RWops* current_file;
int game_utils_files_open(char* path, const char* mode, SDL_RWops** context)
{
	current_file = SDL_RWFromFile(path, mode);
	if (current_file == NULL) {
		LOG_ERROR("Error: game_utils_files_open() failed %s\n", path);
		return 1;
	}
	*context = current_file;
	return 0;
}
void game_utils_files_set_current_file(SDL_RWops* context)
{
	current_file = context;
}
void game_utils_files_write_line(char* line, int line_size, int line_num)
{
	int ret = 0;
	int write_size = line_size;
	if (line[line_size - 1] == '\0') write_size = line_size - 1;

	if (write_size > 0) {
		ret = (int)SDL_RWwrite(current_file, line, write_size, 1);
		if (ret <= 0) {
			LOG_ERROR("Error: game_utils_files_write_line() failed\n");
			return;
		}
	}

	// write new_line
	ret = (int)SDL_RWwrite(current_file, "\n", 1, 1);
}
int game_utils_files_close(SDL_RWops* context)
{
	int ret = SDL_RWclose(context);
	if (ret != 0) {
		LOG_ERROR("Error: game_utils_files_close() failed\n");
		return 1;
	}
	return 0;
}

int game_utils_files_read_line(char* path, callback_read_line_func func, void* callback_argv)
{
	int ret = 0;

	SDL_RWops* in_file = SDL_RWFromFile(path, "r");
	if (in_file == NULL) {
		LOG_ERROR("Error: SDL_RWFromFile() failed\n");
		return 1;
	}

	// read line
	int line_num = 0;
	int line_index = 0;
	char line[256];
	char read_buf[256];
	bool find_eof = false;
	bool remain_line = false;

	while (!find_eof || remain_line) {
		int read_index = 0;

		memset(read_buf, '\0', sizeof(read_buf));
		int read_count = (int)SDL_RWread(in_file, read_buf, sizeof(read_buf) - 1, 1);
		int read_size = (int)strlen(read_buf);

		bool read_buf_end_flag = false;
		while ((!read_buf_end_flag && (read_size > 0)) || remain_line) {
			// find '\n'
			int new_line_size = 0;
			int line_end = read_index;
			for (; line_end < read_size; line_end++) {
				if (read_buf[line_end] == '\n') {
					new_line_size = 1;
					break;
				}
			}

			// copy line
			if ((new_line_size > 0) || ((new_line_size == 0) && remain_line)) {
				int line_size = 0;
				if (new_line_size > 0) {
					// read_buf[line_end-1]='\r' read_buf[line_end]='\n'
					if ((line_end > 0) && (read_buf[line_end - 1] == '\r'))
					{
						new_line_size += 1;
					}
					// line[line_index-1]='\r' read_buf[0]='\n'
					else if (remain_line && (line_index > 0) && (line[line_index - 1] == '\r') && (read_buf[0] == '\n'))
					{
						new_line_size += 1;
					}

					line_size = (line_end - read_index + 1) - new_line_size;
					if (line_size > 0) memcpy(&line[line_index], &read_buf[read_index], line_size);
					line[line_index + line_size] = '\0';
				}

				//
				// do anything
				//
				if (func != NULL) {
					func(line, (line_index + line_size), line_num, callback_argv);
				}
				//printf("test: %s\n", line);

				if (remain_line) remain_line = false;
				line_num += 1;
				line_index = 0;
				read_index = (line_end + 1);
			}

			// buf end
			else {
				int buf_end_size = read_size - read_index;
				if (buf_end_size > 0) {
					memcpy(&line[0], &read_buf[read_index], buf_end_size);
					line[buf_end_size] = '\0';
					line_index = buf_end_size;
					remain_line = true;
				}
				read_index = 0;
				read_buf_end_flag = true;
				break;
			}
		}

		if (read_size <= 0) {
			find_eof = true;
		}
	}

	ret = SDL_RWclose(in_file);
	if (ret != 0) {
		LOG_ERROR("Error: SDL_RWclose() failed\n");
		return 1;
	}

	return 0;
}

//
// node data utils
//
void game_utils_node_init(node_buffer_info_t* node_buffer_info, int node_size)
{
	node_buffer_info->node_size        = node_size;
	node_buffer_info->used_buffer_size = 0;
	node_buffer_info->start_node       = NULL;
	node_buffer_info->end_node         = NULL;
}

node_data_t* game_utils_node_new(node_buffer_info_t* node_buffer_info)
{
	node_data_t* new_node = memory_manager_new_node_buff(node_buffer_info->node_size);
	if (new_node) {
		node_buffer_info->used_buffer_size += 1;
	}
	else {
		LOG_ERROR("Error: game_utils_node_new() buffer overflow\n");
		return NULL;
	}

	if (node_buffer_info->start_node == NULL) {
		node_buffer_info->start_node = new_node;
		new_node->prev = NULL;
	}

	if (node_buffer_info->end_node != NULL) {
		node_buffer_info->end_node->next = new_node;
		new_node->prev = node_buffer_info->end_node;
		new_node->next = NULL;
		node_buffer_info->end_node = new_node;
	} else {
		node_buffer_info->end_node = new_node;
		new_node->next = NULL;
	}

	return new_node;
}

void game_utils_node_delete(node_data_t* node_data, node_buffer_info_t* node_buffer_info)
{
	node_data_t* tmp1 = node_data->prev;
	node_data_t* tmp2 = node_data->next;
	if (tmp1) tmp1->next = tmp2;
	if (tmp2) tmp2->prev = tmp1;

	// replace start node
	if (node_buffer_info->start_node == node_data) {
		node_buffer_info->start_node = tmp2;
	}
	// replace end node
	if (node_buffer_info->end_node == node_data) {
		node_buffer_info->end_node = tmp1;
	}

	node_buffer_info->used_buffer_size -= 1;
	memory_manager_delete_node_buff(node_data);
}

//
// char buffer utils
//

// return: success(0), error(1)
int game_utils_string_copy(char* dst, const char* src)
{
#ifdef _WIN32
	int src_size = (int)strlen(src);
	int ret = strcpy_s(dst, (size_t)(src_size + 1), src);
	if (ret != 0) {
		LOG_ERROR("ERROR: game_utils_string_copy() failed\n");
		return 1;
	}
	dst[src_size] = '\0';
#else
	int src_size = (int)strlen(src);
	char* ret = strcpy(dst, src);
	if (ret == NULL) {
		LOG_ERROR("ERROR: game_utils_string_copy() failed\n");
		return 1;
	}
	dst[src_size] = '\0';
#endif
	return 0;
}

// return: success(0), error(1)
int game_utils_string_copy_n(char* dst, const char* src, int str_size)
{
#ifdef _WIN32
	int ret = strncpy_s(dst, (size_t)(str_size + 1), src, str_size);
	if (ret != 0) {
		LOG_ERROR("ERROR: game_utils_string_copy(str_size) failed\n");
		return 1;
	}
	dst[str_size] = '\0';
#else
	char* ret = strncpy(dst, src, str_size);
	if (ret == NULL) {
		LOG_ERROR("ERROR: game_utils_string_copy(str_size) failed\n");
		return 1;
	}
	dst[str_size] = '\0';
#endif
	return 0;
}

// return: equal(0), src1<src2(-N), src1>src2(N)
int game_utils_string_cmp(char* src1, char* src2)
{
	return strcmp(src1, src2);
}

int game_utils_string_itoa(int src, char* dst, int dst_size, int base)
{
#if 0
	_itoa_s(src, dst, dst_size, base);
#else
	snprintf(dst, dst_size, "%d", src);
#endif
	return 0;
}

//
// string utils
//
#if 0
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

#else // char_buf utils
// return: success(dst_str_size), error(0)
int game_utils_string_cat(char* dst_str, char* src_str1, char* src_str2, char* src_str3)
{
	int ret = 0;
	int dst_str_size = 0;

	if (ret == 0) {
		ret = game_utils_string_copy(dst_str, src_str1);
		dst_str_size += (int)strlen(src_str1);
	}
	if (ret == 0) {
		ret = game_utils_string_copy(&dst_str[dst_str_size], src_str2);
		dst_str_size += (int)strlen(src_str2);
	}
	if ((ret == 0) && (src_str3 != NULL)) {
		ret = game_utils_string_copy(&dst_str[dst_str_size], src_str3);
		dst_str_size += (int)strlen(src_str3);
	}
	if (ret != 0) {
		LOG_ERROR("game_utils_string_cat failed %s, %s, %s\n", src_str1, src_str2, src_str3);
		return 0;
	}
	return dst_str_size;
}

// replace src_str
void game_utils_replace_string(char* src_str, const char old_c, const char new_c)
{
	int i = 0;
	while (src_str[i] != '\0') {
		src_str[i] = (src_str[i] == old_c) ? new_c : src_str[i];
		i++;
	}
	return;
}

// return: success(strlen(dst_str)), error(0)
int game_utils_get_extention(char* src_str, char* dst_str)
{
	int dst_str_size = 0;
	int src_str_size = (int)strlen(src_str);

	bool found_flg = false;
	size_t head_index = 0;
	for (int i = src_str_size - 1; i >= 0; i--) {
		if (src_str[i] == '.') {
			head_index = (size_t)i;
			found_flg = true;
			break;
		}
		else if (src_str[i] == '/') {
			break;
		}
	}

	if (found_flg) {
		size_t new_size = src_str_size - head_index;
		size_t ext_i = 0;
		for (size_t i = head_index; i < src_str_size; i++, ext_i++) {
			dst_str[ext_i] = src_str[i];
		}
		dst_str[ext_i] = '\0';
		dst_str_size = (int)strlen(dst_str);
	}

	return dst_str_size;
}

// return: success(strlen(dst_str)), error(0)
int game_utils_get_filename(char* src_str, char* dst_str)
{
	int dst_str_size = 0;
	int src_str_size = (int)strlen(src_str);

	bool found_flg = false;
	int start_index = src_str_size - 1;
	size_t split_index = 0;
	size_t extention_index = start_index;

	for (int i = start_index; i >= 0; i--) {
		if (src_str[i] == '.') {
			extention_index = (size_t)i;
		}
		else if (src_str[i] == '/') {
			split_index = (size_t)i;
			found_flg = true;
			break;
		}
	}

	// error return
	if (!found_flg) return 0;
	if (split_index < 3) return 0;

	int ret = game_utils_string_copy_n(dst_str, &src_str[split_index + 1], (int)(extention_index - (split_index + 1)));
	if (ret == 0) dst_str_size = (int)strlen(dst_str);

	return dst_str_size;
}

void game_utils_get_localtime(char* dst_str, int dst_str_size)
{
	struct tm current_localtime;
	time_t current_time = time(NULL);

#ifdef _WIN32
	localtime_s(&current_localtime, &current_time);
#else
	localtime_r(&current_time, &current_localtime);
#endif
	if (strftime(dst_str, dst_str_size, "%Y/%m/%d_%H:%M:%S", &current_localtime) == 0) {
		game_utils_string_copy(dst_str, GAME_UTILS_LOCALTIME_FORMAT);
	}

	dst_str[strlen(GAME_UTILS_LOCALTIME_FORMAT)] = '\0';
	return;
}

int game_utils_split_key_value(char* str, char* key, char* val)
{
	int str_size = (int)strlen(str);
	if (str_size == 0) return 1;

	// search =
	int head_index = -1;
	for (int i = 0; i < str_size; i++) {
		if (str[i] == '=') {
			head_index = i;
			break;
		}
	}

	// not found
	if (head_index == -1) {
		key[0] = '\0';
		val[0] = '\0';
		return 1;
	}

	// key
	size_t length = str_size - ((size_t)head_index + 1);
	if (head_index == 0) key[0] = '\0';
	else game_utils_string_copy_n(key, &str[0], head_index);

	// value
	if (length <= 0) val[0] = '\0';
	else game_utils_string_copy_n(val, &str[head_index + 1], (int)length);

	return 0;
}

// return: expand_str size or error(-1)
#define EXPAND_NUM_STR_SIZE  4
int game_utils_expand_value(char* str, char* expand_str)
{
	int str_size = (int)strlen(str);
	if (str_size == 0) return 0;

	// local char buf
	char prefix[MEMORY_MANAGER_NAME_BUF_SIZE];
	char extention[MEMORY_MANAGER_NAME_BUF_SIZE];
	char expand_num_str[EXPAND_NUM_STR_SIZE];

	// search ,.*[.+].*,
	int comma_list[16];  int comma_list_size = 0;
	int head_list[16];   int head_list_size = 0;
	int end_list[16];    int end_list_size = 0;
	int hyphen_list[16]; int hyphen_list_size = 0;
	for (int i = 0; i < str_size; i++) {
		if (str[i] == ',') { comma_list[comma_list_size] = i; comma_list_size++; }
		else if (str[i] == '[') { head_list[head_list_size] = i;  head_list_size++; }
		else if (str[i] == '-') { hyphen_list[hyphen_list_size] = i;  hyphen_list_size++; }
		else if (str[i] == ']') { end_list[end_list_size] = i;  end_list_size++; }
	}

	// not found bracket
	if (head_list_size == 0) {
		return 0;
	}

	// exist irregular set
	if (head_list_size != end_list_size) {
		printf("expand_value Error: not match bracket count \n");
		return -1;
	}

	// expand [] set
	expand_str[0] = '\0';
	int expand_str_index = 0;
	int start_index, end_index;
	int hyphen_cursor = 0;
	int latest_end_index = 0;
	int length = 0;
	for (int i = 0; i < head_list_size; i++) {
		// error check
		if (head_list[i] >= end_list[i]) {
			printf("expand_value Error: not match bracket position \n");
			return -1;
		}

		// split [n1-n2]
		bool hyphen_found = false;
		int start_num = 0;
		int end_num = 0;
		for (; hyphen_cursor < hyphen_list_size; hyphen_cursor++) {
			if ((hyphen_list[hyphen_cursor] > head_list[i]) && (hyphen_list[hyphen_cursor] < end_list[i])) {
				char tmp_char_buf[MEMORY_MANAGER_NAME_BUF_SIZE];

				// get first number
				int length = (hyphen_list[hyphen_cursor] - 1) - (head_list[i] + 1) + 1;
				game_utils_string_copy_n(tmp_char_buf, &str[head_list[i] + 1], length);
				start_num = atoi(tmp_char_buf);

				// get second number
				length = (end_list[i] - 1) - (hyphen_list[hyphen_cursor] + 1) + 1;
				game_utils_string_copy_n(tmp_char_buf, &str[hyphen_list[hyphen_cursor] + 1], length);
				end_num = atoi(tmp_char_buf);

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
		for (int comma_list_i = 0; comma_list_i < comma_list_size; comma_list_i++) {
			if (comma_list[comma_list_i] > end_list[i]) {
				end_index = comma_list[comma_list_i];
				break;
			}
			start_index = comma_list[comma_list_i];
		}

		bool start_comma_none = false;
		bool end_comma_none = false;
		// start comma
		if (str[0] != ',') {
			start_comma_none = true;
		}
		// end comma
		if (end_index == 0) {
			end_index = str_size - 1;
			end_comma_none = true;
		}

		// copy comma before  ("a.png,b[1-4].png,c.png" => ""a.png")
		int length = start_index - latest_end_index;
		if (length > 0) {
			game_utils_string_copy_n(&expand_str[expand_str_index], &str[latest_end_index], length);
			expand_str_index += length;
		}

		// get prefix "(,.*)["
		if (start_comma_none) {
			length = (head_list[i] - 1) - start_index + 1;
			if (length > 0) game_utils_string_copy_n(prefix, &str[start_index], length);
		}
		else {
			length = (head_list[i] - 1) - (start_index + 1) + 1;
			if (length > 0) game_utils_string_copy_n(prefix, &str[start_index + 1], length);
		}

		// get extention "](.*),"
		if (end_comma_none) {
			length = end_index - (end_list[i] + 1) + 1;
			if (length > 0) game_utils_string_copy_n(extention, &str[end_list[i] + 1], length);
		}
		else {
			length = (end_index - 1) - (end_list[i] + 1) + 1;
			if (length > 0) game_utils_string_copy_n(extention, &str[end_list[i] + 1], length);
		}

		// copy expand section (",b[1-4].png," => ",b1.png,b2png,b3.png,b4.png")
		for (int e_i = start_num; e_i <= end_num; e_i++) {
			game_utils_string_itoa(e_i, expand_num_str, (EXPAND_NUM_STR_SIZE - 1), 10 /* decimal */);
			if ((e_i == start_num) && start_comma_none) {
				//expand_str;
			}
			else {
				//expand_str += ",";
				game_utils_string_copy(&expand_str[expand_str_index], ",");
				expand_str_index += 1;
			}

			//expand_str += prefix + expand_num_str + extention;
			length = (int)strlen(prefix);
			if (length > 0) {
				game_utils_string_copy(&expand_str[expand_str_index], prefix);
				expand_str_index += length;
			}

			length = (int)strlen(expand_num_str);
			if (length > 0) {
				game_utils_string_copy(&expand_str[expand_str_index], expand_num_str);
				expand_str_index += length;
			}

			length = (int)strlen(extention);
			if (length > 0) {
				game_utils_string_copy(&expand_str[expand_str_index], extention);
				expand_str_index += length;
			}
		}

		latest_end_index = end_index;
	}

	// copy remain section (",c.png" => ",c.png")
	length = ((int)str_size - 1) - latest_end_index;
	if (length > 0) {
		game_utils_string_copy_n(&expand_str[expand_str_index], &str[latest_end_index], length + 1);
		expand_str_index += length;
	}

	return expand_str_index;
}

// return: split list size
int game_utils_split_conmma_int(char* str, int* int_list, int int_list_size)
{
	int str_size = (int)strlen(str);
	if (str_size == 0) return 0;

	int list_size = 0;
	size_t pre_end = 0;
	for (size_t i = 0; i < str_size; i++) {
		char tmp_char_buf[MEMORY_MANAGER_NAME_BUF_SIZE];

		if (str[i] == ',') {
			game_utils_string_copy_n(tmp_char_buf, &str[pre_end], (int)(i - pre_end));
			pre_end = i + 1;
			int_list[list_size] = atoi(tmp_char_buf);
			list_size++;
		}
		else if (i == (str_size - 1)) {
			game_utils_string_copy_n(tmp_char_buf, &str[pre_end], (int)((i + 1) - pre_end));
			pre_end = i + 1;
			int_list[list_size] = atoi(tmp_char_buf);
			list_size++;
		}
	}
	return list_size;
}

// return: split list size
int game_utils_split_keyword(char* str, char* str_list, int str_list_size, char c_key, int str_list_buf_size)
{
	int str_size = (int)strlen(str);
	if (str_size == 0) return 0;

	int list_size = 0;
	size_t pre_end = 0;
	for (size_t i = 0; i < str_size; i++) {
		if (list_size >= str_list_size) {
			LOG_ERROR("Error: game_utils_split_keyword() buffer overflow\n");
			break;
		}

		if (str[i] == c_key) {
			game_utils_string_copy_n(&str_list[list_size * str_list_buf_size], &str[pre_end], (int)(i - pre_end));
			pre_end = i + 1;
			list_size++;
		}
		else if (i == (str_size - 1)) {
			game_utils_string_copy_n(&str_list[list_size * str_list_buf_size], &str[pre_end], (int)((i + 1) - pre_end));
			pre_end = i + 1;
			list_size++;
		}
	}

	if (list_size == 0) {
		// copy all
		game_utils_string_copy(str_list, str);
		list_size++;
	}

	return list_size;
}

int game_utils_split_conmma(char* str, char* str_list, int str_list_size, int str_list_buf_size)
{
	return game_utils_split_keyword(str, str_list, str_list_size, ',', str_list_buf_size);
}

int game_utils_split_colon(char* str, char* str_list, int str_list_size, int str_list_buf_size)
{
	return game_utils_split_keyword(str, str_list, str_list_size, ':', str_list_buf_size);
}
#endif

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
SDL_Texture* game_utils_render_img_tex(char* path, SDL_Color src_color, SDL_Color dst_color)
{
	SDL_Surface* surf = IMG_Load(path);

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
SDL_Texture* game_utils_render_font_tex(const char* message, char* fontFile, SDL_Color color, int fontSize)
{
	TTF_Font* font = TTF_OpenFont(fontFile, fontSize);
	if (font == NULL) {
		LOG_ERROR("TTF_OpenFont Error: %s", SDL_GetError());
		return NULL;
	}

	//SDL_Surface* surf = TTF_RenderText_Blended(font, message.c_str(), color);
	SDL_Surface* surf = TTF_RenderUTF8_Blended(font, message, color);
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

ResourceImg* game_utils_render_number_font_tex(int number)
{
	ResourceImg* res_img = NULL;
	switch (number) {
		case 0:
			res_img = resource_manager_getTextureFromPath("images/gui/font/0.png");
			break;
		case 1:
			res_img = resource_manager_getTextureFromPath("images/gui/font/1.png");
			break;
		case 2:
			res_img = resource_manager_getTextureFromPath("images/gui/font/2.png");
			break;
		case 3:
			res_img = resource_manager_getTextureFromPath("images/gui/font/3.png");
			break;
		case 4:
			res_img = resource_manager_getTextureFromPath("images/gui/font/4.png");
			break;
		case 5:
			res_img = resource_manager_getTextureFromPath("images/gui/font/5.png");
			break;
		case 6:
			res_img = resource_manager_getTextureFromPath("images/gui/font/6.png");
			break;
		case 7:
			res_img = resource_manager_getTextureFromPath("images/gui/font/7.png");
			break;
		case 8:
			res_img = resource_manager_getTextureFromPath("images/gui/font/8.png");
			break;
		case 9:
			res_img = resource_manager_getTextureFromPath("images/gui/font/9.png");
			break;
		default:
			break;
	}
	return res_img;
}
