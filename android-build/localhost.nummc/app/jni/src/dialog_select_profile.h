#pragma once
#include "game_common.h"

extern bool g_dialog_select_profile_enable;

extern void dialog_select_profile_init();
extern void dialog_select_profile_reset(const char* message, void_func* yes_func);
extern void dialog_select_profile_set_enable(bool enable);
extern void dialog_select_profile_event();
extern void dialog_select_profile_draw();
extern int dialog_select_profile_get_current_profile_index();
