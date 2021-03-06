#include "game_common.h"
#include "gui_common.h"
#include "scene_manager.h"
#include "scene_save_menu.h"

#include "resource_manager.h"
#include "sound_manager.h"
#include "game_key_event.h"
#include "game_mouse_event.h"
#include "game_window.h"
#include "game_utils.h"
#include "game_log.h"
#include "game_save.h"
#include "dialog_message.h"
#include "dialog_select_profile.h"
#include "scene_loading.h"
#include "scene_play_stage.h"
#include "scene_play_story.h"

// draw static data
#define SCENE_SAVE_MENU_ID_LABEL_SLOT1  0
#define SCENE_SAVE_MENU_ID_LABEL_SLOT2  1
#define SCENE_SAVE_MENU_ID_LABEL_SLOT3  2
#define SCENE_SAVE_MENU_ID_LABEL_SLOT4  3
#define SCENE_SAVE_MENU_ID_LABEL_SLOT5  4
#define SCENE_SAVE_MENU_ID_LABEL_SLOT6  5
#define SCENE_SAVE_MENU_ID_LABEL_SLOT7  6
#define SCENE_SAVE_MENU_ID_LABEL_SLOT8  7
#define SCENE_SAVE_MENU_ID_DELETE       8
#define SCENE_SAVE_MENU_ID_CANCEL       9
#define SCENE_SAVE_MENU_ID_OK          10
#define SCENE_SAVE_MENU_ID_END         11

static tex_info_t tex_info[SCENE_SAVE_MENU_ID_END];

// title data
#define SCENE_SAVE_MENU_ID_TITLE_SAVE       0
#define SCENE_SAVE_MENU_ID_TITLE_NEW_GAME   1
#define SCENE_SAVE_MENU_ID_TITLE_LOAD       2
#define SCENE_SAVE_MENU_ID_TITLE_END        3

static tex_info_t tex_info_title[SCENE_SAVE_MENU_ID_TITLE_END];

// description data
#define SCENE_SAVE_MENU_ID_DESCRIPTION_1    0
#define SCENE_SAVE_MENU_ID_DESCRIPTION_2    1
#define SCENE_SAVE_MENU_ID_DESCRIPTION_3    2
#define SCENE_SAVE_MENU_ID_DESCRIPTION_END  3

static tex_info_t tex_info_description[SCENE_SAVE_MENU_ID_DESCRIPTION_END];

// button data
#define BUTTON_ITEM_DELETE  0
#define BUTTON_ITEM_CANCEL  1
#define BUTTON_ITEM_OK      2
#define BUTTON_ITEM_SIZE    3

static gui_item_t button_items[BUTTON_ITEM_SIZE];
static int button_index = 0;

static void button_delete();
static void button_cancel();
static void button_ok();

// slot data
#define SLOT_ITEM_1    0
#define SLOT_ITEM_2    1
#define SLOT_ITEM_3    2
#define SLOT_ITEM_4    3
#define SLOT_ITEM_5    4
#define SLOT_ITEM_6    5
#define SLOT_ITEM_7    6
#define SLOT_ITEM_8    7
#define SLOT_ITEM_END  8

static tex_info_t tex_info_slot_icon[SLOT_ITEM_END];
static tex_info_t tex_info_slot_stage[SLOT_ITEM_END];
static tex_info_t tex_info_slot_timestamp[SLOT_ITEM_END];
static gui_item_t slot_items[SLOT_ITEM_END];
static int slot_index = 0;

static void slot_select();

// slot cursor
static rect_region_t cursor_item;
static int cursor_index = 0;

// dialog callback func
#define DIALOG_MESSAGE_RET_CANCEL 0
#define DIALOG_MESSAGE_RET_OK     1

static int dialog_message_ret = 0;
static void dialog_message_delete_ok();
static void dialog_message_delete_cancel();
static void dialog_message_ok();
static void dialog_message_cancel();
static void dialog_select_profile_ok();

// GUI stat
#define GUI_ITEM_GROUP_ID_SLOT   0
#define GUI_ITEM_GROUP_ID_SELECT 1
#define GUI_ITEM_GROUP_ID_SIZE   2

static int gui_active_group_id;
static int gui_active_button_index;

// event functions
static SceneManagerFunc scene_func;
static int scene_stat;
static int return_scene_id;
static int display_title_type;

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

	bool select_snd_on = false;
	bool click_snd_on = false;
	if (g_dialog_message_enable) {
		dialog_message_event();
		return; // recive only dialog key
	}
	else if (g_dialog_select_profile_enable) {
		dialog_select_profile_event();
		return; // recive only dialog key
	}

	if (game_key_event_get(SDL_SCANCODE_ESCAPE, GUI_SELECT_WAIT_TIMER)) {
		button_cancel();
	}

	if (game_key_event_get(SDL_SCANCODE_TAB, GUI_SELECT_WAIT_TIMER)) {
		select_snd_on = true;
		gui_active_group_id += 1;
		if (gui_active_group_id >= GUI_ITEM_GROUP_ID_SIZE) gui_active_group_id = 0;
	}
	if (game_key_event_get(SDL_SCANCODE_UP, GUI_SELECT_WAIT_TIMER)) {
		if (gui_active_group_id == GUI_ITEM_GROUP_ID_SLOT) {
			if (slot_index >= 4) {
				select_snd_on = true;
				slot_index -= 4;
			}
		}
		else if (gui_active_group_id == GUI_ITEM_GROUP_ID_SELECT) {
			select_snd_on = true;
			gui_active_group_id -= 1;
			if (slot_index < 4) slot_index += 4;
		}
	}
	if (game_key_event_get(SDL_SCANCODE_DOWN, GUI_SELECT_WAIT_TIMER)) {
		if (gui_active_group_id == GUI_ITEM_GROUP_ID_SLOT) {
			select_snd_on = true;
			if (slot_index < 4) slot_index += 4;
			else gui_active_group_id += 1;
		}
	}

	if (game_key_event_get(SDL_SCANCODE_LEFT, GUI_SELECT_WAIT_TIMER)) {
		if (gui_active_group_id == GUI_ITEM_GROUP_ID_SELECT) {
			select_snd_on = true;
			button_index -= 1;
			if (button_index < 0) button_index = BUTTON_ITEM_SIZE - 1;
		}
		else if (gui_active_group_id == GUI_ITEM_GROUP_ID_SLOT) {
			if (slot_index <= 0) { /* stay */ }
			else if (slot_index < 4) {
				select_snd_on = true;
				slot_index -= 1;
			}
			else if (slot_index == 4) { /* stay */ }
			else if (slot_index < 8) {
				select_snd_on = true;
				slot_index -= 1;
			}
		}
	}
	if (game_key_event_get(SDL_SCANCODE_RIGHT, GUI_SELECT_WAIT_TIMER)) {
		if (gui_active_group_id == GUI_ITEM_GROUP_ID_SELECT) {
			select_snd_on = true;
			button_index += 1;
			if (button_index >= BUTTON_ITEM_SIZE) button_index = 0;
		}
		else if (gui_active_group_id == GUI_ITEM_GROUP_ID_SLOT) {
			if (slot_index < 3) {
				select_snd_on = true;
				slot_index += 1;
			}
			else if (slot_index == 3) { /* stay */ }
			else if (slot_index < 7) {
				select_snd_on = true;
				slot_index += 1;
			}
			else if (slot_index >= 7) { /* stay */ }
		}
	}
	if (game_key_event_get(SDL_SCANCODE_RETURN, GUI_SELECT_WAIT_TIMER)) {
		if (gui_active_group_id == GUI_ITEM_GROUP_ID_SELECT) {
			(*button_items[button_index].func)();
		}
		else if (gui_active_group_id == GUI_ITEM_GROUP_ID_SLOT) {
			click_snd_on = true;
			(*slot_items[slot_index].func)();
		}
	}

	// mouse event
	int x = 0, y = 0;
	if (game_mouse_event_get_motion(&x, &y)) {
		gui_active_button_index = -1;
		for (int i = 0; i < BUTTON_ITEM_SIZE; i++) {
			if (game_utils_decision_internal(&tex_info[button_items[i].tex_info_id].dst_rect, x, y)) {
				if ((button_index != i) || (gui_active_group_id != GUI_ITEM_GROUP_ID_SELECT)) select_snd_on = true;
				button_index = i;
				button_items[i].mouse_stat = GUI_BUTTON_ACTIVE;
				gui_active_button_index = i;
				gui_active_group_id = GUI_ITEM_GROUP_ID_SELECT;
			}
			else {
				button_items[i].mouse_stat = 0;
			}
		}

		for (int i = 0; i < SLOT_ITEM_END; i++) {
			if (game_utils_decision_internal(&slot_items[i].dst_rect, x, y)) {
				if ((slot_index != i) || (gui_active_group_id != GUI_ITEM_GROUP_ID_SLOT)) select_snd_on = true;
				slot_index = i;
				slot_items[i].mouse_stat = GUI_BUTTON_ACTIVE;
				gui_active_button_index = i;
				gui_active_group_id = GUI_ITEM_GROUP_ID_SLOT;
			}
			else {
				slot_items[i].mouse_stat = 0;
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
		else if (gui_active_group_id == GUI_ITEM_GROUP_ID_SLOT) {
			if (slot_items[gui_active_button_index].mouse_stat & GUI_BUTTON_ACTIVE) {
				slot_items[gui_active_button_index].mouse_stat |= GUI_BUTTON_CLICK;
			}
		}
	}
	if ((mouse_left_stat & GAME_MOUSE_RELEASE) && (gui_active_button_index >= 0)) {
		if (gui_active_group_id == GUI_ITEM_GROUP_ID_SELECT) {
			if (button_items[gui_active_button_index].mouse_stat == (GUI_BUTTON_ACTIVE | GUI_BUTTON_CLICK)) {
				button_items[gui_active_button_index].mouse_stat &= ~GUI_BUTTON_CLICK;
				(*button_items[gui_active_button_index].func)();
			}
		}
		else if (gui_active_group_id == GUI_ITEM_GROUP_ID_SLOT) {
			if (slot_items[gui_active_button_index].mouse_stat == (GUI_BUTTON_ACTIVE | GUI_BUTTON_CLICK)) {
				click_snd_on = true;
				slot_items[gui_active_button_index].mouse_stat &= ~GUI_BUTTON_CLICK;
				(*slot_items[gui_active_button_index].func)();
			}
		}
	}

	// play snd
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
	SDL_SetRenderDrawColor(g_ren, 255, 255, 255, 255);
	SDL_RenderFillRect(g_ren, &g_screen_size);

	// title
	if (display_title_type == SCENE_SAVE_MENU_DISP_TYPE_SAVE) {
		GUI_tex_info_draw(&tex_info_title[SCENE_SAVE_MENU_ID_TITLE_SAVE]);
		GUI_tex_info_draw(&tex_info_description[SCENE_SAVE_MENU_ID_DESCRIPTION_1]);
	}
	else if (display_title_type == SCENE_SAVE_MENU_DISP_TYPE_NEW_GAME) {
		GUI_tex_info_draw(&tex_info_title[SCENE_SAVE_MENU_ID_TITLE_NEW_GAME]);
		GUI_tex_info_draw(&tex_info_description[SCENE_SAVE_MENU_ID_DESCRIPTION_2]);
	}
	else if (display_title_type == SCENE_SAVE_MENU_DISP_TYPE_LOAD) {
		GUI_tex_info_draw(&tex_info_title[SCENE_SAVE_MENU_ID_TITLE_LOAD]);
		GUI_tex_info_draw(&tex_info_description[SCENE_SAVE_MENU_ID_DESCRIPTION_3]);
	}

	// draw item
	GUI_tex_info_draw(tex_info, SCENE_SAVE_MENU_ID_END);

	// draw slot
	for (int i = 0; i < SLOT_ITEM_END; i++) {
		SDL_SetRenderDrawColor(g_ren, 0, 0, 0, 255);
		SDL_RenderDrawRect(g_ren, &slot_items[SLOT_ITEM_1 + i].dst_rect);

		GUI_tex_info_draw(&tex_info_slot_icon[i]);
		GUI_tex_info_draw(&tex_info_slot_stage[i]);
		GUI_tex_info_draw(&tex_info_slot_timestamp[i]);
	}

	// draw cursor
	SDL_SetRenderDrawColor(g_ren, 64, 64, 64, 255);
	SDL_RenderDrawRect(g_ren, &cursor_item.dst_rect);

	// draw selected
	if (gui_active_group_id == GUI_ITEM_GROUP_ID_SELECT) {
		SDL_SetRenderDrawColor(g_ren, 0, 0, 0, 32);
		SDL_RenderFillRect(g_ren, &button_items[button_index].dst_rect);
	}
	else if (gui_active_group_id == GUI_ITEM_GROUP_ID_SLOT) {
		SDL_SetRenderDrawColor(g_ren, 0, 0, 0, 32);
		SDL_RenderFillRect(g_ren, &slot_items[slot_index].dst_rect);
	}

	// dialog message
	if (g_dialog_message_enable) {
		// set background dark
		SDL_SetRenderDrawColor(g_ren, 0, 0, 0, 128);
		SDL_RenderFillRect(g_ren, &g_screen_size);

		// draw dialog
		dialog_message_draw();
	}
	// select profile dialog
	else if (g_dialog_select_profile_enable) {
		// set background dark
		SDL_SetRenderDrawColor(g_ren, 0, 0, 0, 128);
		SDL_RenderFillRect(g_ren, &g_screen_size);

		// draw dialog
		dialog_select_profile_draw();
	}

	SDL_RenderPresent(g_ren);
}
static void after_draw() {
	if (scene_stat != SCENE_STAT_ACTIVE) return;
}
static void load_event() {
	return_scene_id = scene_manager_get_scene_id();

	// resize
	GUI_tex_info_reset(tex_info, SCENE_SAVE_MENU_ID_END);
	GUI_tex_info_reset(tex_info_title, SCENE_SAVE_MENU_ID_TITLE_END);
	GUI_tex_info_reset(tex_info_description, SCENE_SAVE_MENU_ID_DESCRIPTION_END);
	GUI_gui_item_reset(button_items, BUTTON_ITEM_SIZE);
	GUI_gui_item_reset(slot_items, SLOT_ITEM_END);

	// load save data
	SDL_Rect* tmp_rect;
	int ret;
	int w, h;
	char slot_player[GAME_UTILS_STRING_NAME_BUF_SIZE];
	char slot_stage[GAME_UTILS_STRING_NAME_BUF_SIZE];
	char slot_timestamp[GAME_UTILS_STRING_NAME_BUF_SIZE];
	slot_index = 0;
	cursor_index = 0;
	bool set_slot_index_flg = false;
	for (int i = 0; i < SLOT_ITEM_END; i++) {
		game_save_get_config_slot(i, slot_player, slot_stage, slot_timestamp);

		char tmp_string[GAME_UTILS_STRING_CHAR_BUF_SIZE];
		int tmp_string_size = 0;
		if (display_title_type == SCENE_SAVE_MENU_DISP_TYPE_LOAD) {
			// select data slot
			if ((!set_slot_index_flg) && (slot_stage[0] != '\0')) {
				set_slot_index_flg = true;
				slot_index = i;
				cursor_index = i;
				slot_select(); // set cursor rect
			}
		}
		else {
			// select empty slot
			if ((!set_slot_index_flg) && (slot_stage[0] == '\0')) {
				set_slot_index_flg = true;
				slot_index = i;
				cursor_index = i;
				slot_select(); // set cursor rect
			}
		}

		tex_info_slot_icon[SLOT_ITEM_1 + i].res_img = resource_manager_getFontTextureFromPath(" ");
		for (int prof_i = 0; prof_i < RESOURCE_MANAGER_PROFILE_LIST_SIZE; prof_i++) {
			if (STRCMP_EQ(slot_player, g_resource_manager_profile[prof_i].name)) {
				//std::string icon_path = g_resource_manager_profile[prof_i].icon_img_path;
				char icon_path[GAME_UTILS_STRING_CHAR_BUF_SIZE];
				int icon_path_size = game_utils_string_cat(icon_path, (char*)"{ scale_mode:linear }", (char*)g_resource_manager_profile[prof_i].icon_img_path);
				tex_info_slot_icon[SLOT_ITEM_1 + i].res_img = resource_manager_getTextureFromPath(icon_path);
				break;
			}
		}
		ret = GUI_QueryTexture(tex_info_slot_icon[SLOT_ITEM_1 + i].res_img, NULL, NULL, &w, &h);
		if (ret == 0) {
			tex_info_slot_icon[SLOT_ITEM_1 + i].src_rect = { 0, 0, w, h };
			tmp_rect = &slot_items[SLOT_ITEM_1 + i].dst_rect_base;
			tex_info_slot_icon[SLOT_ITEM_1 + i].dst_rect_base = { tmp_rect->x + w / 4, tmp_rect->y + h / 4, w, h };
			GUI_tex_info_reset(&tex_info_slot_icon[SLOT_ITEM_1 + i]);
		}

		if (slot_stage[0] == '\0') {
			tex_info_slot_stage[SLOT_ITEM_1 + i].res_img = resource_manager_getFontTextureFromPath("(EMPTY)");
		}
		else {
			tmp_string_size = game_utils_string_cat(tmp_string, (char*)"STAGE", slot_stage);
			if (tmp_string_size <= 0) LOG_ERROR("Error: scene_save_menu load_event() get slot_stage\n");
			tex_info_slot_stage[SLOT_ITEM_1 + i].res_img = resource_manager_getFontTextureFromPath(tmp_string);
		}
		ret = GUI_QueryTexture(tex_info_slot_stage[SLOT_ITEM_1 + i].res_img, NULL, NULL, &w, &h);
		if (ret == 0) {
			tex_info_slot_stage[SLOT_ITEM_1 + i].src_rect = { 0, 0, w, h };
			tmp_rect = &slot_items[SLOT_ITEM_1 + i].dst_rect_base;
			tex_info_slot_stage[SLOT_ITEM_1 + i].dst_rect_base = { tmp_rect->x + (tmp_rect->w / 2) - (w/2), tmp_rect->y + (tmp_rect->h / 2) - h, w, h };
			GUI_tex_info_reset(&tex_info_slot_stage[SLOT_ITEM_1 + i]);
		}

		if (slot_stage[0] == '\0') {
			tex_info_slot_timestamp[SLOT_ITEM_1 + i].res_img = resource_manager_getFontTextureFromPath(" ");
		}
		else {
			tmp_string_size = game_utils_string_cat(tmp_string, (char*)"{18}", (char*)slot_timestamp);
			if (tmp_string_size <= 0) LOG_ERROR("Error: scene_save_menu load_event() get slot_timestamp\n");
			tex_info_slot_timestamp[SLOT_ITEM_1 + i].res_img = resource_manager_getFontTextureFromPath(tmp_string);
		}
		ret = GUI_QueryTexture(tex_info_slot_timestamp[SLOT_ITEM_1 + i].res_img, NULL, NULL, &w, &h);
		if (ret == 0) {
			tex_info_slot_timestamp[SLOT_ITEM_1 + i].src_rect = { 0, 0, w, h };
			tmp_rect = &slot_items[SLOT_ITEM_1 + i].dst_rect_base;
			tex_info_slot_timestamp[SLOT_ITEM_1 + i].dst_rect_base = { tmp_rect->x + (tmp_rect->w / 2) - (w / 2), tmp_rect->y + (tmp_rect->h / 2) + 10, w, h };
			GUI_tex_info_reset(&tex_info_slot_timestamp[SLOT_ITEM_1 + i]);
		}
	}

	if (!set_slot_index_flg) {
		slot_index = 0;
		cursor_index = 0;
		slot_select(); // set cursor rect
	}

	// key event switch
	game_key_event_init();
	game_key_event_set_key(SDL_SCANCODE_ESCAPE);
	game_key_event_set_key(SDL_SCANCODE_TAB);
	game_key_event_set_key(SDL_SCANCODE_RETURN);
	game_key_event_set_key(SDL_SCANCODE_UP);
	game_key_event_set_key(SDL_SCANCODE_DOWN);
	game_key_event_set_key(SDL_SCANCODE_LEFT);
	game_key_event_set_key(SDL_SCANCODE_RIGHT);

	game_mouse_event_init(0, 400, 200, 150, 5);

	// get current mouse position
	int x = 0, y = 0;
	button_index = BUTTON_ITEM_CANCEL;
	gui_active_group_id = GUI_ITEM_GROUP_ID_SLOT;
	gui_active_button_index = -1;
	game_mouse_event_get_motion(&x, &y);
	for (int i = 0; i < BUTTON_ITEM_SIZE; i++) {
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

	for (int i = 0; i < SLOT_ITEM_END; i++) {
		if (game_utils_decision_internal(&slot_items[i].dst_rect, x, y)) {
			slot_index = i;
			slot_items[i].mouse_stat = GUI_BUTTON_ACTIVE;
			gui_active_button_index = i;
			gui_active_group_id = GUI_ITEM_GROUP_ID_SLOT;
		}
		else {
			slot_items[i].mouse_stat = 0;
		}
	}

	// play music
	sound_manager_play(resource_manager_getChunkFromPath("music/save_menu.ogg"), SOUND_MANAGER_CH_MUSIC, -1);
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

// save & load next scene
static void save_and_load_scene()
{
	// save slot
	if (display_title_type == SCENE_SAVE_MENU_DISP_TYPE_NEW_GAME) {
		dialog_select_profile_reset("Please select your charactor.", dialog_select_profile_ok);
		dialog_select_profile_set_enable(true);
	}
}

// button callback func
static void button_delete() {
	// confirm overwrite
	char slot_stage[GAME_UTILS_STRING_NAME_BUF_SIZE];
	game_save_get_config_slot(cursor_index, NULL, slot_stage, NULL);
	if (slot_stage[0] == '\0') {
		// already empty
		sound_manager_play(resource_manager_getChunkFromPath("sounds/sfx_error1.ogg"), SOUND_MANAGER_CH_SFX2);
		dialog_message_reset("Empty Slot!", NULL, dialog_message_cancel, DIALOG_MSG_TYPE_OK_ONLY);
		dialog_message_set_enable(true);
		return;
	}
	else {
		sound_manager_play(resource_manager_getChunkFromPath("sounds/sfx_warning1.ogg"), SOUND_MANAGER_CH_SFX2);
		dialog_message_reset("Delete Slot?", dialog_message_delete_cancel, dialog_message_delete_ok);
		dialog_message_set_enable(true);
	}
}
static void button_cancel() {
	sound_manager_play(resource_manager_getChunkFromPath("sounds/sfx_cancel1.ogg"), SOUND_MANAGER_CH_SFX2);
	set_stat_event(SCENE_STAT_IDLE);
	scene_manager_load(return_scene_id);
}
static void button_ok() {
	if (display_title_type == SCENE_SAVE_MENU_DISP_TYPE_LOAD) {
		char slot_player[GAME_UTILS_STRING_NAME_BUF_SIZE];
		char slot_stage[GAME_UTILS_STRING_NAME_BUF_SIZE];
		char slot_timestamp[GAME_UTILS_STRING_NAME_BUF_SIZE];
		game_save_get_config_slot(cursor_index, slot_player, slot_stage, slot_timestamp);
		if (slot_stage[0] == '\0') {
			sound_manager_play(resource_manager_getChunkFromPath("sounds/sfx_error1.ogg"), SOUND_MANAGER_CH_SFX2);
			dialog_message_reset("Empty Slot!", NULL, dialog_message_cancel, DIALOG_MSG_TYPE_OK_ONLY);
			dialog_message_set_enable(true);
			return;
		}

		if (scene_play_stage_get_func()->get_stat_event() != SCENE_STAT_NONE) {
			// release previous buffer
			scene_play_stage_get_func()->unload_event();
		}

		// load backup
		game_save_get_config_player_backup(cursor_index);

		//std::string player_path = "units/player/" + slot_player + "/" + slot_player + ".unit";
		char player_dir_path[GAME_UTILS_STRING_CHAR_BUF_SIZE];
		char player_file_path[GAME_UTILS_STRING_CHAR_BUF_SIZE];
		int player_path_size = game_utils_string_cat(player_dir_path, (char*)"units/player/", slot_player, (char*)"/");
		if (player_path_size <= 0) { LOG_ERROR("Error: scene_save_menu button_ok() get player_dir_path\n"); return; }
		player_path_size = game_utils_string_cat(player_file_path, player_dir_path, slot_player, (char*)".unit");
		if (player_path_size <= 0) { LOG_ERROR("Error: scene_save_menu button_ok() get player_file_path\n"); return; }

		scene_play_stage_set_player(player_file_path, true);
		scene_loading_set_stage(slot_stage);
		scene_play_stage_set_stage_id(slot_stage);

		// save default slot for continue
		if (game_save_set_config_default_slot(cursor_index) == 0) {
			game_save_config_save();
		}

		sound_manager_play(resource_manager_getChunkFromPath("sounds/sfx_click1.ogg"), SOUND_MANAGER_CH_SFX2);
		set_stat_event(SCENE_STAT_IDLE);
		scene_manager_load(SCENE_ID_PLAY_STAGE, true);
	}
	// New Game
	else {
		// confirm overwrite
		char slot_stage[GAME_UTILS_STRING_NAME_BUF_SIZE];
		game_save_get_config_slot(cursor_index, NULL, slot_stage, NULL);
		if (slot_stage[0] != '\0') {
			sound_manager_play(resource_manager_getChunkFromPath("sounds/sfx_warning1.ogg"), SOUND_MANAGER_CH_SFX2);
			dialog_message_reset("Overwrite?", dialog_message_cancel, dialog_message_ok);
			dialog_message_set_enable(true);
			return;
		}
		else {
			sound_manager_play(resource_manager_getChunkFromPath("sounds/sfx_click1.ogg"), SOUND_MANAGER_CH_SFX2);
		}

		save_and_load_scene();
	}
}

// dialog func
static void dialog_message_delete_ok() {
	dialog_message_ret = DIALOG_MESSAGE_RET_OK;

	sound_manager_play(resource_manager_getChunkFromPath("sounds/sfx_click1.ogg"), SOUND_MANAGER_CH_SFX2);
	if (game_save_set_config_slot(cursor_index, (char*)"", (char*)"", true) == 0) {
		game_save_config_save();

		// reload delete slot
		int w, h;
		SDL_Rect* tmp_rect;
		int i = cursor_index;

		tex_info_slot_icon[SLOT_ITEM_1 + i].res_img = resource_manager_getFontTextureFromPath(" ");
		int ret = GUI_QueryTexture(tex_info_slot_icon[SLOT_ITEM_1 + i].res_img, NULL, NULL, &w, &h);
		if (ret == 0) {
			tex_info_slot_icon[SLOT_ITEM_1 + i].src_rect = { 0, 0, w, h };
			tmp_rect = &slot_items[SLOT_ITEM_1 + i].dst_rect_base;
			tex_info_slot_icon[SLOT_ITEM_1 + i].dst_rect_base = { tmp_rect->x + w / 4, tmp_rect->y + h / 4, w, h };
			GUI_tex_info_reset(&tex_info_slot_icon[SLOT_ITEM_1 + i]);
		}

		tex_info_slot_stage[SLOT_ITEM_1 + i].res_img = resource_manager_getFontTextureFromPath("(EMPTY)");
		ret = GUI_QueryTexture(tex_info_slot_stage[SLOT_ITEM_1 + i].res_img, NULL, NULL, &w, &h);
		if (ret == 0) {
			tex_info_slot_stage[SLOT_ITEM_1 + i].src_rect = { 0, 0, w, h };
			tmp_rect = &slot_items[SLOT_ITEM_1 + i].dst_rect_base;
			tex_info_slot_stage[SLOT_ITEM_1 + i].dst_rect_base = { tmp_rect->x + (tmp_rect->w / 2) - (w / 2), tmp_rect->y + (tmp_rect->h / 2) - h, w, h };
			GUI_tex_info_reset(&tex_info_slot_stage[SLOT_ITEM_1 + i]);
		}

		tex_info_slot_timestamp[SLOT_ITEM_1 + i].res_img = resource_manager_getFontTextureFromPath(" ");
		ret = GUI_QueryTexture(tex_info_slot_timestamp[SLOT_ITEM_1 + i].res_img, NULL, NULL, &w, &h);
		if (ret == 0) {
			tex_info_slot_timestamp[SLOT_ITEM_1 + i].src_rect = { 0, 0, w, h };
			tmp_rect = &slot_items[SLOT_ITEM_1 + i].dst_rect_base;
			tex_info_slot_timestamp[SLOT_ITEM_1 + i].dst_rect_base = { tmp_rect->x + (tmp_rect->w / 2) - (w / 2), tmp_rect->y + (tmp_rect->h / 2) + 10, w, h };
			GUI_tex_info_reset(&tex_info_slot_timestamp[SLOT_ITEM_1 + i]);
		}
	}

	dialog_message_set_enable(false);
}
static void dialog_message_delete_cancel() {
	sound_manager_play(resource_manager_getChunkFromPath("sounds/sfx_cancel1.ogg"), SOUND_MANAGER_CH_SFX2);
	dialog_message_ret = DIALOG_MESSAGE_RET_CANCEL;
	dialog_message_set_enable(false);
}

static void dialog_message_ok() {
	sound_manager_play(resource_manager_getChunkFromPath("sounds/sfx_click1.ogg"), SOUND_MANAGER_CH_SFX2);
	dialog_message_ret = DIALOG_MESSAGE_RET_OK;
	dialog_message_set_enable(false);
	save_and_load_scene();
}
static void dialog_message_cancel() {
	sound_manager_play(resource_manager_getChunkFromPath("sounds/sfx_cancel1.ogg"), SOUND_MANAGER_CH_SFX2);
	dialog_message_ret = DIALOG_MESSAGE_RET_CANCEL;
	dialog_message_set_enable(false);
}

static void dialog_select_profile_ok() {
	int unlock_stat;
	game_save_get_config_unlock_stat(&unlock_stat);
	int profile_index = dialog_select_profile_get_current_profile_index();
	if (unlock_stat & (0x00000001 << profile_index)) {
		// save slot
		game_save_set_config_default_slot(cursor_index);
		std::string start_player = g_resource_manager_profile[profile_index].name;
		scene_play_stage_set_player(g_resource_manager_profile[profile_index].unit_path);

		const char* story_path = "scenes/story/infinity/opening.dat";
		for (int prof_i = 0; prof_i < RESOURCE_MANAGER_PROFILE_LIST_SIZE; prof_i++) {
			if (strcmp(g_resource_manager_profile[prof_i].name, start_player.c_str()) == 0) {
				story_path = g_resource_manager_profile[prof_i].opening_path;
				break;
			}
		}
		scene_play_story_set_story(story_path, true);

		set_stat_event(SCENE_STAT_IDLE);
		dialog_select_profile_set_enable(false);
		scene_manager_load(SCENE_ID_PLAY_STORY);
	}
	// locking charactor
	else {
		sound_manager_play(resource_manager_getChunkFromPath("sounds/sfx_error1.ogg"), SOUND_MANAGER_CH_SFX2);
	}
}

// slot callback func
static void slot_select() {
	cursor_index = slot_index;

	// set cursor rect
	cursor_item.dst_rect_base = { slot_items[cursor_index].dst_rect_base.x - 5, slot_items[cursor_index].dst_rect_base.y - 5,
		slot_items[cursor_index].dst_rect_base.w + 10, slot_items[cursor_index].dst_rect_base.h + 10 };
	GUI_rect_region_reset(&cursor_item);
}

// init draw items
static void tex_info_init()
{
	int w, h;
	int w_pos = 0, h_pos = 0;
	int slot_w_pos = 0, slot_h_pos = 0;
	int button_h_pos = 0;

	// title
	tex_info_title[SCENE_SAVE_MENU_ID_TITLE_SAVE].res_img = resource_manager_getFontTextureFromPath("Save");
	int ret = GUI_QueryTexture(tex_info_title[SCENE_SAVE_MENU_ID_TITLE_SAVE].res_img, NULL, NULL, &w, &h);
	if (ret == 0) {
		w_pos = 20;
		h_pos = 20;
		GUI_tex_info_init_rect(&tex_info_title[SCENE_SAVE_MENU_ID_TITLE_SAVE], w, h, w_pos, h_pos);
	}

	tex_info_title[SCENE_SAVE_MENU_ID_TITLE_NEW_GAME].res_img = resource_manager_getFontTextureFromPath("NewGame");
	ret = GUI_QueryTexture(tex_info_title[SCENE_SAVE_MENU_ID_TITLE_NEW_GAME].res_img, NULL, NULL, &w, &h);
	if (ret == 0) {
		GUI_tex_info_init_rect(&tex_info_title[SCENE_SAVE_MENU_ID_TITLE_NEW_GAME], w, h, w_pos, h_pos);
	}

	tex_info_title[SCENE_SAVE_MENU_ID_TITLE_LOAD].res_img = resource_manager_getFontTextureFromPath("Load");
	ret = GUI_QueryTexture(tex_info_title[SCENE_SAVE_MENU_ID_TITLE_LOAD].res_img, NULL, NULL, &w, &h);
	if (ret == 0) {
		GUI_tex_info_init_rect(&tex_info_title[SCENE_SAVE_MENU_ID_TITLE_LOAD], w, h, w_pos, h_pos);
		w_pos = w_pos + 32;
		h_pos = h_pos + h + 20;
	}

	// description
	tex_info_description[SCENE_SAVE_MENU_ID_DESCRIPTION_1].res_img = resource_manager_getFontTextureFromPath("Please select save Slot.");
	ret = GUI_QueryTexture(tex_info_description[SCENE_SAVE_MENU_ID_DESCRIPTION_1].res_img, NULL, NULL, &w, &h);
	if (ret == 0) {
		GUI_tex_info_init_rect(&tex_info_description[SCENE_SAVE_MENU_ID_DESCRIPTION_1], w, h, w_pos, h_pos);
	}

	tex_info_description[SCENE_SAVE_MENU_ID_DESCRIPTION_2].res_img = resource_manager_getFontTextureFromPath("Please select new save Slot.");
	ret = GUI_QueryTexture(tex_info_description[SCENE_SAVE_MENU_ID_DESCRIPTION_2].res_img, NULL, NULL, &w, &h);
	if (ret == 0) {
		GUI_tex_info_init_rect(&tex_info_description[SCENE_SAVE_MENU_ID_DESCRIPTION_2], w, h, w_pos, h_pos);
	}

	tex_info_description[SCENE_SAVE_MENU_ID_DESCRIPTION_3].res_img = resource_manager_getFontTextureFromPath("Please select Slot.");
	ret = GUI_QueryTexture(tex_info_description[SCENE_SAVE_MENU_ID_DESCRIPTION_3].res_img, NULL, NULL, &w, &h);
	if (ret == 0) {
		GUI_tex_info_init_rect(&tex_info_description[SCENE_SAVE_MENU_ID_DESCRIPTION_3], w, h, w_pos, h_pos);
		slot_w_pos = w_pos;
		slot_h_pos = h_pos + h + 20;
	}

	// button (left binding [delete])
	tex_info[SCENE_SAVE_MENU_ID_DELETE].res_img = resource_manager_getTextureFromPath("{scale_mode:linear}images/gui/button/delete.png");
	ret = GUI_QueryTexture(tex_info[SCENE_SAVE_MENU_ID_DELETE].res_img, NULL, NULL, &w, &h);
	if (ret == 0) {
		w_pos = 10;
		h_pos = SCREEN_HEIGHT - h - 10;
		GUI_tex_info_init_rect(&tex_info[SCENE_SAVE_MENU_ID_DELETE], w, h, w_pos, h_pos);
		button_items[BUTTON_ITEM_DELETE].tex_info_id = SCENE_SAVE_MENU_ID_DELETE;
		button_items[BUTTON_ITEM_DELETE].dst_rect = tex_info[SCENE_SAVE_MENU_ID_DELETE].dst_rect;
		button_items[BUTTON_ITEM_DELETE].dst_rect_base = tex_info[SCENE_SAVE_MENU_ID_DELETE].dst_rect_base;
		button_items[BUTTON_ITEM_DELETE].func = NULL;
	}

	// button (right binding [cancel] [ok])
	tex_info[SCENE_SAVE_MENU_ID_OK].res_img = resource_manager_getTextureFromPath("{scale_mode:linear}images/gui/button/ok.png");
	ret = GUI_QueryTexture(tex_info[SCENE_SAVE_MENU_ID_OK].res_img, NULL, NULL, &w, &h);
	if (ret == 0) {
		w_pos = SCREEN_WIDTH - w - 10;
		h_pos = SCREEN_HEIGHT - h - 10;
		button_h_pos = h_pos;
		GUI_tex_info_init_rect(&tex_info[SCENE_SAVE_MENU_ID_OK], w, h, w_pos, h_pos);
		button_items[BUTTON_ITEM_OK].tex_info_id = SCENE_SAVE_MENU_ID_OK;
		button_items[BUTTON_ITEM_OK].dst_rect = tex_info[SCENE_SAVE_MENU_ID_OK].dst_rect;
		button_items[BUTTON_ITEM_OK].dst_rect_base = tex_info[SCENE_SAVE_MENU_ID_OK].dst_rect_base;
		button_items[BUTTON_ITEM_OK].func = NULL;
	}

	tex_info[SCENE_SAVE_MENU_ID_CANCEL].res_img = resource_manager_getTextureFromPath("{scale_mode:linear}images/gui/button/cancel.png");
	ret = GUI_QueryTexture(tex_info[SCENE_SAVE_MENU_ID_CANCEL].res_img, NULL, NULL, &w, &h);
	if (ret == 0) {
		w_pos = w_pos - w - 10;
		//h_pos = dialog_bottom - h - 10;
		GUI_tex_info_init_rect(&tex_info[SCENE_SAVE_MENU_ID_CANCEL], w, h, w_pos, h_pos);
		button_items[BUTTON_ITEM_CANCEL].tex_info_id = SCENE_SAVE_MENU_ID_CANCEL;
		button_items[BUTTON_ITEM_CANCEL].dst_rect = tex_info[SCENE_SAVE_MENU_ID_CANCEL].dst_rect;
		button_items[BUTTON_ITEM_CANCEL].dst_rect_base = tex_info[SCENE_SAVE_MENU_ID_CANCEL].dst_rect_base;
		button_items[BUTTON_ITEM_CANCEL].func = NULL;
	}

	// slot
	int slot_top     = slot_h_pos;
	int slot_left    = slot_w_pos;
	int slot_right   = SCREEN_WIDTH - 20;
	int slot_bottom  = button_h_pos - 40;
	int slot_item_dw = (slot_right - slot_left) / 4;
	int slot_item_dh = (slot_bottom - slot_top) / 2;
	int slot_item_w  = slot_left + 30;
	int slot_item_h  = slot_top  + 30;
	int slot_label_w = 0;
	int slot_label_h = 0;

	SDL_Rect* tmp_rect;
	std::string label_slot = "SLOT1";
	for (int i = 0; i < 4; i++) {
		tex_info[SCENE_SAVE_MENU_ID_LABEL_SLOT1 + i].res_img = resource_manager_getFontTextureFromPath(label_slot.c_str());
		ret = GUI_QueryTexture(tex_info[SCENE_SAVE_MENU_ID_LABEL_SLOT1 + i].res_img, NULL, NULL, &w, &h);
		if (ret == 0) {
			slot_label_w = slot_left + i * slot_item_dw + (slot_item_dw / 2) - (w/2);
			slot_label_h = slot_top + slot_item_dh - h - 20;
			GUI_tex_info_init_rect(&tex_info[SCENE_SAVE_MENU_ID_LABEL_SLOT1 + i], w, h, slot_label_w, slot_label_h);
			slot_items[SLOT_ITEM_1 + i].tex_info_id = -1; // no texture
			slot_items[SLOT_ITEM_1 + i].dst_rect_base = { slot_item_w + i * slot_item_dw, slot_item_h, (slot_item_dw - 60), (slot_item_dh - h - 20 - 60) };
			tmp_rect = &slot_items[SLOT_ITEM_1 + i].dst_rect_base;
			slot_items[SLOT_ITEM_1 + i].dst_rect = VIEW_SCALE_RECT(tmp_rect->x, tmp_rect->y, tmp_rect->w, tmp_rect->h);
			slot_items[SLOT_ITEM_1 + i].func = NULL;
		}
		label_slot[4] += 1;
	}

	label_slot = "SLOT5";
	for (int i = 0; i < 4; i++) {
		tex_info[SCENE_SAVE_MENU_ID_LABEL_SLOT5 + i].res_img = resource_manager_getFontTextureFromPath(label_slot.c_str());
		ret = GUI_QueryTexture(tex_info[SCENE_SAVE_MENU_ID_LABEL_SLOT5 + i].res_img, NULL, NULL, &w, &h);
		if (ret == 0) {
			slot_label_w = slot_left + i * slot_item_dw + (slot_item_dw / 2) - (w / 2);
			slot_label_h = slot_top + 2 * slot_item_dh - h - 20;
			GUI_tex_info_init_rect(&tex_info[SCENE_SAVE_MENU_ID_LABEL_SLOT5 + i], w, h, slot_label_w, slot_label_h);
			slot_items[SLOT_ITEM_5 + i].tex_info_id = -1; // no texture
			slot_items[SLOT_ITEM_5 + i].dst_rect_base = { slot_item_w + i * slot_item_dw, slot_item_h + slot_item_dh, (slot_item_dw - 60), (slot_item_dh - h - 20 - 60) };
			tmp_rect = &slot_items[SLOT_ITEM_5 + i].dst_rect_base;
			slot_items[SLOT_ITEM_5 + i].dst_rect = VIEW_SCALE_RECT(tmp_rect->x, tmp_rect->y, tmp_rect->w, tmp_rect->h);
			slot_items[SLOT_ITEM_5 + i].func = NULL;
		}
		label_slot[4] += 1;
	}
}

void scene_save_menu_init() {
	// set stat
	scene_stat = SCENE_STAT_NONE;

	// set fuctions
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
	resource_manager_load_dat((char*)"scenes/scene_save_menu.dat");

	// set texture position
	tex_info_init();

	// init dialog
	dialog_select_profile_init();

	// set button data
	button_items[BUTTON_ITEM_DELETE].mouse_stat = 0;
	button_items[BUTTON_ITEM_DELETE].func = button_delete;
	button_items[BUTTON_ITEM_CANCEL].mouse_stat = 0;
	button_items[BUTTON_ITEM_CANCEL].func = button_cancel;
	button_items[BUTTON_ITEM_OK].mouse_stat = 0;
	button_items[BUTTON_ITEM_OK].func = button_ok;
	button_index = 0;

	// set button data
	for (int i = 0; i < SLOT_ITEM_END; i++) {
		slot_items[SLOT_ITEM_1 + i].mouse_stat = 0;
		slot_items[SLOT_ITEM_1 + i].func = slot_select;
	}
	slot_index = 0;
	cursor_index = 0;
}

SceneManagerFunc* scene_save_menu_get_func() {
	return &scene_func;
}

void scene_save_menu_set_display_title_type(int disp_type) {
	display_title_type = disp_type;
}
