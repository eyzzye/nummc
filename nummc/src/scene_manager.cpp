#include <thread>
#include "game_common.h"
#include "scene_manager.h"

#include "scene_top_menu.h"
#include "scene_save_menu.h"
#include "scene_settings_menu.h"
#include "scene_play_stage.h"
#include "scene_play_story.h"
#include "scene_loading.h"
#include "scene_escape_menu.h"
#include "game_log.h"

static bool loading_enable;
static bool pre_load_stat;
static std::thread* pre_load_th;

static int scene_id;
static int next_scene_id;
static SceneManagerFunc scene_func;

static SceneManagerFunc* get_scene_func(int id)
{
	if (id == SCENE_ID_TOP_MENU) {
		return scene_top_menu_get_func();

	}
	else if (id == SCENE_ID_PLAY_STAGE) {
		return scene_play_stage_get_func();

	}
	else if (id == SCENE_ID_PLAY_STORY) {
		return scene_play_story_get_func();

	}
	else if (id == SCENE_ID_SAVE_MENU) {
		return scene_save_menu_get_func();

	}
	else if (id == SCENE_ID_SETTINGS_MENU) {
		return scene_settings_menu_get_func();

	}
	else if (id == SCENE_ID_LOADING) {
		return scene_loading_get_func();

	}
	else if (id == SCENE_ID_ESCAPE_MENU) {
		return scene_escape_menu_get_func();

	}
	return NULL;
}

int scene_manager_get_scene_id()
{
	return scene_id;
}

void scene_manager_init()
{
	scene_top_menu_init();
	scene_play_stage_init();
	scene_play_story_init();
	scene_save_menu_init();
	scene_settings_menu_init();
	scene_loading_init();
	scene_escape_menu_init();

	loading_enable = false;
	pre_load_stat = false;
	pre_load_th = NULL;

	scene_id = SCENE_ID_NONE;
	next_scene_id = SCENE_ID_NONE;
}

int scene_manager_load(int id, bool loading_on)
{
	if ((scene_manager_get_stat_event() != SCENE_STAT_NONE) && (scene_manager_get_stat_event() != SCENE_STAT_IDLE)) {
		return 1;
	}

	if (loading_on) {
		loading_enable = true;
		SceneManagerFunc* func = get_scene_func(id);
		func->set_stat_event(SCENE_STAT_LOADING);
		pre_load_th = new std::thread(func->pre_load_event, (void*)NULL);

		next_scene_id = id;
		id = SCENE_ID_LOADING;
	}

	{
		SceneManagerFunc* func = get_scene_func(id);
		if (func == NULL) { return 1; }

		scene_func.pre_event      = func->pre_event;
		scene_func.key_event      = func->key_event;
		scene_func.main_event     = func->main_event;
		scene_func.pre_draw       = func->pre_draw;
		scene_func.draw           = func->draw;
		scene_func.after_draw     = func->after_draw;
		scene_func.pre_load_event = func->pre_load_event;
		scene_func.load_event     = func->load_event;
		scene_func.unload_event   = func->unload_event;
		scene_func.get_stat_event = func->get_stat_event;
		scene_func.set_stat_event = func->set_stat_event;

		// load scene
		(*scene_func.load_event)();

		scene_manager_set_stat_event(SCENE_STAT_ACTIVE);
		scene_id = id;
		if (!loading_on) next_scene_id = SCENE_ID_NONE;
	}

	return 0;
}

// loading function
bool scene_manager_get_pre_load_stat()
{
	return pre_load_stat;
}
void scene_manager_set_pre_load_stat(bool stat)
{
	pre_load_stat = stat;
}

int scene_manager_loading_finish()
{
	if (loading_enable) {
		// terminate thread
		if (pre_load_th->joinable()) {
			pre_load_th->join();
		}
		delete pre_load_th;
		pre_load_th = NULL;

		// load next scene
		SceneManagerFunc* func = get_scene_func(next_scene_id);
		if (func == NULL) { return 1; }

		loading_enable = false;
		scene_manager_set_pre_load_stat(false);
		scene_manager_set_stat_event(SCENE_STAT_IDLE); // set loading stat

		scene_func.pre_event      = func->pre_event;
		scene_func.key_event      = func->key_event;
		scene_func.main_event     = func->main_event;
		scene_func.pre_draw       = func->pre_draw;
		scene_func.draw           = func->draw;
		scene_func.after_draw     = func->after_draw;
		scene_func.pre_load_event = func->pre_load_event;
		scene_func.load_event     = func->load_event;
		scene_func.unload_event   = func->unload_event;
		scene_func.get_stat_event = func->get_stat_event;
		scene_func.set_stat_event = func->set_stat_event;
		(*scene_func.load_event)();

		scene_manager_set_stat_event(SCENE_STAT_ACTIVE);
		scene_id = next_scene_id;
		next_scene_id = SCENE_ID_NONE;

		return 0;
	}
	return 1;
}

void scene_manager_pre_event()
{
	if (scene_func.pre_event != NULL) (*scene_func.pre_event)();
}
void scene_manager_key_event(SDL_Event* e)
{
	if (scene_func.key_event != NULL) (*scene_func.key_event)(e);
}
void scene_manager_main_event()
{
	if (scene_func.main_event != NULL) (*scene_func.main_event)();
}
void scene_manager_pre_draw()
{
	if (scene_func.pre_draw != NULL) (*scene_func.pre_draw)();
}
void scene_manager_draw()
{
	if (scene_func.draw != NULL) (*scene_func.draw)();
}
void scene_manager_after_draw()
{
	if (scene_func.after_draw != NULL) (*scene_func.after_draw)();
}

void scene_manager_load_event()
{
	if (scene_func.load_event != NULL) (*scene_func.load_event)();
}
void scene_manager_unload_event()
{
	if (scene_func.unload_event != NULL) (*scene_func.unload_event)();
}
int scene_manager_get_stat_event()
{
	if (scene_func.get_stat_event != NULL) return (*scene_func.get_stat_event)();
	return SCENE_STAT_NONE;
}
void scene_manager_set_stat_event(int stat)
{
	if (scene_func.set_stat_event != NULL) (*scene_func.set_stat_event)(stat);
}
