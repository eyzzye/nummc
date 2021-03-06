#pragma once
#include "game_common.h"
#include "scene_manager.h"

extern void scene_loading_set_stage(const char* id);
extern void scene_loading_init();
extern SceneManagerFunc* scene_loading_get_func();
