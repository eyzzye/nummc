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
static std::string error_file_path;
static std::ofstream* error_file_stream;
#endif // GAME_LOG_ERROR_ENABLE

#ifdef GAME_LOG_DEBUG_ENABLE
#define DEBUG_STACK_SIZE 32
#define DEBUG_STRING_SIZE 256
static char debug_stack[DEBUG_STACK_SIZE][DEBUG_STRING_SIZE];
static int debug_index;
static std::string debug_file_path;
static std::ofstream* debug_file_stream;
#endif // GAME_LOG_DEBUG_ENABLE

int game_log_init()
{
#ifdef GAME_LOG_ERROR_ENABLE
	error_index = 0;
	memset(error_stack, '\0', sizeof(error_stack) * sizeof(error_stack[0][0]));
	error_file_path = g_save_folder + "error.log";
#ifdef GAME_LOG_FILE_ENABLE
	if (game_utils_backup_file(error_file_path)) return 1;
	error_file_stream = new std::ofstream(error_file_path, std::ostream::app);
#endif
#endif // GAME_LOG_ERROR_ENABLE

#ifdef GAME_LOG_DEBUG_ENABLE
	debug_index = 0;
	memset(debug_stack, '\0', sizeof(debug_stack) * sizeof(debug_stack[0][0]));
	debug_file_path = g_save_folder + "debug.log";

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
	std::string tag_str = ":ERROR: ";
	size_t format_str_size = strlen(GAME_UTILS_LOCALTIME_FORMAT) + tag_str.size();

	char buff[ERROR_STRING_SIZE] = { '\0' };
	va_list argptr;
	va_start(argptr, format);
	vsnprintf_s(buff, (ERROR_STRING_SIZE - format_str_size - 1), format, argptr);
	va_end(argptr);

	std::string log_str = buff;
	log_str = game_utils_get_localtime() + tag_str + log_str; // yyyy/mm/dd_hh:mm:ss:ERROR: format_str
	size_t log_str_size = log_str.size();
	LOG_ERROR_CONSOLE("%s", log_str.c_str());

	memcpy(error_stack[error_index], log_str.c_str(), log_str_size);
	error_stack[error_index][log_str_size] = '\0';

#ifdef GAME_LOG_FILE_ENABLE
	if (error_file_stream->is_open()) {
		error_file_stream->write(log_str.c_str(), log_str.size());
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
	std::string tag_str = ":DEBUG: ";
	size_t format_str_size = strlen(GAME_UTILS_LOCALTIME_FORMAT) + tag_str.size();

	char buff[DEBUG_STRING_SIZE] = { '\0' };
	va_list argptr;
	va_start(argptr, format);
	vsnprintf_s(buff, (DEBUG_STRING_SIZE - format_str_size - 1), format, argptr);
	va_end(argptr);

	std::string log_str = buff;
	log_str = game_utils_get_localtime() + tag_str + log_str; // yyyy/mm/dd_hh:mm:ss:DEBUG: format_str
	size_t log_str_size = log_str.size();
	LOG_DEBUG_CONSOLE("%s", log_str.c_str());

	memcpy(debug_stack[debug_index], log_str.c_str(), log_str_size);
	debug_stack[debug_index][log_str_size] = '\0';

#ifdef GAME_LOG_FILE_ENABLE
	if (debug_file_stream->is_open()) {
		debug_file_stream->write(log_str.c_str(), log_str.size());
	}
	else {
		LOG_ERROR_CONSOLE("game_log_error write error\n");
	}
#endif

	debug_index += 1;
	if (debug_index >= DEBUG_STACK_SIZE) debug_index = 0;
}
#endif // GAME_LOG_DEBUG_ENABLE
