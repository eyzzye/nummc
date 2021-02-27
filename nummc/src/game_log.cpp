#include <io.h>
#include <fstream>
#include "game_common.h"
#include "game_log.h"

#include "game_save.h"
#include "game_utils.h"

#ifdef GAME_LOG_ERROR_ENABLE
#define ERROR_STACK_SIZE 32
#define ERROR_STRING_SIZE 256
static char error_stack[ERROR_STACK_SIZE][ERROR_STRING_SIZE];
static int error_index;
static char error_file_path[GAME_FULL_PATH_MAX];
static char tmp_char_buf_error[ERROR_STRING_SIZE];
static std::ofstream* error_file_stream;
#endif // GAME_LOG_ERROR_ENABLE

#ifdef GAME_LOG_DEBUG_ENABLE
#define DEBUG_STACK_SIZE 32
#define DEBUG_STRING_SIZE 256
static char debug_stack[DEBUG_STACK_SIZE][DEBUG_STRING_SIZE];
static int debug_index;
static char debug_file_path[GAME_FULL_PATH_MAX];
static char tmp_char_buf_debug[DEBUG_STRING_SIZE];
static std::ofstream* debug_file_stream;
#endif // GAME_LOG_DEBUG_ENABLE

int game_log_init()
{
#ifdef GAME_LOG_ERROR_ENABLE
	error_index = 0;
	memset(error_stack, '\0', sizeof(error_stack) * sizeof(error_stack[0][0]));
	//error_file_path = g_save_folder + "error.log";
	if (game_utils_string_copy(error_file_path, g_save_folder) != 0) return 1;
	if (game_utils_string_copy(&error_file_path[g_save_folder_size], "error.log") != 0) return 1;
#ifdef GAME_LOG_FILE_ENABLE
	if (game_utils_backup_file(error_file_path)) return 1;
	error_file_stream = new std::ofstream(error_file_path, std::ostream::app);
#endif
#endif // GAME_LOG_ERROR_ENABLE

#ifdef GAME_LOG_DEBUG_ENABLE
	debug_index = 0;
	memset(debug_stack, '\0', sizeof(debug_stack) * sizeof(debug_stack[0][0]));
	//debug_file_path = g_save_folder + "debug.log";
	if (game_utils_string_copy(debug_file_path, g_save_folder) != 0) return 1;
	if (game_utils_string_copy(&debug_file_path[g_save_folder_size], "debug.log") != 0) return 1;

#ifdef GAME_LOG_FILE_ENABLE
	if (game_utils_backup_file(debug_file_path)) return 1;
	debug_file_stream = new std::ofstream(debug_file_path, std::ostream::app);
#endif
#endif // GAME_LOG_DEBUG_ENABLE

	return 0;
}

void game_log_close()
{
#ifdef GAME_LOG_ERROR_ENABLE
#ifdef GAME_LOG_FILE_ENABLE
	error_file_stream->close();
	delete error_file_stream;
	error_file_stream = NULL;
#endif
#endif // GAME_LOG_ERROR_ENABLE

#ifdef GAME_LOG_DEBUG_ENABLE
#ifdef GAME_LOG_FILE_ENABLE
	debug_file_stream->close();
	delete debug_file_stream;
	debug_file_stream = NULL;
#endif
#endif // GAME_LOG_DEBUG_ENABLE
}

#ifdef GAME_LOG_ERROR_ENABLE
void game_log_error(const char* format, ...)
{
	int log_str_size = 0;

	//log_str = game_utils_get_localtime() + tag_str; // yyyy/mm/dd_hh:mm:ss:ERROR: 
	game_utils_get_localtime(tmp_char_buf_error, sizeof(tmp_char_buf_error) - 1);
	log_str_size = (int)strlen(tmp_char_buf_error);
	game_utils_string_copy(&tmp_char_buf_error[log_str_size], ":ERROR: ");
	log_str_size = (int)strlen(tmp_char_buf_error);

	//log_str += format_str;
	va_list argptr;
	va_start(argptr, format);
	int format_str_max = (ERROR_STRING_SIZE - log_str_size - 1);
	vsnprintf_s(&tmp_char_buf_error[log_str_size], format_str_max, format_str_max, format, argptr);
	va_end(argptr);

	// console output
	log_str_size = (int)strlen(tmp_char_buf_error);
	LOG_ERROR_CONSOLE("%s", tmp_char_buf_error);

	// register stack
	game_utils_string_copy(error_stack[error_index], tmp_char_buf_error);

#ifdef GAME_LOG_FILE_ENABLE
	if (error_file_stream->is_open()) {
		error_file_stream->write(tmp_char_buf_error, log_str_size);
	}
	else {
		LOG_ERROR_CONSOLE("game_log_error write error\n");
	}
#endif

	error_index += 1;
	if (error_index >= ERROR_STACK_SIZE) error_index = 0;
}
#endif // GAME_LOG_ERROR_ENABLE

#ifdef GAME_LOG_DEBUG_ENABLE
void game_log_debug(const char* format, ...)
{
	int log_str_size = 0;

	//log_str = game_utils_get_localtime() + tag_str; // yyyy/mm/dd_hh:mm:ss:DEBUG: 
	game_utils_get_localtime(tmp_char_buf_debug, sizeof(tmp_char_buf_debug) - 1);
	log_str_size = (int)strlen(tmp_char_buf_debug);
	game_utils_string_copy(&tmp_char_buf_debug[log_str_size], ":DEBUG: ");
	log_str_size = (int)strlen(tmp_char_buf_debug);

	//log_str += format_str;
	va_list argptr;
	va_start(argptr, format);
	int format_str_max = (DEBUG_STRING_SIZE - log_str_size - 1);
	vsnprintf_s(&tmp_char_buf_debug[log_str_size], format_str_max, format_str_max, format, argptr);
	va_end(argptr);

	// console output
	log_str_size = (int)strlen(tmp_char_buf_debug);
	LOG_DEBUG_CONSOLE("%s", tmp_char_buf_debug);

	// register stack
	game_utils_string_copy(debug_stack[debug_index], tmp_char_buf_debug);

#ifdef GAME_LOG_FILE_ENABLE
	if (debug_file_stream->is_open()) {
		debug_file_stream->write(tmp_char_buf_debug, log_str_size);
	}
	else {
		LOG_ERROR_CONSOLE("game_log_debug write error\n");
	}
#endif

	debug_index += 1;
	if (debug_index >= DEBUG_STACK_SIZE) debug_index = 0;
}
#endif // GAME_LOG_DEBUG_ENABLE
