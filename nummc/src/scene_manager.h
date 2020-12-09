#pragma once
#include "game_common.h"

//int scene_stat;
#define SCENE_STAT_NONE      0
#define SCENE_STAT_LOADING   1
#define SCENE_STAT_IDLE      2
#define SCENE_STAT_ACTIVE    3
#define SCENE_STAT_END       4

//int scene_id;
#define SCENE_ID_NONE            0
#define SCENE_ID_TOP_MENU        1
#define SCENE_ID_LOAD_MENU       2
#define SCENE_ID_SAVE_MENU       3
#define SCENE_ID_SETTINGS_MENU   4
#define SCENE_ID_LOADING         5
#define SCENE_ID_ESCAPE_MENU     6
#define SCENE_ID_PLAY_STAGE      7
#define SCENE_ID_PLAY_STORY      8
#define SCENE_ID_END             9

typedef struct _SceneManagerFunc SceneManagerFunc;
struct _SceneManagerFunc{
	void_func*    pre_event;
	event_func*   key_event;
	void_func*    main_event;
	void_func*    pre_draw;
	void_func*    draw;
	void_func*    after_draw;
	void_p_func*  pre_load_event;
	void_func*    load_event;
	void_func*    unload_event;
	ret_int_func* get_stat_event;
	int_func*     set_stat_event;
};


extern int scene_manager_get_scene_id();
extern void scene_manager_init();
extern int scene_manager_load(int id, bool loading_on = false);

extern bool scene_manager_get_pre_load_stat();
extern void scene_manager_set_pre_load_stat(bool stat);
extern int scene_manager_loading_finish();

extern void scene_manager_pre_event();
extern void scene_manager_key_event(SDL_Event* e);
extern void scene_manager_main_event();
extern void scene_manager_pre_draw();
extern void scene_manager_draw();
extern void scene_manager_after_draw();

extern void scene_manager_load_event();
extern void scene_manager_unload_event();
extern int scene_manager_get_stat_event();
extern void scene_manager_set_stat_event(int stat);
