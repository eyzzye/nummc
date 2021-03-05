#include "game_common.h"
#include "gui_common.h"
#include "dialog_message.h"

#include "game_key_event.h"
#include "game_mouse_event.h"
#include "game_window.h"
#include "resource_manager.h"
#include "sound_manager.h"
#include "game_utils.h"

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

// GUI stat
static int dialog_type;
static int gui_active_button_index;

// extern display stat
bool g_dialog_message_enable;

// init draw items
static void tex_info_init_message(const char* message)
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
	int dialog_w      = SCREEN_WIDTH  * 3 / 5;
	int dialog_h      = SCREEN_HEIGHT * 3 / 5;
	int dialog_top    = SCREEN_HEIGHT * 2 / 5 / 2;
	int dialog_left   = SCREEN_WIDTH  * 2 / 5 / 2;
	int dialog_right  = dialog_left + dialog_w;
	int dialog_bottom = dialog_top + dialog_h;

	// background
	GUI_rect_region_init_rect(&dialog_bg, dialog_left, dialog_top, dialog_w, dialog_h);

	// button (right binding [cancel] [ok])
	if ((dialog_type == DIALOG_MSG_TYPE_CANCEL_OK) || (dialog_type == DIALOG_MSG_TYPE_OK_ONLY)) {
		tex_info[DIALOG_MESSAGE_ID_OK].res_img = resource_manager_getTextureFromPath("{scale_mode:linear}images/gui/button/ok.png");
	}
	else if (dialog_type == DIALOG_MSG_TYPE_NO_YES) {
		tex_info[DIALOG_MESSAGE_ID_OK].res_img = resource_manager_getTextureFromPath("{scale_mode:linear}images/gui/button/yes.png");
	}
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

	if ((dialog_type == DIALOG_MSG_TYPE_CANCEL_OK) || (dialog_type == DIALOG_MSG_TYPE_OK_ONLY)) {
		tex_info[DIALOG_MESSAGE_ID_CANCEL].res_img = resource_manager_getTextureFromPath("{scale_mode:linear}images/gui/button/cancel.png");
	}
	else if (dialog_type == DIALOG_MSG_TYPE_NO_YES) {
		tex_info[DIALOG_MESSAGE_ID_CANCEL].res_img = resource_manager_getTextureFromPath("{scale_mode:linear}images/gui/button/no.png");
	}
	ret = GUI_QueryTexture(tex_info[DIALOG_MESSAGE_ID_CANCEL].res_img, NULL, NULL, &w, &h);
	if (ret == 0) {
		w_pos = w_pos - w - 10;
		//h_pos = dialog_bottom - h - 10;
		GUI_tex_info_init_rect(&tex_info[DIALOG_MESSAGE_ID_CANCEL], w, h, w_pos, h_pos);
		button_items[BUTTON_ITEM_CANCEL].tex_info_id = DIALOG_MESSAGE_ID_CANCEL;
		button_items[BUTTON_ITEM_CANCEL].dst_rect = tex_info[DIALOG_MESSAGE_ID_CANCEL].dst_rect;
		button_items[BUTTON_ITEM_CANCEL].dst_rect_base = tex_info[DIALOG_MESSAGE_ID_CANCEL].dst_rect_base;
		button_items[BUTTON_ITEM_CANCEL].func = NULL;
	}
}

void dialog_message_init() {
	// do nothing
}

void dialog_message_reset(const char* message, void_func* no_func, void_func* yes_func, int dialog_type_)
{
	dialog_type = dialog_type_;
	tex_info_init_message(message);
	tex_info_init();

	GUI_rect_region_reset(&dialog_bg);
	GUI_tex_info_reset(tex_info, DIALOG_MESSAGE_ID_END);
	GUI_tex_info_reset(&tex_info_message);
	GUI_gui_item_reset(button_items, BUTTON_ITEM_SIZE);

	if (dialog_type == DIALOG_MSG_TYPE_CANCEL_OK) {
		button_items[BUTTON_ITEM_CANCEL].mouse_stat = 0;
		button_items[BUTTON_ITEM_CANCEL].func = no_func;
		button_items[BUTTON_ITEM_OK].mouse_stat = 0;
		button_items[BUTTON_ITEM_OK].func = yes_func;
	}
	else if (dialog_type == DIALOG_MSG_TYPE_OK_ONLY) {
		button_items[BUTTON_ITEM_OK].mouse_stat = 0;
		button_items[BUTTON_ITEM_OK].func = yes_func;
	}
	else if (dialog_type == DIALOG_MSG_TYPE_NO_YES) {
		button_items[BUTTON_ITEM_CANCEL].mouse_stat = 0;
		button_items[BUTTON_ITEM_CANCEL].func = no_func;
		button_items[BUTTON_ITEM_OK].mouse_stat = 0;
		button_items[BUTTON_ITEM_OK].func = yes_func;
	}

	button_index = (dialog_type == DIALOG_MSG_TYPE_OK_ONLY) ? BUTTON_ITEM_OK : BUTTON_ITEM_CANCEL;
	g_dialog_message_enable = false;
}

void dialog_message_set_enable(bool enable)
{
	if (enable) {
		// get current mouse position
		int x = 0, y = 0;
		gui_active_button_index = -1;
		game_mouse_event_get_motion(&x, &y);

		int i = 0;
		if (dialog_type == DIALOG_MSG_TYPE_OK_ONLY) i = BUTTON_ITEM_OK;
		for ( ; i < BUTTON_ITEM_SIZE; i++) {
			if (game_utils_decision_internal(&tex_info[button_items[i].tex_info_id].dst_rect, x, y)) {
				button_index = i;
				button_items[i].mouse_stat = GUI_BUTTON_ACTIVE;
				gui_active_button_index = i;
			}
			else {
				button_items[i].mouse_stat = 0;
			}
		}
	}

	g_dialog_message_enable = enable;
}

void dialog_message_event() {
	if (g_dialog_message_enable) {
		bool select_snd_on = false;

		// key event
		if (game_key_event_get(SDL_SCANCODE_LEFT, GUI_SELECT_WAIT_TIMER)) {
			if (dialog_type != DIALOG_MSG_TYPE_OK_ONLY) {
				select_snd_on = true;
				button_index -= 1;
				if (button_index < 0) button_index = BUTTON_ITEM_SIZE - 1;
			}
		}
		if (game_key_event_get(SDL_SCANCODE_RIGHT, GUI_SELECT_WAIT_TIMER)) {
			if (dialog_type != DIALOG_MSG_TYPE_OK_ONLY) {
				select_snd_on = true;
				button_index += 1;
				if (button_index >= BUTTON_ITEM_SIZE) button_index = 0;
			}
		}
		if (game_key_event_get(SDL_SCANCODE_RETURN, GUI_SELECT_WAIT_TIMER)) {
			(*button_items[button_index].func)();
		}

		// mouse event
		int x = 0, y = 0;
		if (game_mouse_event_get_motion(&x, &y)) {
			gui_active_button_index = -1;

			int i = 0;
			if (dialog_type == DIALOG_MSG_TYPE_OK_ONLY) i = BUTTON_ITEM_OK;
			for ( ; i < BUTTON_ITEM_SIZE; i++) {
				if (game_utils_decision_internal(&tex_info[button_items[i].tex_info_id].dst_rect, x, y)) {
					if (button_index != i) select_snd_on = true;
					button_index = i;
					button_items[i].mouse_stat = GUI_BUTTON_ACTIVE;
					gui_active_button_index = i;
				}
				else {
					button_items[i].mouse_stat = 0;
				}
			}
		}

		Uint32 mouse_left_stat = game_mouse_event_get(GAME_MOUSE_LEFT);
		if ((mouse_left_stat & GAME_MOUSE_CLICK) && (gui_active_button_index >= 0)) {
			if (button_items[gui_active_button_index].mouse_stat & GUI_BUTTON_ACTIVE) {
				button_items[gui_active_button_index].mouse_stat |= GUI_BUTTON_CLICK;
			}
		}
		if ((mouse_left_stat & GAME_MOUSE_RELEASE) && (gui_active_button_index >= 0)) {
			if (button_items[gui_active_button_index].mouse_stat == (GUI_BUTTON_ACTIVE | GUI_BUTTON_CLICK)) {
				button_items[gui_active_button_index].mouse_stat = 0;
				(*button_items[gui_active_button_index].func)();
			}
		}

		// play snd
		if (select_snd_on) {
			sound_manager_play(resource_manager_getChunkFromPath("sounds/sfx_select1.ogg"), SOUND_MANAGER_CH_SFX1);
		}
	}
}

void dialog_message_draw() {
	if (g_dialog_message_enable) {
		// draw background
		SDL_SetRenderDrawColor(g_ren, 255, 255, 255, 255);
		SDL_RenderFillRect(g_ren, &dialog_bg.dst_rect);

		// draw button
		if (dialog_type != DIALOG_MSG_TYPE_OK_ONLY) GUI_tex_info_draw(&tex_info[DIALOG_MESSAGE_ID_CANCEL]);
		GUI_tex_info_draw(&tex_info[DIALOG_MESSAGE_ID_OK]);
		GUI_tex_info_draw(&tex_info_message);

		// draw selected
		SDL_SetRenderDrawColor(g_ren, 0, 0, 0, 32);
		SDL_RenderFillRect(g_ren, &button_items[button_index].dst_rect);
	}
}
