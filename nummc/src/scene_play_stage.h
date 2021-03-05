#pragma once
#include "game_common.h"
#include "scene_manager.h"

extern void scene_play_stage_init();
extern SceneManagerFunc* scene_play_stage_get_func();
extern void scene_play_stage_set_stage_id(const char* id);
extern void scene_play_stage_set_player(const char* path, bool reload_on=false);
extern void scene_play_stage_play_current_bgm(bool on_off);
extern void scene_play_next_stage();
extern void scene_play_next_section(int go_next_id);
extern void scene_play_stage_close_door();
extern void scene_play_stage_set_lose();
