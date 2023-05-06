#include "game_common.h"
#include "gui_common.h"
#include "gui_loading.h"

#include "memory_manager.h"
#include "resource_manager.h"
#include "sound_manager.h"
#include "game_key_event.h"
#include "game_window.h"
#include "game_timer.h"
#include "game_utils.h"
#include "game_log.h"

#ifdef _ANDROID
#include "gui_touch_control.h"
#endif

static tex_info_t tex_info_title;

// title data
#define GUI_LOADING_ID_PROGRESS_1    0
#define GUI_LOADING_ID_PROGRESS_2    1
#define GUI_LOADING_ID_PROGRESS_3    2
#define GUI_LOADING_ID_PROGRESS_4    3
#define GUI_LOADING_ID_PROGRESS_END  4

static tex_info_t tex_info_progress[GUI_LOADING_ID_PROGRESS_END];

static bool loading_on;
static int progress_timer;
static int progress_index;
static bool progress_dirt;
static char next_stage_id[MEMORY_MANAGER_NAME_BUF_SIZE];

// init draw items
static void tex_info_init()
{
	int w, h;
	int w_pos = 0, h_pos = 0;

	// progress
	const char* progress_str = "{-,204:204:204:204,-,-}Loading ...";
	int progress_str_size = (int)strlen(progress_str);
	for (int i = 0; i < GUI_LOADING_ID_PROGRESS_END; i++) {
		char progress_substr[MEMORY_MANAGER_STRING_BUF_SIZE];
		int progress_substr_size = progress_str_size - (GUI_LOADING_ID_PROGRESS_END - 1 - i);
		int ret = game_utils_string_copy_n(progress_substr, progress_str, progress_substr_size);
		if (ret != 0) {
			LOG_ERROR("gui_loading tex_info_init() get progress_substr\n");
			continue;
		}

		tex_info_progress[GUI_LOADING_ID_PROGRESS_1 + i].res_img = resource_manager_getFontTextureFromPath(progress_substr);
		ret = GUI_QueryTexture(tex_info_progress[GUI_LOADING_ID_PROGRESS_1 + i].res_img, NULL, NULL, &w, &h);
		if (ret == 0) {
			w_pos = 20;
			h_pos = SCREEN_HEIGHT - 20 - h;
			GUI_tex_info_init_rect(&tex_info_progress[GUI_LOADING_ID_PROGRESS_1 + i], w, h, w_pos, h_pos);
		}
	}

	// clear title for start loading
	tex_info_title.res_img = NULL;
}

void gui_loading_set_stage(const char* id)
{
	//next_stage_id = id;
	int ret = game_utils_string_copy(next_stage_id, id);
	if (ret != 0) { LOG_ERROR("gui_loading_set_stage get next_stage_id\n"); return; }

	int w, h;
	int w_pos = 0, h_pos = 0;

	// title
	//std::string title_str = "{48,204:204:204:204,-,-}STAGE " + next_stage_id;
	char title_str[MEMORY_MANAGER_STRING_BUF_SIZE];
	int title_str_size = game_utils_string_cat(title_str, (char*)"{48,204:204:204:204,-,-}STAGE ", (char*)id);
	if (title_str_size <= 0) { LOG_ERROR("gui_loading_set_stage get title_str\n");	}

	tex_info_title.res_img = resource_manager_getFontTextureFromPath(title_str, RESOURCE_MANAGER_TYPE_STATIC);
	ret = GUI_QueryTexture(tex_info_title.res_img, NULL, NULL, &w, &h);
	if (ret == 0) {
		w_pos = (SCREEN_WIDTH - w) / 2;
		h_pos = (SCREEN_HEIGHT - h) / 2;
		GUI_tex_info_init_rect(&tex_info_title, w, h, w_pos, h_pos);
	}
}

void gui_loading_init()
{
	loading_on = false;
	progress_index = 0;
	progress_timer = 0;
	progress_dirt = true;
	next_stage_id[0] = '\0';

	// load resource files
	resource_manager_load_dat((char*)"scenes/gui/gui_loading.dat");

	tex_info_init();
}

void gui_loading_update_progress()
{
	// loading_on=false, through all process
	if (!loading_on) return;

	int dt = (int)game_timer_update();
	progress_timer += (int)game_timer_get_delta_time();
	while(game_timer_get_delta_time() == ONE_FRAME_TIME) {
		progress_timer += (int)ONE_FRAME_TIME;
	}

	if (progress_timer > 1000) {
		if (progress_index >= 3) {
			progress_index = 0;
		}
		else {
			progress_index += 1;
		}

		progress_timer = 0;
		progress_dirt = true;
	}

	// draw loading...
	gui_loading_draw();
}

void gui_loading_focus(bool _on)
{
	if (_on) {
		// reset size
		GUI_tex_info_reset(tex_info_progress, GUI_LOADING_ID_PROGRESS_END);
		GUI_tex_info_reset(&tex_info_title);

		progress_index = 0;
		progress_timer = 0;
		progress_dirt = true;
		next_stage_id[0] = '\0';

		// key event switch
		game_key_event_init();

		//SDL_RenderPresent(l_ren);
		sound_manager_play(resource_manager_getChunkFromPath("music/loading.ogg"), SOUND_MANAGER_CH_MUSIC, -1);

		loading_on = true;

		// draw loading...
		gui_loading_draw();
	}
	else {
		sound_manager_stop(SOUND_MANAGER_CH_MUSIC);

		loading_on = false;
	}
}

void gui_loading_draw()
{
	if (progress_dirt) {
		if(g_render_mtx.try_lock()) {
		//if (1) { g_render_mtx.lock();

			// set background
			SDL_SetRenderDrawColor(g_ren, 0, 0, 0, 255);
			SDL_RenderClear(g_ren);
			SDL_SetRenderDrawColor(g_ren, 16, 16, 16, 255);
			SDL_RenderFillRect(g_ren, &g_screen_size);

			// title
			GUI_tex_info_draw(&tex_info_title);

			// loading...
			GUI_tex_info_draw(&tex_info_progress[GUI_LOADING_ID_PROGRESS_1 + progress_index]);

#ifdef _ANDROID
			// draw gui_touch_control
			gui_touch_control_draw();
#endif

			SDL_RenderPresent(g_ren);

			g_render_mtx.unlock();
			progress_dirt = false;
		}
		else {
			LOG_DEBUG("gui_loading_draw() try_lock fail ");
		}
	}
}
