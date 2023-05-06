#pragma once
#include "game_common.h"

extern void gui_loading_init();
extern void gui_loading_set_stage(const char* id);
extern void gui_loading_update_progress();
extern void gui_loading_focus(bool _on);
extern void gui_loading_draw();
