#pragma once
#include "game_common.h"

extern bool g_dialog_exit_enable;

extern void dialog_exit_init(void_func* ok_func, void_func* cancel_func);
extern void dialog_exit_reset();
extern void dialog_exit_set_enable(bool enable);
extern void dialog_exit_event();
extern void dialog_exit_draw();
