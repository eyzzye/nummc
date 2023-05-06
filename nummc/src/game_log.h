#pragma once
#include "game_common.h"

#ifdef _WIN32
#include "windows.h"
#elif _ANDROID
#include <android/log.h>
#endif

//#define GAME_LOG_CONSOLE_ENABLE 1
#define GAME_LOG_ERROR_ENABLE   1
//#define GAME_LOG_DEBUG_ENABLE   1

#ifdef _ANDROID
//#define GAME_LOG_FILE_ENABLE    1
#else
#define GAME_LOG_FILE_ENABLE    1
#endif

#ifdef GAME_LOG_CONSOLE_ENABLE
#ifdef _WIN32
#include <stdio.h>
#define LOG_ERROR_CONSOLE(...) { char buff[256]; sprintf_s(buff, __VA_ARGS__); OutputDebugStringA(buff); }
#define LOG_DEBUG_CONSOLE(...) { char buff[256]; sprintf_s(buff, __VA_ARGS__); OutputDebugStringA(buff); }

#define LOG_ERROR_WINAPI_CONSOLE(FORMAT_STR) { char err[256]; \
 FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM, NULL, GetLastError(), MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US), err, sizeof(err), NULL); \
 LOG_ERROR_CONSOLE(FORMAT_STR, err); }
#define LOG_DEBUG_WINAPI_CONSOLE(FORMAT_STR) { char err[256]; \
 FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM, NULL, GetLastError(), MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US), err, sizeof(err), NULL); \
 LOG_DEBUG_CONSOLE(FORMAT_STR, err); }

#elif _ANDROID
#define LOG_ERROR_CONSOLE(...) { __android_log_print(ANDROID_LOG_DEBUG, "nummc", __VA_ARGS__); }
#define LOG_DEBUG_CONSOLE(...) { __android_log_print(ANDROID_LOG_DEBUG, "nummc", __VA_ARGS__); }
#define LOG_ERROR_WINAPI_CONSOLE(FORMAT_STR)
#define LOG_DEBUG_WINAPI_CONSOLE(FORMAT_STR)
#else  //_WIN32
#include <stdio.h>
#define LOG_ERROR_CONSOLE(...) { printf(__VA_ARGS__); }
#define LOG_DEBUG_CONSOLE(...) { printf(__VA_ARGS__); }
#define LOG_ERROR_WINAPI_CONSOLE(FORMAT_STR)
#define LOG_DEBUG_WINAPI_CONSOLE(FORMAT_STR)
#endif //_WIN32

#else  //GAME_LOG_CONSOLE_ENABLE
#define LOG_ERROR_CONSOLE(...)
#define LOG_DEBUG_CONSOLE(...)
#define LOG_ERROR_WINAPI_CONSOLE(FORMAT_STR)
#define LOG_DEBUG_WINAPI_CONSOLE(FORMAT_STR)
#endif //GAME_LOG_CONSOLE_ENABLE

#ifdef GAME_LOG_ERROR_ENABLE
#define LOG_ERROR(...) game_log_error(__VA_ARGS__)
#else
#define LOG_ERROR(...)
#endif

#ifdef GAME_LOG_DEBUG_ENABLE
#define LOG_DEBUG(...) game_log_debug(__VA_ARGS__)
#else
#define LOG_DEBUG(...)
#endif

extern int game_log_init();
extern void game_log_close();
extern void game_log_error(const char* format, ...);
extern void game_log_debug(const char* format, ...);
