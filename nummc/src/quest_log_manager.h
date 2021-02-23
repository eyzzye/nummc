#pragma once
#include "game_common.h"

#define QUEST_LOG_REGIST_TIME  2000

extern int quest_log_manager_init();
extern void quest_log_manager_unload();
extern void quest_log_manager_reset();
extern void quest_log_manager_update();
extern void quest_log_manager_display();

extern void quest_log_manager_message(const char* message_fmt, ...);
extern void quest_log_manager_set_new_message(char* message, int message_length, int regist_timer = QUEST_LOG_REGIST_TIME);
