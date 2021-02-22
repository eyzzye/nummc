#include "game_common.h"
#include "gui_common.h"
#include "dialog_select_profile.h"

#include "game_key_event.h"
#include "game_mouse_event.h"
#include "game_window.h"
#include "resource_manager.h"
#include "sound_manager.h"
#include "game_utils.h"
#include "game_save.h"

// draw data
static tex_info_t tex_info_message;

#define DIALOG_MESSAGE_ID_CANCEL  0
#define DIALOG_MESSAGE_ID_OK      1
#define DIALOG_MESSAGE_ID_END     2

static tex_info_t tex_info[DIALOG_MESSAGE_ID_END];

// slot cursor
static rect_region_t dialog_bg;

// button data
#define BUTTON_ITEM_CANCEL 0
#define BUTTON_ITEM_OK     1
#define BUTTON_ITEM_SIZE   2

static gui_item_t button_items[BUTTON_ITEM_SIZE];
static int button_index = 0;

// profile items
#define PROFILE_ITEM_SIZE  (RESOURCE_MANAGER_PROFILE_LIST_SIZE - 1) // exclude "unlock"
static tex_info_t tex_info_profile[PROFILE_ITEM_SIZE];
static int profile_item_index = 0;
static int current_profile_stat;

#define PROFILE_BUTTON_ITEM_ID_LEFT  0
#define PROFILE_BUTTON_ITEM_ID_RIGHT 1
#define PROFILE_BUTTON_ITEM_ID_SIZE  2
static tex_info_t tex_info_profile_button[PROFILE_BUTTON_ITEM_ID_SIZE];

#define PROFILE_BUTTON_ITEM_LEFT  0
#define PROFILE_BUTTON_ITEM_RIGHT 1
#define PROFILE_BUTTON_ITEM_SIZE  2
static gui_item_t profile_button_items[PROFILE_BUTTON_ITEM_SIZE];

static void profile_button_left();
static void profile_button_right();

// GUI stat
#define GUI_ITEM_GROUP_ID_PROFILE  0
#define GUI_ITEM_GROUP_ID_SELECT   1
#define GUI_ITEM_GROUP_ID_SIZE     2

static rect_region_t group_region_profile;
static int gui_active_group_id;
static int gui_active_button_index;

// extern display stat
bool g_dialog_select_profile_enable;

// button functions
static void profile_button_left() {
	sound_manager_play(resource_manager_getChunkFromPath("sounds/sfx_click1.ogg"), SOUND_MANAGER_CH_SFX2);
	profile_item_index -= 1;
	if (profile_item_index < 0) profile_item_index = (PROFILE_ITEM_SIZE - 1);
}
static void profile_button_right() {
	sound_manager_play(resource_manager_getChunkFromPath("sounds/sfx_click1.ogg"), SOUND_MANAGER_CH_SFX2);
	profile_item_index += 1;
	if (profile_item_index > PROFILE_ITEM_SIZE - 1) { profile_item_index = 0; }
}

// init draw items
static void tex_info_init_message(std::string message)
{
	int w, h;
	int w_pos = 0, h_pos = 0;

	int dialog_top = SCREEN_HEIGHT * 2 / 5 / 2;
	int dialog_left = SCREEN_WIDTH * 2 / 5 / 2;

	// message
	tex_info_message.res_img = resource_manager_getFontTextureFromPath(message);
	int ret = GUI_QueryTexture(tex_info_message.res_img, NULL, NULL, &w, &h);
	if (ret == 0) {
		w_pos = dialog_left + 20;
		h_pos = dialog_top + 20;
		GUI_tex_info_init_rect(&tex_info_message, w, h, w_pos, h_pos);
	}
}

static void tex_info_init()
{
	int w, h;
	int w_pos = 0, h_pos = 0;

	// dialog background
	int dialog_w = SCREEN_WIDTH * 3 / 5;
	int dialog_h = SCREEN_HEIGHT * 3 / 5;
	int dialog_top = SCREEN_HEIGHT * 2 / 5 / 2;
	int dialog_left = SCREEN_WIDTH * 2 / 5 / 2;
	int dialog_right = dialog_left + dialog_w;
	int dialog_bottom = dialog_top + dialog_h;

	// background
	GUI_rect_region_init_rect(&dialog_bg, dialog_left, dialog_top, dialog_w, dialog_h);

	// button (right binding [cancel] [ok])
	tex_info[DIALOG_MESSAGE_ID_OK].res_img = resource_manager_getTextureFromPath("{scale_mode:linear}images/gui/button/ok.png");
	int ret = GUI_QueryTexture(tex_info[DIALOG_MESSAGE_ID_OK].res_img, NULL, NULL, &w, &h);
	if (ret == 0) {
		w_pos = dialog_right - w - 10;
		h_pos = dialog_bottom - h - 10;
		GUI_tex_info_init_rect(&tex_info[DIALOG_MESSAGE_ID_OK], w, h, w_pos, h_pos);
		button_items[BUTTON_ITEM_OK].tex_info_id = DIALOG_MESSAGE_ID_OK;
		button_items[BUTTON_ITEM_OK].dst_rect = tex_info[DIALOG_MESSAGE_ID_OK].dst_rect;
		button_items[BUTTON_ITEM_OK].dst_rect_base = tex_info[DIALOG_MESSAGE_ID_OK].dst_rect_base;
		button_items[BUTTON_ITEM_OK].func = NULL;
	}

	// profile img
	int profile_stat_flag = 0x00000001 << 1;
	for (int prof_i = 0; prof_i < PROFILE_ITEM_SIZE; prof_i++) {
		std::string profile_img = g_resource_manager_profile[0].portrait_img_path; // "unlock"
		tex_info_profile[prof_i].res_img = resource_manager_getTextureFromPath("{scale_mode:linear}" + profile_img);
		if (current_profile_stat & profile_stat_flag) { // enable charactor
			tex_info_profile[prof_i].res_img = resource_manager_getTextureFromPath(g_resource_manager_profile[prof_i + 1].portrait_img_path);
		}
		int ret = GUI_QueryTexture(tex_info_profile[prof_i].res_img, NULL, NULL, &w, &h);
		if (ret == 0) {
			w_pos = dialog_left + dialog_w / 2 - w / 2;
			h_pos = dialog_top + dialog_h / 2 - h / 2;
			GUI_tex_info_init_rect(&tex_info_profile[prof_i], w, h, w_pos, h_pos);
		}
		profile_stat_flag <<= 1;
	}

	// set group region
	int group_region_profile_top    = h_pos;
	int group_region_profile_left   = dialog_left + dialog_w / 6;
	int group_region_profile_right  = dialog_right - dialog_w / 6;
	int group_region_profile_bottom = h_pos + h;
	int group_region_profile_w = group_region_profile_right - group_region_profile_left;
	int group_region_profile_h = group_region_profile_bottom - group_region_profile_top;
	GUI_rect_region_init_rect(&group_region_profile,
		group_region_profile_left, group_region_profile_top, group_region_profile_w, group_region_profile_h);

	// left button
	tex_info_profile_button[PROFILE_BUTTON_ITEM_ID_LEFT].res_img = resource_manager_getTextureFromPath("{scale_mode:linear}images/gui/button/cursor_l.png");
	ret = GUI_QueryTexture(tex_info_profile_button[PROFILE_BUTTON_ITEM_ID_LEFT].res_img, NULL, NULL, &w, &h);
	if (ret == 0) {
		w_pos = dialog_left + dialog_w / 5;
		h_pos = dialog_top + dialog_h / 2;
		GUI_tex_info_init_rect(&tex_info_profile_button[PROFILE_BUTTON_ITEM_ID_LEFT], w, h, w_pos, h_pos);
		profile_button_items[PROFILE_BUTTON_ITEM_LEFT].tex_info_id = PROFILE_BUTTON_ITEM_ID_LEFT;
		profile_button_items[PROFILE_BUTTON_ITEM_LEFT].dst_rect = tex_info_profile_button[PROFILE_BUTTON_ITEM_ID_LEFT].dst_rect;
		profile_button_items[PROFILE_BUTTON_ITEM_LEFT].dst_rect_base = tex_info_profile_button[PROFILE_BUTTON_ITEM_ID_LEFT].dst_rect_base;
		profile_button_items[PROFILE_BUTTON_ITEM_LEFT].func = NULL;
	}

	// right button
	tex_info_profile_button[PROFILE_BUTTON_ITEM_ID_RIGHT].res_img = resource_manager_getTextureFromPath("{scale_mode:linear}images/gui/button/cursor.png");
	ret = GUI_QueryTexture(tex_info_profile_button[PROFILE_BUTTON_ITEM_ID_RIGHT].res_img, NULL, NULL, &w, &h);
	if (ret == 0) {
		w_pos = dialog_right - dialog_w / 5 - w;
		h_pos = dialog_top + dialog_h / 2;
		GUI_tex_info_init_rect(&tex_info_profile_button[PROFILE_BUTTON_ITEM_ID_RIGHT], w, h, w_pos, h_pos);
		profile_button_items[PROFILE_BUTTON_ITEM_RIGHT].tex_info_id = PROFILE_BUTTON_ITEM_ID_RIGHT;
		profile_button_items[PROFILE_BUTTON_ITEM_RIGHT].dst_rect = tex_info_profile_button[PROFILE_BUTTON_ITEM_ID_RIGHT].dst_rect;
		profile_button_items[PROFILE_BUTTON_ITEM_RIGHT].dst_rect_base = tex_info_profile_button[PROFILE_BUTTON_ITEM_ID_RIGHT].dst_rect_base;
		profile_button_items[PROFILE_BUTTON_ITEM_RIGHT].func = NULL;
	}
}

void dialog_select_profile_init() {
	// do nothing
}

void dialog_select_profile_reset(std::string message, void_func* yes_func)
{
	game_save_get_config_unlock_stat(&current_profile_stat);
	profile_item_index = 0;

	tex_info_init_message(message);
	tex_info_init();

	GUI_rect_region_reset(&dialog_bg);
	GUI_tex_info_reset(tex_info, DIALOG_MESSAGE_ID_END);
	GUI_tex_info_reset(&tex_info_message);
	GUI_gui_item_reset(button_items, BUTTON_ITEM_SIZE);
	GUI_rect_region_reset(&group_region_profile);

	// set button
	button_items[BUTTON_ITEM_OK].mouse_stat = 0;
	button_items[BUTTON_ITEM_OK].func = yes_func;
	button_index = BUTTON_ITEM_OK;

	// set profile
	profile_button_items[PROFILE_BUTTON_ITEM_LEFT].mouse_stat = 0;
	profile_button_items[PROFILE_BUTTON_ITEM_LEFT].func = profile_button_left;
	profile_button_items[PROFILE_BUTTON_ITEM_RIGHT].mouse_stat = 0;
	profile_button_items[PROFILE_BUTTON_ITEM_RIGHT].func = profile_button_right;

	g_dialog_select_profile_enable = false;
}

void dialog_select_profile_set_enable(bool enable)
{
	if (enable) {
		// get current mouse position
		int x = 0, y = 0;
		gui_active_button_index = -1;
		game_mouse_event_get_motion(&x, &y);

		for (int i = BUTTON_ITEM_OK; i < BUTTON_ITEM_SIZE; i++) {
			if (game_utils_decision_internal(&tex_info[button_items[i].tex_info_id].dst_rect, x, y)) {
				button_index = i;
				button_items[i].mouse_stat = GUI_BUTTON_ACTIVE;
				gui_active_button_index = i;
				gui_active_group_id = GUI_ITEM_GROUP_ID_SELECT;
			}
			else {
				button_items[i].mouse_stat = 0;
			}
		}

		if (game_utils_decision_internal(&group_region_profile.dst_rect, x, y)) {
			gui_active_group_id = GUI_ITEM_GROUP_ID_PROFILE;
		}
		for (int i = 0; i < PROFILE_BUTTON_ITEM_SIZE; i++) {
			if (game_utils_decision_internal(&tex_info_profile_button[profile_button_items[i].tex_info_id].dst_rect, x, y)) {
				profile_button_items[i].mouse_stat = GUI_BUTTON_ACTIVE;
				gui_active_button_index = i;
				gui_active_group_id = GUI_ITEM_GROUP_ID_PROFILE;
			}
			else {
				profile_button_items[i].mouse_stat = 0;
			}
		}
	}

	g_dialog_select_profile_enable = enable;
}

void dialog_select_profile_event() {
	if (g_dialog_select_profile_enable) {
		bool select_snd_on = false;

		// key event
		if (game_key_event_get(SDL_SCANCODE_LEFT, GUI_SELECT_WAIT_TIMER)) {
			profile_button_left();
		}
		if (game_key_event_get(SDL_SCANCODE_RIGHT, GUI_SELECT_WAIT_TIMER)) {
			profile_button_right();
		}
		if (game_key_event_get(SDL_SCANCODE_RETURN, GUI_SELECT_WAIT_TIMER)) {
			(*button_items[button_index].func)();
		}

		// mouse event
		int x = 0, y = 0;
		if (game_mouse_event_get_motion(&x, &y)) {
			gui_active_button_index = -1;			
			for (int i = BUTTON_ITEM_OK; i < BUTTON_ITEM_SIZE; i++) {
				if (game_utils_decision_internal(&tex_info[button_items[i].tex_info_id].dst_rect, x, y)) {
					if ((button_index != i) || (gui_active_group_id != GUI_ITEM_GROUP_ID_SELECT)) select_snd_on = true;
					button_index = i;
					button_items[i].mouse_stat = GUI_BUTTON_ACTIVE;
					gui_active_group_id = GUI_ITEM_GROUP_ID_SELECT;
					gui_active_button_index = i;
				}
				else {
					button_items[i].mouse_stat = 0;
				}
			}

			if (game_utils_decision_internal(&group_region_profile.dst_rect, x, y)) {
				gui_active_group_id = GUI_ITEM_GROUP_ID_PROFILE;
			}
			for (int i = 0; i < PROFILE_BUTTON_ITEM_SIZE; i++) {
				if (game_utils_decision_internal(&tex_info_profile_button[profile_button_items[i].tex_info_id].dst_rect, x, y)) {
					if (profile_button_items[i].mouse_stat != GUI_BUTTON_ACTIVE) select_snd_on = true;
					profile_button_items[i].mouse_stat = GUI_BUTTON_ACTIVE;
					gui_active_button_index = i;
					gui_active_group_id = GUI_ITEM_GROUP_ID_PROFILE;
				}
				else {
					profile_button_items[i].mouse_stat = 0;
				}
			}
		}

		Uint32 mouse_left_stat = game_mouse_event_get(GAME_MOUSE_LEFT);
		if ((mouse_left_stat & GAME_MOUSE_CLICK) && (gui_active_button_index >= 0)) {
			if (gui_active_group_id == GUI_ITEM_GROUP_ID_SELECT) {
				if (button_items[gui_active_button_index].mouse_stat & GUI_BUTTON_ACTIVE) {
					button_items[gui_active_button_index].mouse_stat |= GUI_BUTTON_CLICK;
				}
			}
			else if (gui_active_group_id == GUI_ITEM_GROUP_ID_PROFILE) {
				if (profile_button_items[gui_active_button_index].mouse_stat & GUI_BUTTON_ACTIVE) {
					profile_button_items[gui_active_button_index].mouse_stat |= GUI_BUTTON_CLICK;
				}
			}
		}
		if ((mouse_left_stat & GAME_MOUSE_RELEASE) && (gui_active_button_index >= 0)) {
			if (gui_active_group_id == GUI_ITEM_GROUP_ID_SELECT) {
				if (button_items[gui_active_button_index].mouse_stat == (GUI_BUTTON_ACTIVE | GUI_BUTTON_CLICK)) {
					button_items[gui_active_button_index].mouse_stat = 0;
					(*button_items[gui_active_button_index].func)();
				}
			}
			else if (gui_active_group_id == GUI_ITEM_GROUP_ID_PROFILE) {
				if (profile_button_items[gui_active_button_index].mouse_stat == (GUI_BUTTON_ACTIVE | GUI_BUTTON_CLICK)) {
					profile_button_items[gui_active_button_index].mouse_stat &= ~GUI_BUTTON_CLICK;
					(*profile_button_items[gui_active_button_index].func)();
				}
			}
		}

		// play snd
		if (select_snd_on) {
			sound_manager_play(resource_manager_getChunkFromPath("sounds/sfx_select1.ogg"), SOUND_MANAGER_CH_SFX1);
		}
	}
}

void dialog_select_profile_draw() {
	if (g_dialog_select_profile_enable) {
		// draw background
		SDL_SetRenderDrawColor(g_ren, 255, 255, 255, 255);
		SDL_RenderFillRect(g_ren, &dialog_bg.dst_rect);

		// profile left/right button
		GUI_tex_info_draw(&tex_info_profile[profile_item_index]);
		GUI_tex_info_draw(&tex_info_profile_button[PROFILE_BUTTON_ITEM_ID_LEFT]);
		GUI_tex_info_draw(&tex_info_profile_button[PROFILE_BUTTON_ITEM_ID_RIGHT]);

		// draw button
		GUI_tex_info_draw(&tex_info[DIALOG_MESSAGE_ID_OK]);
		GUI_tex_info_draw(&tex_info_message);

		// draw selected
		if (gui_active_group_id == GUI_ITEM_GROUP_ID_SELECT) {
			SDL_SetRenderDrawColor(g_ren, 0, 0, 0, 32);
			SDL_RenderFillRect(g_ren, &button_items[button_index].dst_rect);
		}
		else if ((gui_active_group_id == GUI_ITEM_GROUP_ID_PROFILE) && (gui_active_button_index != -1)) {
			SDL_SetRenderDrawColor(g_ren, 0, 0, 0, 32);
			SDL_RenderFillRect(g_ren, &tex_info_profile_button[gui_active_button_index].dst_rect);
		}

		SDL_SetRenderDrawColor(g_ren, 0, 0, 0, 255);
		SDL_RenderDrawRect(g_ren, &group_region_profile.dst_rect);
	}
}

int dialog_select_profile_get_current_profile_index()
{
	return (profile_item_index + 1);
}
