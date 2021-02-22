#include "game_common.h"
#include "gui_common.h"
#include "scene_manager.h"
#include "scene_escape_menu.h"

#include "scene_save_menu.h"
#include "resource_manager.h"
#include "sound_manager.h"
#include "game_key_event.h"
#include "game_mouse_event.h"
#include "game_window.h"
#include "game_loop.h"
#include "game_save.h"
#include "game_utils.h"
#include "dialog_message.h"
#include "scene_play_stage.h"

// draw data
#define SCENE_ESCAPE_MENU_ID_TITLE     0
#define SCENE_ESCAPE_MENU_ID_RESUME    1
#define SCENE_ESCAPE_MENU_ID_LOAD      2
#define SCENE_ESCAPE_MENU_ID_SETTINGS  3
#define SCENE_ESCAPE_MENU_ID_EXIT      4
#define SCENE_ESCAPE_MENU_ID_END       5

static tex_info_t tex_info[SCENE_ESCAPE_MENU_ID_END];
static tex_info_t tex_cursor;

#define MENU_ITEM_RESUME    0
#define MENU_ITEM_LOAD      1
#define MENU_ITEM_SETTINGS  2
#define MENU_ITEM_EXIT      3
#define MENU_ITEM_END       4

static gui_item_t menu_items[MENU_ITEM_END];
static int menu_index = 0;

static void dialog_message_cancel();
static void dialog_message_ok();

// GUI stat
#define GUI_ITEM_GROUP_ID_SELECT 0
#define GUI_ITEM_GROUP_ID_SIZE   1

static int gui_active_group_id;
static int gui_active_menu_index;

// event functions
static SceneManagerFunc scene_func;
static int scene_stat;
static int return_scene_id;

// event func
static void pre_event() {
	if (scene_stat != SCENE_STAT_ACTIVE) return;

	game_mouse_event_reset();
}
static void key_event(SDL_Event* e) {
	if (scene_stat != SCENE_STAT_ACTIVE) return;

	game_key_event_set(e);
	game_mouse_event_set(e);
}

static void main_event() {
	if (scene_stat != SCENE_STAT_ACTIVE) return;

	if (g_dialog_message_enable) {
		dialog_message_event();
		return; // recive only dialog key
	}

	bool select_snd_on = false;
	bool click_snd_on = false;
	if (game_key_event_get(SDL_SCANCODE_UP, GUI_SELECT_WAIT_TIMER)) {
		select_snd_on = true;
		menu_index -= 1;
		if (menu_index < 0) menu_index = MENU_ITEM_END - 1;
	}
	if (game_key_event_get(SDL_SCANCODE_DOWN, GUI_SELECT_WAIT_TIMER)) {
		select_snd_on = true;
		menu_index += 1;
		if (menu_index >= MENU_ITEM_END) menu_index = 0;
	}
	if (game_key_event_get(SDL_SCANCODE_RETURN, GUI_SELECT_WAIT_TIMER)) {
		click_snd_on = true;
		(*menu_items[menu_index].func)();
	}
	if (game_key_event_get(SDL_SCANCODE_ESCAPE, GUI_SELECT_WAIT_TIMER)) {
		scene_stat = SCENE_STAT_IDLE;
		scene_manager_load(SCENE_ID_PLAY_STAGE);
	}

	// mouse event
	int x = 0, y = 0;
	if (game_mouse_event_get_motion(&x, &y)) {
		gui_active_menu_index = -1;
		for (int i = 0; i < MENU_ITEM_END; i++) {
			if (game_utils_decision_internal(&tex_info[menu_items[i].tex_info_id].dst_rect, x, y)) {
				if (menu_index != i) select_snd_on = true;
				menu_index = i;
				menu_items[i].mouse_stat = GUI_BUTTON_ACTIVE;
				gui_active_menu_index = i;
				gui_active_group_id = GUI_ITEM_GROUP_ID_SELECT;
			}
			else {
				menu_items[i].mouse_stat = 0;
			}
		}
	}

	Uint32 mouse_left_stat = game_mouse_event_get(GAME_MOUSE_LEFT);
	if ((mouse_left_stat & GAME_MOUSE_CLICK) && (gui_active_menu_index >= 0)) {
		if (gui_active_group_id == GUI_ITEM_GROUP_ID_SELECT) {
			if (menu_items[gui_active_menu_index].mouse_stat & GUI_BUTTON_ACTIVE) {
				menu_items[gui_active_menu_index].mouse_stat |= GUI_BUTTON_CLICK;
			}
		}
	}
	if ((mouse_left_stat & GAME_MOUSE_RELEASE) && (gui_active_menu_index >= 0)) {
		if (gui_active_group_id == GUI_ITEM_GROUP_ID_SELECT) {
			if (menu_items[gui_active_menu_index].mouse_stat == (GUI_BUTTON_ACTIVE | GUI_BUTTON_CLICK)) {
				click_snd_on = true;
				menu_items[gui_active_menu_index].mouse_stat &= ~GUI_BUTTON_CLICK;
				(*menu_items[gui_active_menu_index].func)();
			}
		}
	}

	if (select_snd_on) {
		sound_manager_play(resource_manager_getChunkFromPath("sounds/sfx_select1.ogg"), SOUND_MANAGER_CH_SFX1);
	}
	if (click_snd_on) {
		sound_manager_play(resource_manager_getChunkFromPath("sounds/sfx_click1.ogg"), SOUND_MANAGER_CH_SFX2);
	}
}
static void pre_draw() {
	if (scene_stat != SCENE_STAT_ACTIVE) return;

}
static void draw() {
	if (scene_stat != SCENE_STAT_ACTIVE) return;

	// set background
	SDL_SetRenderDrawColor(g_ren, 0, 0, 0, 255);
	SDL_RenderClear(g_ren);
	SDL_SetRenderDrawColor(g_ren, 192, 192, 192, 255);
	SDL_RenderFillRect(g_ren, &g_screen_size);

	// draw item
	GUI_tex_info_draw(tex_info, SCENE_ESCAPE_MENU_ID_END);

	// draw cursor
	GUI_RenderCopy(tex_cursor.res_img, &tex_cursor.src_rect, &menu_items[menu_index].dst_rect);

	// dialog exit
	if (g_dialog_message_enable) {
		// set background dark
		SDL_SetRenderDrawColor(g_ren, 0, 0, 0, 128);
		SDL_RenderFillRect(g_ren, &g_screen_size);

		// draw dialog
		dialog_message_draw();
	}

	SDL_RenderPresent(g_ren);
}
static void after_draw() {
	if (scene_stat != SCENE_STAT_ACTIVE) return;

}
static void load_event() {
	// resize
	GUI_tex_info_reset(tex_info, SCENE_ESCAPE_MENU_ID_END);
	GUI_tex_info_reset(&tex_cursor);
	GUI_gui_item_reset(menu_items, MENU_ITEM_END);

	// key event switch
	game_key_event_init();
	g_key_event_enable.push_back(SDL_SCANCODE_ESCAPE);
	g_key_event_enable.push_back(SDL_SCANCODE_RETURN);
	g_key_event_enable.push_back(SDL_SCANCODE_UP);
	g_key_event_enable.push_back(SDL_SCANCODE_DOWN);
	g_key_event_enable.push_back(SDL_SCANCODE_LEFT);
	g_key_event_enable.push_back(SDL_SCANCODE_RIGHT);
	g_key_event_enable.push_back(SDL_SCANCODE_W);
	g_key_event_enable.push_back(SDL_SCANCODE_S);
	g_key_event_enable.push_back(SDL_SCANCODE_A);
	g_key_event_enable.push_back(SDL_SCANCODE_D);

	game_mouse_event_init(0, 400, 200, 150, 5);

	// get current mouse position
	int x = 0, y = 0;
	menu_index = MENU_ITEM_RESUME;
	gui_active_group_id = GUI_ITEM_GROUP_ID_SELECT;
	gui_active_menu_index = -1;
	game_mouse_event_get_motion(&x, &y);
	for (int i = 0; i < MENU_ITEM_END; i++) {
		if (game_utils_decision_internal(&tex_info[menu_items[i].tex_info_id].dst_rect, x, y)) {
			menu_index = i;
			menu_items[i].mouse_stat = GUI_BUTTON_ACTIVE;
			gui_active_menu_index = i;
			gui_active_group_id = GUI_ITEM_GROUP_ID_SELECT;
		}
		else {
			menu_items[i].mouse_stat = 0;
		}
	}
}
static void unload_event() {
	scene_play_stage_get_func()->unload_event();
}
static int get_stat_event() {
	return scene_stat;
}
static void set_stat_event(int stat) {
	scene_stat = stat;
}

// menu func
static void menu_resume() {
	scene_stat = SCENE_STAT_IDLE;
	scene_manager_load(SCENE_ID_PLAY_STAGE);
}
static void menu_load() {
	scene_save_menu_set_display_title_type(SCENE_SAVE_MENU_DISP_TYPE_LOAD);
	scene_stat = SCENE_STAT_IDLE;
	scene_manager_load(SCENE_ID_SAVE_MENU);
}
static void menu_settings() {
	scene_stat = SCENE_STAT_IDLE;
	scene_manager_load(SCENE_ID_SETTINGS_MENU);
	//unload_event();
}
static void menu_exit() {
	dialog_message_reset("Exit to TopMenu?", dialog_message_cancel, dialog_message_ok, DIALOG_MSG_TYPE_NO_YES);
	dialog_message_set_enable(true);
}

// dialog func
static void dialog_message_ok() {
	//close scene_play_game
	scene_play_stage_get_func()->unload_event();

	sound_manager_play(resource_manager_getChunkFromPath("sounds/sfx_click1.ogg"), SOUND_MANAGER_CH_SFX2);
	scene_stat = SCENE_STAT_IDLE;
	scene_manager_load(SCENE_ID_TOP_MENU);

	dialog_message_set_enable(false);
}
static void dialog_message_cancel() {
	sound_manager_play(resource_manager_getChunkFromPath("sounds/sfx_cancel1.ogg"), SOUND_MANAGER_CH_SFX2);
	dialog_message_set_enable(false);
}


// init draw items
static void tex_info_init()
{
	int w, h;
	int w_pos = 0, h_pos = 0;
	int cur_w = 0, cur_h = 0;
	int cur_w_pos = 0, cur_h_pos = 0;

	// title
	tex_info[SCENE_ESCAPE_MENU_ID_TITLE].res_img = resource_manager_getFontTextureFromPath(" ");
	int ret = GUI_QueryTexture(tex_info[SCENE_ESCAPE_MENU_ID_TITLE].res_img, NULL, NULL, &w, &h);
	if (ret == 0) {
		GUI_tex_info_init_rect(&tex_info[SCENE_ESCAPE_MENU_ID_TITLE], w, h, 20, 20);
	}

	// menu1
	tex_info[SCENE_ESCAPE_MENU_ID_RESUME].res_img = resource_manager_getTextureFromPath("{scale_mode:linear}images/gui/esc_menu/resume.png");
	ret = GUI_QueryTexture(tex_info[SCENE_ESCAPE_MENU_ID_RESUME].res_img, NULL, NULL, &w, &h);
	if (ret == 0) {
		cur_w_pos = w_pos = SCREEN_WIDTH / 2 - w / 2;
		cur_h_pos = h_pos = SCREEN_HEIGHT / 2 - h / 2;
		GUI_tex_info_init_rect(&tex_info[SCENE_ESCAPE_MENU_ID_RESUME], w, h, w_pos, h_pos);
		h_pos += h + h / 10;
	}

	// cursor
	tex_cursor.res_img = resource_manager_getTextureFromPath("{scale_mode:linear}images/gui/button/cursor.png");
	ret = GUI_QueryTexture(tex_cursor.res_img, NULL, NULL, &cur_w, &cur_h);
	if (ret == 0) {
		cur_w_pos = cur_w_pos - cur_w - cur_w / 2;
		tex_cursor.src_rect = { 0, 0, cur_w, cur_h };
		menu_items[MENU_ITEM_RESUME].tex_info_id = SCENE_ESCAPE_MENU_ID_RESUME;
		menu_items[MENU_ITEM_RESUME].dst_rect = VIEW_SCALE_RECT(cur_w_pos, cur_h_pos, cur_w, cur_h);
		menu_items[MENU_ITEM_RESUME].dst_rect_base = { cur_w_pos, cur_h_pos, cur_w, cur_h };
		menu_items[MENU_ITEM_RESUME].func = menu_resume;
		cur_h_pos += h + h / 10;
	}

	// menu2 ...
	tex_info[SCENE_ESCAPE_MENU_ID_LOAD].res_img = resource_manager_getTextureFromPath("{scale_mode:linear}images/gui/top_menu/load.png");
	ret = GUI_QueryTexture(tex_info[SCENE_ESCAPE_MENU_ID_LOAD].res_img, NULL, NULL, &w, &h);
	if (ret == 0) {
		GUI_tex_info_init_rect(&tex_info[SCENE_ESCAPE_MENU_ID_LOAD], w, h, w_pos, h_pos);
		menu_items[MENU_ITEM_LOAD].tex_info_id = SCENE_ESCAPE_MENU_ID_LOAD;
		menu_items[MENU_ITEM_LOAD].dst_rect = VIEW_SCALE_RECT(cur_w_pos, cur_h_pos, cur_w, cur_h);
		menu_items[MENU_ITEM_LOAD].dst_rect_base = { cur_w_pos, cur_h_pos, cur_w, cur_h };
		menu_items[MENU_ITEM_LOAD].func = menu_load;
		h_pos += h + h / 10;
		cur_h_pos += h + h / 10;
	}

	tex_info[SCENE_ESCAPE_MENU_ID_SETTINGS].res_img = resource_manager_getTextureFromPath("{scale_mode:linear}images/gui/top_menu/settings.png");
	ret = GUI_QueryTexture(tex_info[SCENE_ESCAPE_MENU_ID_SETTINGS].res_img, NULL, NULL, &w, &h);
	if (ret == 0) {
		GUI_tex_info_init_rect(&tex_info[SCENE_ESCAPE_MENU_ID_SETTINGS], w, h, w_pos, h_pos);
		menu_items[MENU_ITEM_SETTINGS].tex_info_id = SCENE_ESCAPE_MENU_ID_SETTINGS;
		menu_items[MENU_ITEM_SETTINGS].dst_rect = VIEW_SCALE_RECT(cur_w_pos, cur_h_pos, cur_w, cur_h);
		menu_items[MENU_ITEM_SETTINGS].dst_rect_base = { cur_w_pos, cur_h_pos, cur_w, cur_h };
		menu_items[MENU_ITEM_SETTINGS].func = menu_settings;
		h_pos += h + h / 10;
		cur_h_pos += h + h / 10;
	}

	tex_info[SCENE_ESCAPE_MENU_ID_EXIT].res_img = resource_manager_getTextureFromPath("{scale_mode:linear}images/gui/top_menu/exit.png");
	ret = GUI_QueryTexture(tex_info[SCENE_ESCAPE_MENU_ID_EXIT].res_img, NULL, NULL, &w, &h);
	if (ret == 0) {
		GUI_tex_info_init_rect(&tex_info[SCENE_ESCAPE_MENU_ID_EXIT], w, h, w_pos, h_pos);
		menu_items[MENU_ITEM_EXIT].tex_info_id = SCENE_ESCAPE_MENU_ID_EXIT;
		menu_items[MENU_ITEM_EXIT].dst_rect = VIEW_SCALE_RECT(cur_w_pos, cur_h_pos, cur_w, cur_h);
		menu_items[MENU_ITEM_EXIT].dst_rect_base = { cur_w_pos, cur_h_pos, cur_w, cur_h };
		menu_items[MENU_ITEM_EXIT].func = menu_exit;
		h_pos += h + h / 10;
		cur_h_pos += h + h / 10;
	}
}

void scene_escape_menu_init()
{
	// set stat
	scene_stat = SCENE_STAT_NONE;

	// set fuctions
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
	resource_manager_load_dat("scenes/scene_escape_menu.dat");

	// set texture position
	tex_info_init();
}

SceneManagerFunc* scene_escape_menu_get_func()
{
	return &scene_func;
}
