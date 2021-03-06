#include "game_common.h"
#include "gui_common.h"
#include "scene_manager.h"
#include "scene_loading.h"

#include "memory_manager.h"
#include "resource_manager.h"
#include "sound_manager.h"
#include "game_key_event.h"
#include "game_window.h"
#include "game_timer.h"
#include "game_utils.h"
#include "game_log.h"

static tex_info_t tex_info_title;

// title data
#define SCENE_LOADING_ID_PROGRESS_1    0
#define SCENE_LOADING_ID_PROGRESS_2    1
#define SCENE_LOADING_ID_PROGRESS_3    2
#define SCENE_LOADING_ID_PROGRESS_4    3
#define SCENE_LOADING_ID_PROGRESS_END  4

static tex_info_t tex_info_progress[SCENE_LOADING_ID_PROGRESS_END];

static int title_wait_timer;
static int progress_timer;
static int progress_index;
static bool progress_dirt;
static char next_stage_id[MEMORY_MANAGER_NAME_BUF_SIZE];

static SceneManagerFunc scene_func;
static int scene_stat;

static void pre_event() {

}
static void key_event(SDL_Event* e) {

}
static void main_event() {
	title_wait_timer += g_delta_time;
	progress_timer += g_delta_time;
	for (int i = 3; i >= 0; i--) {
		if (progress_timer > (i + 1) * 1000) {
			if (progress_index != i) {
				progress_index = i;
				progress_dirt = true;
			}
			break;
		}
	}
	if (progress_timer > 4000) {
		progress_timer = 0;
	}

	// watch loading stat
	if ((title_wait_timer > 5000) && scene_manager_get_pre_load_stat()) {
		scene_manager_loading_finish();
	}
}
static void pre_draw() {

}
static void draw() {
	if (scene_stat != SCENE_STAT_ACTIVE) return;

	if (progress_dirt) {
		g_render_mtx.lock();

		// set background
		SDL_SetRenderDrawColor(g_ren, 0, 0, 0, 255);
		SDL_RenderClear(g_ren);
		SDL_SetRenderDrawColor(g_ren, 16, 16, 16, 255);
		SDL_RenderFillRect(g_ren, &g_screen_size);

		// title
		GUI_tex_info_draw(&tex_info_title);

		// loading...
		GUI_tex_info_draw(&tex_info_progress[SCENE_LOADING_ID_PROGRESS_1 + progress_index]);

		SDL_RenderPresent(g_ren);

		g_render_mtx.unlock();
		progress_dirt = false;
	}
}
static void after_draw() {

}
static void load_event() {
	// reset size
	GUI_tex_info_reset(tex_info_progress, SCENE_LOADING_ID_PROGRESS_END);
	GUI_tex_info_reset(&tex_info_title);

	// key event switch
	game_key_event_init();

	title_wait_timer = 0;
	progress_index = 0;
	progress_timer = 0;
	progress_dirt = true;
	next_stage_id[0] = '\0';

	// play music
	sound_manager_play(resource_manager_getChunkFromPath("music/loading.ogg"), SOUND_MANAGER_CH_MUSIC, -1);
}
static void unload_event() {

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
static void tex_info_init()
{
	int w, h;
	int w_pos = 0, h_pos = 0;

	// progress
	const char* progress_str = "{-,204:204:204:204,-,-}Loading ...";
	int progress_str_size = (int)strlen(progress_str);
	for (int i = 0; i < SCENE_LOADING_ID_PROGRESS_END; i++) {
		char progress_substr[MEMORY_MANAGER_STRING_BUF_SIZE];
		int progress_substr_size = progress_str_size - (SCENE_LOADING_ID_PROGRESS_END - 1 - i);
		int ret = game_utils_string_copy_n(progress_substr, progress_str, progress_substr_size);
		if (ret != 0) {
			LOG_ERROR("Error: scene_loading tex_info_init() get progress_substr\n");
			continue;
		}

		tex_info_progress[SCENE_LOADING_ID_PROGRESS_1 + i].res_img = resource_manager_getFontTextureFromPath(progress_substr);
		ret = GUI_QueryTexture(tex_info_progress[SCENE_LOADING_ID_PROGRESS_1 + i].res_img, NULL, NULL, &w, &h);
		if (ret == 0) {
			w_pos = 20;
			h_pos = SCREEN_HEIGHT - 20 - h;
			GUI_tex_info_init_rect(&tex_info_progress[SCENE_LOADING_ID_PROGRESS_1 + i], w, h, w_pos, h_pos);
		}
	}
}

void scene_loading_set_stage(const char* id)
{
	//next_stage_id = id;
	int ret = game_utils_string_copy(next_stage_id, id);
	if (ret != 0) { LOG_ERROR("Error: scene_loading_set_stage get next_stage_id\n"); return; }

	int w, h;
	int w_pos = 0, h_pos = 0;

	// title
	//std::string title_str = "{48,204:204:204:204,-,-}STAGE " + next_stage_id;
	char title_str[MEMORY_MANAGER_STRING_BUF_SIZE];
	int title_str_size = game_utils_string_cat(title_str, (char*)"{48,204:204:204:204,-,-}STAGE ", (char*)id);
	if (title_str_size <= 0) { LOG_ERROR("Error: scene_loading_set_stage get title_str\n");	}

	tex_info_title.res_img = resource_manager_getFontTextureFromPath(title_str, RESOURCE_MANAGER_TYPE_STATIC);
	ret = GUI_QueryTexture(tex_info_title.res_img, NULL, NULL, &w, &h);
	if (ret == 0) {
		w_pos = (SCREEN_WIDTH - w) / 2;
		h_pos = (SCREEN_HEIGHT - h) / 2;
		GUI_tex_info_init_rect(&tex_info_title, w, h, w_pos, h_pos);
	}
}

void scene_loading_init()
{
	// set stat
	scene_stat = SCENE_STAT_NONE;

	scene_func.pre_event      = &pre_event;
	scene_func.key_event      = &key_event;
	scene_func.main_event     = &main_event;
	scene_func.pre_draw       = &pre_draw;
	scene_func.draw           = &draw;
	scene_func.after_draw     = &after_draw;
	scene_func.load_event     = &load_event;
	scene_func.unload_event   = &unload_event;
	scene_func.get_stat_event = &get_stat_event;
	scene_func.set_stat_event = &set_stat_event;

	// load resource files
	resource_manager_load_dat((char*)"scenes/scene_loading.dat");

	// set texture position
	tex_info_init();
}

SceneManagerFunc* scene_loading_get_func()
{
	return &scene_func;
}
