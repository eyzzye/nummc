#pragma once
#include "game_common.h"
#include "scene_manager.h"

#define SCENE_SAVE_MENU_DISP_TYPE_SAVE      0
#define SCENE_SAVE_MENU_DISP_TYPE_NEW_GAME  1
#define SCENE_SAVE_MENU_DISP_TYPE_LOAD      2
#define SCENE_SAVE_MENU_DISP_TYPE_END       3

extern void scene_save_menu_init();
extern SceneManagerFunc* scene_save_menu_get_func();
extern void scene_save_menu_set_display_title_type(int disp_type);
