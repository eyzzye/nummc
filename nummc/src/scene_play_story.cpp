#include "game_common.h"
#include "gui_common.h"
#include "scene_manager.h"
#include "scene_play_story.h"

#include "resource_manager.h"
#include "sound_manager.h"
#include "story_manager.h"
#include "game_key_event.h"
#include "game_window.h"
#include "game_timer.h"
#include "game_log.h"
#include "game_save.h"
#include "scene_loading.h"
#include "scene_play_stage.h"

#define SCENE_PLAY_STORY_AUTO_TEXT_END  32
tex_info_t tex_info_auto_text[SCENE_PLAY_STORY_AUTO_TEXT_END];

tex_info_t tex_info_background;

tex_info_t tex_info_enter;
#define ENTER_TEXT_BLINK_TIME  1000
static int enter_text_blink_timer;
static bool auto_text_finish;
static bool enter_text_disp;

static int auto_text_timer;
static int auto_text_wait_time;
static int auto_text_index;
static std::string story_path;

static void tex_info_reset();
static void unload_event();
static void set_stat_event(int stat);

static SceneManagerFunc scene_func;
static int scene_stat;
static bool is_opening;
static bool draw_dirt;

static void pre_event() {

}
static void key_event(SDL_Event* e) {
	if (scene_stat != SCENE_STAT_ACTIVE) return;

	game_key_event_set(e);
}
static void main_event() {
	if (!auto_text_finish) auto_text_timer += g_delta_time;
	else enter_text_blink_timer += g_delta_time;

	if ((!auto_text_finish) && (auto_text_timer > (auto_text_index + 1) * auto_text_wait_time)) {
		auto_text_index += 1;
		draw_dirt = true;

		if (auto_text_index >= g_story_data->auto_text_list.size()) {
			auto_text_finish = true;
		}
	}

	if (auto_text_finish) {
		if (enter_text_blink_timer > ENTER_TEXT_BLINK_TIME) {
			enter_text_blink_timer = 0;
			enter_text_disp = !enter_text_disp;
			draw_dirt = true;
		}
	}

	if (game_key_event_get(SDL_SCANCODE_RETURN, GUI_SELECT_WAIT_TIMER)) {
		if (is_opening) {
			std::string start_stage = "1";

			// set loading data (set bin data)
			scene_loading_set_stage(start_stage);
			scene_play_stage_set_stage_id(start_stage);

			// loading play stage
			set_stat_event(SCENE_STAT_NONE);
			scene_manager_load(SCENE_ID_PLAY_STAGE, true);
			unload_event();
		}
		else {
			set_stat_event(SCENE_STAT_NONE);
			scene_manager_load(SCENE_ID_TOP_MENU);
			unload_event();
		}
	}
}
static void pre_draw() {

}
static void draw() {
	if (scene_stat != SCENE_STAT_ACTIVE) return;

	if (draw_dirt) {
		// set background
		SDL_SetRenderDrawColor(g_ren, 0, 0, 0, 255);
		SDL_RenderClear(g_ren);
		SDL_SetRenderDrawColor(g_ren, 16, 16, 16, 255);
		SDL_RenderFillRect(g_ren, &g_screen_size);

		// background image
		if (tex_info_background.res_img) GUI_tex_info_draw(&tex_info_background);

		if (auto_text_finish) {
			if (enter_text_disp) GUI_tex_info_draw(&tex_info_enter);
		}
		else {
			// auto text
			GUI_tex_info_draw(&tex_info_auto_text[auto_text_index]);
		}

		SDL_RenderPresent(g_ren);

		draw_dirt = false;
	}
}
static void after_draw() {

}
static void load_event() {
	// load story data
	auto_text_finish = false;
	story_manager_init();
	story_manager_load(story_path);

	// reset size
	tex_info_reset();

	// key event switch
	game_key_event_init();
	g_key_event_enable.push_back(SDL_SCANCODE_RETURN);

	auto_text_timer = 0;
	auto_text_wait_time = g_story_data->auto_text_time;
	auto_text_index = 0;
	draw_dirt = true;

	enter_text_blink_timer = 0;
	enter_text_disp = false;

	// play music
	if (g_story_data->res_chunk) {
		sound_manager_play(g_story_data->res_chunk, SOUND_MANAGER_CH_MUSIC, -1);
	}
}
static void unload_event() {
	story_manager_unload();
}
static int get_stat_event() {
	return scene_stat;
}
static void set_stat_event(int stat) {
	if (stat == SCENE_STAT_IDLE) {
		sound_manager_stop(SOUND_MANAGER_CH_MUSIC);
	}
	scene_stat = stat;
}


// init draw items
static void tex_info_reset()
{
	int ret = 0;
	int w, h;
	int w_pos = 0, h_pos = 0;

	// background
	if (g_story_data->res_img) {
		tex_info_background.res_img = g_story_data->res_img;
		ret = GUI_QueryTexture(tex_info_background.res_img, NULL, NULL, &w, &h);
		if (ret == 0) {
			w_pos = 0;
			h_pos = 0;
			GUI_tex_info_init_rect(&tex_info_background, w, h, w_pos, h_pos);
		}
	}

	// enter
	std::string enter_text_str = "{-,204:204:204:204,-,-}Press enter";
	tex_info_enter.res_img = resource_manager_getFontTextureFromPath(enter_text_str);
	ret = GUI_QueryTexture(tex_info_enter.res_img, NULL, NULL, &w, &h);
	if (ret == 0) {
		w_pos = SCREEN_WIDTH / 2 - w / 2;
		h_pos = SCREEN_HEIGHT - 40 - h;
		GUI_tex_info_init_rect(&tex_info_enter, w, h, w_pos, h_pos);
	}

	// auto_text
	for (int i = 0; i < SCENE_PLAY_STORY_AUTO_TEXT_END; i++) {
		if (i >= g_story_data->auto_text_list.size()) {
			break;
		}

		std::string auto_text_str = "{-,204:204:204:204,-,-}" + g_story_data->auto_text_list[i];
		tex_info_auto_text[i].res_img = resource_manager_getFontTextureFromPath(auto_text_str);
		ret = GUI_QueryTexture(tex_info_auto_text[i].res_img, NULL, NULL, &w, &h);
		if (ret == 0) {
			w_pos = SCREEN_WIDTH / 2 - w / 2;
			h_pos = SCREEN_HEIGHT - 40 - h;
			GUI_tex_info_init_rect(&tex_info_auto_text[i], w, h, w_pos, h_pos);
		}
	}
}

void scene_play_story_set_story(std::string& path, bool is_opening_)
{
	story_path = path;
	is_opening = is_opening_;
}

void scene_play_story_init()
{
	// set stat
	scene_stat = SCENE_STAT_NONE;

	scene_func.pre_event = &pre_event;
	scene_func.key_event = &key_event;
	scene_func.main_event = &main_event;
	scene_func.pre_draw = &pre_draw;
	scene_func.draw = &draw;
	scene_func.after_draw = &after_draw;
	scene_func.load_event = &load_event;
	scene_func.unload_event = &unload_event;
	scene_func.get_stat_event = &get_stat_event;
	scene_func.set_stat_event = &set_stat_event;

	// load resource files
	resource_manager_load_dat("scenes/scene_play_story.dat");
}

SceneManagerFunc* scene_play_story_get_func()
{
	return &scene_func;
}
