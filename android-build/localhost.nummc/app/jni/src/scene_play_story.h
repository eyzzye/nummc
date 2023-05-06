#pragma once
#include "game_common.h"
#include "scene_manager.h"

extern void scene_play_story_set_story(const char* path, bool is_opening_=false);
extern void scene_play_story_init();
extern SceneManagerFunc* scene_play_story_get_func();
