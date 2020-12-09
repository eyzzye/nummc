#pragma once
#include "game_common.h"

extern const int g_hud_offset_x;
extern const int g_hud_offset_y;

extern int hud_manager_init();
extern void hud_manager_unload();
extern void hud_manager_reset();
extern void hud_manager_update();
extern void hud_manager_display();
