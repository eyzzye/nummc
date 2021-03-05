#pragma once
#include "game_common.h"

// dialog type
#define DIALOG_MSG_TYPE_CANCEL_OK  0
#define DIALOG_MSG_TYPE_OK_ONLY    1
#define DIALOG_MSG_TYPE_NO_YES     2
#define DIALOG_MSG_TYPE_END        3

extern bool g_dialog_message_enable;

extern void dialog_message_init();
extern void dialog_message_reset(const char* message, void_func* no_func, void_func* yes_func, int dialog_type_ = DIALOG_MSG_TYPE_CANCEL_OK);
extern void dialog_message_set_enable(bool enable);
extern void dialog_message_event();
extern void dialog_message_draw();
