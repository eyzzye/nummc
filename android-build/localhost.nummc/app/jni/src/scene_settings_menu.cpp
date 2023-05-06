#include "game_common.h"
#include "gui_common.h"
#include "scene_manager.h"
#include "scene_settings_menu.h"

#include "resource_manager.h"
#include "sound_manager.h"
#include "game_key_event.h"
#include "game_mouse_event.h"
#include "game_window.h"
#include "game_utils.h"
#include "game_save.h"

#ifdef _ANDROID
#include "gui_touch_control.h"
#endif

// draw static data
#define SCENE_SETTINGS_MENU_ID_TITLE       0
#define SCENE_SETTINGS_MENU_ID_LABEL_VIDEO 1
#define SCENE_SETTINGS_MENU_ID_LABEL_MUSIC 2
#define SCENE_SETTINGS_MENU_ID_LABEL_SFX   3
#define SCENE_SETTINGS_MENU_ID_DEFAULT     4
#define SCENE_SETTINGS_MENU_ID_CANCEL      5
#define SCENE_SETTINGS_MENU_ID_OK          6
#define SCENE_SETTINGS_MENU_ID_END         7

static tex_info_t tex_info[SCENE_SETTINGS_MENU_ID_END];

// button data
#define BUTTON_ITEM_DEFAULT 0
#define BUTTON_ITEM_CANCEL  1
#define BUTTON_ITEM_OK      2
#define BUTTON_ITEM_SIZE    3

static gui_item_t button_items[BUTTON_ITEM_SIZE];
static int button_index = 0;

static void button_default();
static void button_cancel();
static void button_ok();

// video data
#define VIDEO_ITEM_ID_640x480   0
#define VIDEO_ITEM_ID_1280x720  1
#define VIDEO_ITEM_ID_SIZE      2
static tex_info_t tex_info_video[VIDEO_ITEM_ID_SIZE];
static int video_item_index = 0;

#define VIDEO_BUTTON_ITEM_ID_LEFT  0
#define VIDEO_BUTTON_ITEM_ID_RIGHT 1
#define VIDEO_BUTTON_ITEM_ID_SIZE  2
static tex_info_t tex_info_video_button[VIDEO_BUTTON_ITEM_ID_SIZE];

#define VIDEO_BUTTON_ITEM_LEFT  0
#define VIDEO_BUTTON_ITEM_RIGHT 1
#define VIDEO_BUTTON_ITEM_SIZE  2
static gui_item_t video_button_items[VIDEO_BUTTON_ITEM_SIZE];

static void video_button_left();
static void video_button_right();

// music data
#define MUSIC_VALUE_MIN    0
#define MUSIC_VALUE_MAX  100
static tex_info_t tex_info_music_text;
static rect_region_t music_guid_line;
static tex_info_t tex_info_music_slider;

static gui_item_t music_slider_items;
static int music_volume_value;

static void music_button_left();
static void music_button_right();
static void music_button_click();
static void tex_info_reset_music_volume();

// sfx data
#define SFX_VALUE_MIN    0
#define SFX_VALUE_MAX  100
static tex_info_t tex_info_sfx_text;
static rect_region_t sfx_guid_line;
static tex_info_t tex_info_sfx_slider;

static gui_item_t sfx_slider_items;
static int sfx_volume_value;

static void sfx_button_left();
static void sfx_button_right();
static void sfx_button_click();
static void tex_info_reset_sfx_volume();

// GUI stat
#define GUI_ITEM_GROUP_ID_VIDEO  0
#define GUI_ITEM_GROUP_ID_MUSIC  1
#define GUI_ITEM_GROUP_ID_SFX    2
#define GUI_ITEM_GROUP_ID_SELECT 3
#define GUI_ITEM_GROUP_ID_SIZE   4

static rect_region_t group_region_video;
static rect_region_t group_region_music;
static rect_region_t group_region_sfx;
static int gui_active_group_id;
static int gui_active_button_index;
static int current_mouse_x;

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

	bool select_snd_on = false;
	void_func* action_func = NULL;

	if (game_key_event_get(SDL_SCANCODE_ESCAPE, GUI_SELECT_WAIT_TIMER)) {
		button_cancel();
	}
	if (game_key_event_get(SDL_SCANCODE_UP, GUI_SELECT_WAIT_TIMER)) {
		if (gui_active_group_id > 0) {
			select_snd_on = true;
			gui_active_group_id -= 1;
		}
	}
	if (game_key_event_get(SDL_SCANCODE_DOWN, GUI_SELECT_WAIT_TIMER)) {
		if (gui_active_group_id < GUI_ITEM_GROUP_ID_SIZE - 1) {
			select_snd_on = true;
			gui_active_group_id += 1;
		}
	}
	if (game_key_event_get(SDL_SCANCODE_TAB, GUI_SELECT_WAIT_TIMER)) {
		select_snd_on = true;
		gui_active_group_id += 1;
		if (gui_active_group_id >= GUI_ITEM_GROUP_ID_SIZE) gui_active_group_id = 0;
	}

	if (game_key_event_get(SDL_SCANCODE_LEFT, GUI_SELECT_WAIT_TIMER)) {
		if (gui_active_group_id == GUI_ITEM_GROUP_ID_SELECT) {
			select_snd_on = true;
			button_index -= 1;
			if (button_index < 0) button_index = BUTTON_ITEM_SIZE - 1;
		}
		else if (gui_active_group_id == GUI_ITEM_GROUP_ID_VIDEO) {
			video_button_left();
		}
		else if (gui_active_group_id == GUI_ITEM_GROUP_ID_MUSIC) {
			music_button_left();
		}
		else if (gui_active_group_id == GUI_ITEM_GROUP_ID_SFX) {
			sfx_button_left();
		}
	}
	if (game_key_event_get(SDL_SCANCODE_RIGHT, GUI_SELECT_WAIT_TIMER)) {
		if (gui_active_group_id == GUI_ITEM_GROUP_ID_SELECT) {
			select_snd_on = true;
			button_index += 1;
			if (button_index >= BUTTON_ITEM_SIZE) button_index = 0;
		}
		else if (gui_active_group_id == GUI_ITEM_GROUP_ID_VIDEO) {
			video_button_right();
		}
		else if (gui_active_group_id == GUI_ITEM_GROUP_ID_MUSIC) {
			music_button_right();
		}
		else if (gui_active_group_id == GUI_ITEM_GROUP_ID_SFX) {
			sfx_button_right();
		}
	}
	if (game_key_event_get(SDL_SCANCODE_RETURN, GUI_SELECT_WAIT_TIMER)) {
		if (gui_active_group_id == GUI_ITEM_GROUP_ID_SELECT) {
			action_func = button_items[button_index].func;
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

		if (game_utils_decision_internal(&group_region_video.dst_rect, x, y)) {
			if (gui_active_group_id != GUI_ITEM_GROUP_ID_VIDEO) select_snd_on = true;
			gui_active_group_id = GUI_ITEM_GROUP_ID_VIDEO;
		}
		for (int i = 0; i < VIDEO_BUTTON_ITEM_SIZE; i++) {
			if (game_utils_decision_internal(&tex_info_video_button[video_button_items[i].tex_info_id].dst_rect, x, y)) {
				video_button_items[i].mouse_stat = GUI_BUTTON_ACTIVE;
				gui_active_button_index = i;
				gui_active_group_id = GUI_ITEM_GROUP_ID_VIDEO;
			}
			else {
				video_button_items[i].mouse_stat = 0;
			}
		}

		if (game_utils_decision_internal(&group_region_music.dst_rect, x, y)) {
			if (gui_active_group_id != GUI_ITEM_GROUP_ID_MUSIC) select_snd_on = true;
			gui_active_group_id = GUI_ITEM_GROUP_ID_MUSIC;
		}
		if (game_utils_decision_internal(&music_slider_items.dst_rect, x, y)) {
			music_slider_items.mouse_stat = GUI_BUTTON_ACTIVE;
			gui_active_button_index = 0;
			gui_active_group_id = GUI_ITEM_GROUP_ID_MUSIC;
		}
		else {
			music_slider_items.mouse_stat = 0;
		}

		if (game_utils_decision_internal(&group_region_sfx.dst_rect, x, y)) {
			if (gui_active_group_id != GUI_ITEM_GROUP_ID_SFX) select_snd_on = true;
			gui_active_group_id = GUI_ITEM_GROUP_ID_SFX;
		}
		if (game_utils_decision_internal(&sfx_slider_items.dst_rect, x, y)) {
			sfx_slider_items.mouse_stat = GUI_BUTTON_ACTIVE;
			gui_active_button_index = 0;
			gui_active_group_id = GUI_ITEM_GROUP_ID_SFX;
		}
		else {
			sfx_slider_items.mouse_stat = 0;
		}
	}

	Uint32 mouse_left_stat = game_mouse_event_get(GAME_MOUSE_LEFT);
	if ((mouse_left_stat & GAME_MOUSE_CLICK) && (gui_active_button_index >= 0)) {
		if (gui_active_group_id == GUI_ITEM_GROUP_ID_SELECT) {
			if (button_items[gui_active_button_index].mouse_stat & GUI_BUTTON_ACTIVE) {
				button_items[gui_active_button_index].mouse_stat |= GUI_BUTTON_CLICK;
			}
		}
		else if (gui_active_group_id == GUI_ITEM_GROUP_ID_VIDEO) {
			if (video_button_items[gui_active_button_index].mouse_stat & GUI_BUTTON_ACTIVE) {
				video_button_items[gui_active_button_index].mouse_stat |= GUI_BUTTON_CLICK;
			}
		}
		else if (gui_active_group_id == GUI_ITEM_GROUP_ID_MUSIC) {
			if (music_slider_items.mouse_stat & GUI_BUTTON_ACTIVE) {
				music_slider_items.mouse_stat |= GUI_BUTTON_CLICK;
			}
		}
		else if (gui_active_group_id == GUI_ITEM_GROUP_ID_SFX) {
			if (sfx_slider_items.mouse_stat & GUI_BUTTON_ACTIVE) {
				sfx_slider_items.mouse_stat |= GUI_BUTTON_CLICK;
			}
		}
	}
	if ((mouse_left_stat & GAME_MOUSE_RELEASE) && (gui_active_button_index >= 0)) {
		if (gui_active_group_id == GUI_ITEM_GROUP_ID_SELECT) {
			if (button_items[gui_active_button_index].mouse_stat == (GUI_BUTTON_ACTIVE | GUI_BUTTON_CLICK)) {
				button_items[gui_active_button_index].mouse_stat &= ~GUI_BUTTON_CLICK;
				action_func = button_items[gui_active_button_index].func;
			}
		}
		else if (gui_active_group_id == GUI_ITEM_GROUP_ID_VIDEO) {
			if (video_button_items[gui_active_button_index].mouse_stat == (GUI_BUTTON_ACTIVE | GUI_BUTTON_CLICK)) {
				video_button_items[gui_active_button_index].mouse_stat &= ~GUI_BUTTON_CLICK;
				action_func = video_button_items[gui_active_button_index].func;
			}
		}
		else if (gui_active_group_id == GUI_ITEM_GROUP_ID_MUSIC) {
			if (music_slider_items.mouse_stat == (GUI_BUTTON_ACTIVE | GUI_BUTTON_CLICK)) {
				music_slider_items.mouse_stat &= ~GUI_BUTTON_CLICK;
				current_mouse_x = x;
				action_func = music_slider_items.func;
			}
		}
		else if (gui_active_group_id == GUI_ITEM_GROUP_ID_SFX) {
			if (sfx_slider_items.mouse_stat == (GUI_BUTTON_ACTIVE | GUI_BUTTON_CLICK)) {
				sfx_slider_items.mouse_stat &= ~GUI_BUTTON_CLICK;
				current_mouse_x = x;
				action_func = sfx_slider_items.func;
			}
		}
	}

	if (select_snd_on) {
		sound_manager_play(resource_manager_getChunkFromPath("sounds/sfx_select1.ogg"), SOUND_MANAGER_CH_SFX1);
	}
	if (action_func) {
		(*action_func)();
		action_func = NULL;
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

	// draw item
	GUI_tex_info_draw(tex_info, SCENE_SETTINGS_MENU_ID_END);

	// draw video
	GUI_tex_info_draw(&tex_info_video[video_item_index]);
	if (video_item_index != VIDEO_ITEM_ID_640x480) {
		GUI_tex_info_draw(&tex_info_video_button[VIDEO_BUTTON_ITEM_ID_LEFT]);
	}
	if (video_item_index != (VIDEO_ITEM_ID_SIZE - 1)) {
		GUI_tex_info_draw(&tex_info_video_button[VIDEO_BUTTON_ITEM_ID_RIGHT]);
	}

	// draw music
	SDL_SetRenderDrawColor(g_ren, 192, 192, 192, 255);
	SDL_RenderFillRect(g_ren, &music_guid_line.dst_rect);
	GUI_tex_info_draw(&tex_info_music_text);
	GUI_tex_info_draw(&tex_info_music_slider);

	// draw sfx
	SDL_RenderFillRect(g_ren, &sfx_guid_line.dst_rect);
	GUI_tex_info_draw(&tex_info_sfx_text);
	GUI_tex_info_draw(&tex_info_sfx_slider);

	// draw selected
	if (gui_active_group_id == GUI_ITEM_GROUP_ID_SELECT) {
		SDL_SetRenderDrawColor(g_ren, 0, 0, 0, 32);
		SDL_RenderFillRect(g_ren, &button_items[button_index].dst_rect);
	}
	else if (gui_active_group_id == GUI_ITEM_GROUP_ID_VIDEO) {
		SDL_SetRenderDrawColor(g_ren, 0, 0, 0, 32);
		SDL_RenderFillRect(g_ren, &tex_info_video[VIDEO_ITEM_ID_1280x720].dst_rect);
	}
	else if (gui_active_group_id == GUI_ITEM_GROUP_ID_MUSIC) {
		SDL_SetRenderDrawColor(g_ren, 0, 0, 0, 32);
		SDL_RenderFillRect(g_ren, &tex_info_music_text.dst_rect);
	}
	else if (gui_active_group_id == GUI_ITEM_GROUP_ID_SFX) {
		SDL_SetRenderDrawColor(g_ren, 0, 0, 0, 32);
		SDL_RenderFillRect(g_ren, &tex_info_sfx_text.dst_rect);
	}

	// group region
#if 1
	SDL_SetRenderDrawColor(g_ren, 0, 0, 0, 255);
	SDL_RenderDrawRect(g_ren, &group_region_video.dst_rect);
	SDL_RenderDrawRect(g_ren, &group_region_music.dst_rect);
	SDL_RenderDrawRect(g_ren, &group_region_sfx.dst_rect);
#endif

#ifdef _ANDROID
	// draw gui_touch_control
	gui_touch_control_draw();
#endif

	SDL_RenderPresent(g_ren);
}
static void after_draw() {
	if (scene_stat != SCENE_STAT_ACTIVE) return;
}
static void load_event() {
	return_scene_id = scene_manager_get_scene_id();

	// set Video setting
	int w = 0, h = 0;
	video_item_index = VIDEO_ITEM_ID_1280x720;
	game_save_get_config_resolution(&w, &h);
	if ((w == 640) && (h == 480)) video_item_index = VIDEO_ITEM_ID_640x480;
	else if ((w == 1280) && (h == 720)) video_item_index = VIDEO_ITEM_ID_1280x720;

	// set music setting
	int vol = GAME_SAVE_CONFIG_MUSIC_VOLUME_DEFAULT;
	game_save_get_config_music_volume(&vol);
	music_volume_value = vol;
	tex_info_reset_music_volume();

	// set sfx setting
	vol = GAME_SAVE_CONFIG_SFX_VOLUME_DEFAULT;
	game_save_get_config_sfx_volume(&vol);
	sfx_volume_value = vol;
	tex_info_reset_sfx_volume();

	// resize
	GUI_tex_info_reset(tex_info, SCENE_SETTINGS_MENU_ID_END);
	GUI_gui_item_reset(button_items, BUTTON_ITEM_SIZE);
	GUI_tex_info_reset(tex_info_video, VIDEO_ITEM_ID_SIZE);
	GUI_tex_info_reset(tex_info_video_button, VIDEO_BUTTON_ITEM_ID_SIZE);
	GUI_gui_item_reset(video_button_items, VIDEO_BUTTON_ITEM_SIZE);
	GUI_rect_region_reset(&group_region_video);

	GUI_tex_info_reset(&tex_info_music_text);
	GUI_rect_region_reset(&music_guid_line);
	GUI_tex_info_reset(&tex_info_music_slider);
	GUI_gui_item_reset(&music_slider_items);
	GUI_rect_region_reset(&group_region_music);

	GUI_tex_info_reset(&tex_info_sfx_text);
	GUI_rect_region_reset(&sfx_guid_line);
	GUI_tex_info_reset(&tex_info_sfx_slider);
	GUI_gui_item_reset(&sfx_slider_items);
	GUI_rect_region_reset(&group_region_sfx);

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
	gui_active_group_id = GUI_ITEM_GROUP_ID_VIDEO;
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

	if (game_utils_decision_internal(&group_region_video.dst_rect, x, y)) {
		gui_active_group_id = GUI_ITEM_GROUP_ID_VIDEO;
	}
	for (int i = 0; i < VIDEO_BUTTON_ITEM_SIZE; i++) {
		if (game_utils_decision_internal(&tex_info_video_button[video_button_items[i].tex_info_id].dst_rect, x, y)) {
			video_button_items[i].mouse_stat = GUI_BUTTON_ACTIVE;
			gui_active_button_index = i;
			gui_active_group_id = GUI_ITEM_GROUP_ID_VIDEO;
		}
		else {
			video_button_items[i].mouse_stat = 0;
		}
	}

	if (game_utils_decision_internal(&group_region_music.dst_rect, x, y)) {
		gui_active_group_id = GUI_ITEM_GROUP_ID_MUSIC;
	}
	if (game_utils_decision_internal(&music_slider_items.dst_rect, x, y)) {
		music_slider_items.mouse_stat = GUI_BUTTON_ACTIVE;
		gui_active_button_index = 0;
		gui_active_group_id = GUI_ITEM_GROUP_ID_MUSIC;
	}
	else {
		music_slider_items.mouse_stat = 0;
	}

	if (game_utils_decision_internal(&group_region_sfx.dst_rect, x, y)) {
		gui_active_group_id = GUI_ITEM_GROUP_ID_SFX;
	}
	if (game_utils_decision_internal(&sfx_slider_items.dst_rect, x, y)) {
		sfx_slider_items.mouse_stat = GUI_BUTTON_ACTIVE;
		gui_active_button_index = 0;
		gui_active_group_id = GUI_ITEM_GROUP_ID_SFX;
	}
	else {
		sfx_slider_items.mouse_stat = 0;
	}

	current_mouse_x = x;
}
static void unload_event() {

}
static int get_stat_event() {
	return scene_stat;
}
static void set_stat_event(int stat) {
	scene_stat = stat;
}

// button callback func
static void button_default() {
	sound_manager_play(resource_manager_getChunkFromPath("sounds/sfx_click1.ogg"), SOUND_MANAGER_CH_SFX2);
	video_item_index = VIDEO_ITEM_ID_1280x720;

	music_volume_value = GAME_SAVE_CONFIG_MUSIC_VOLUME_DEFAULT;
	tex_info_reset_music_volume();

	sfx_volume_value = GAME_SAVE_CONFIG_SFX_VOLUME_DEFAULT;
	tex_info_reset_sfx_volume();
}
static void button_cancel() {
	sound_manager_play(resource_manager_getChunkFromPath("sounds/sfx_cancel1.ogg"), SOUND_MANAGER_CH_SFX2);
	scene_stat = SCENE_STAT_IDLE;
	scene_manager_load(return_scene_id);
}
static void button_ok() {
	sound_manager_play(resource_manager_getChunkFromPath("sounds/sfx_click1.ogg"), SOUND_MANAGER_CH_SFX2);

	// save resolution settings
	int w = GAME_SAVE_CONFIG_RESOLUTION_W_DEFAULT, h = GAME_SAVE_CONFIG_RESOLUTION_H_DEFAULT;
	if (video_item_index == VIDEO_ITEM_ID_640x480) { w = 640;  h = 480; }
	else if (video_item_index == VIDEO_ITEM_ID_1280x720)  { w = 1280; h = 720; }

	int ret = 0;
	if (game_window_set_resolution(w, h) == 0) {
		ret = game_save_set_config_resolution(w, h);
	}

	// save music settings
	ret &= game_save_set_config_music_volume(music_volume_value);

	// save sfx settings
	ret &= game_save_set_config_sfx_volume(sfx_volume_value);

	// save *.ini
	if (ret == 0) {
		game_save_config_save();
		sound_manager_init_track_volume();
	}

	scene_stat = SCENE_STAT_IDLE;
	scene_manager_load(return_scene_id);
}
static void video_button_left() {
	if (video_item_index > 0) {
		sound_manager_play(resource_manager_getChunkFromPath("sounds/sfx_click1.ogg"), SOUND_MANAGER_CH_SFX2);
		video_item_index -= 1;
	}
}
static void video_button_right() {
	if (video_item_index < VIDEO_ITEM_ID_SIZE - 1) {
		sound_manager_play(resource_manager_getChunkFromPath("sounds/sfx_click1.ogg"), SOUND_MANAGER_CH_SFX2);
		video_item_index += 1;
	}
}
static void music_button_left() {
	if (music_volume_value > MUSIC_VALUE_MIN) {
		sound_manager_play(resource_manager_getChunkFromPath("sounds/sfx_click1.ogg"), SOUND_MANAGER_CH_SFX2);
		music_volume_value -= 1;
	}
	tex_info_reset_music_volume();
}
static void music_button_right() {
	if (music_volume_value < MUSIC_VALUE_MAX) {
		sound_manager_play(resource_manager_getChunkFromPath("sounds/sfx_click1.ogg"), SOUND_MANAGER_CH_SFX2);
		music_volume_value += 1;
	}
	tex_info_reset_music_volume();
}
static void music_button_click() {
	int guid_line_x_min = music_guid_line.dst_rect.x;
	int guid_line_x_max = guid_line_x_min + music_guid_line.dst_rect.w;

	// set clicked volume
	int new_value = music_volume_value;
	if (current_mouse_x < guid_line_x_min) new_value = MUSIC_VALUE_MIN;
	else if (current_mouse_x > guid_line_x_max) new_value = MUSIC_VALUE_MAX;
	else {
		new_value = (current_mouse_x - guid_line_x_min) * MUSIC_VALUE_MAX / music_guid_line.dst_rect.w;
	}

	if (music_volume_value != new_value) {
		sound_manager_play(resource_manager_getChunkFromPath("sounds/sfx_click1.ogg"), SOUND_MANAGER_CH_SFX2);
		music_volume_value = new_value;
	}

	tex_info_reset_music_volume();
}
static void sfx_button_left() {
	if (sfx_volume_value > SFX_VALUE_MIN) {
		sound_manager_play(resource_manager_getChunkFromPath("sounds/sfx_click1.ogg"), SOUND_MANAGER_CH_SFX2);
		sfx_volume_value -= 1;
	}
	tex_info_reset_sfx_volume();
}
static void sfx_button_right() {
	if (sfx_volume_value < SFX_VALUE_MAX) {
		sound_manager_play(resource_manager_getChunkFromPath("sounds/sfx_click1.ogg"), SOUND_MANAGER_CH_SFX2);
		sfx_volume_value += 1;
	}
	tex_info_reset_sfx_volume();
}
static void sfx_button_click() {
	int guid_line_x_min = sfx_guid_line.dst_rect.x;
	int guid_line_x_max = guid_line_x_min + sfx_guid_line.dst_rect.w;

	// set clicked volume
	int new_value = sfx_volume_value;
	if (current_mouse_x < guid_line_x_min) new_value = SFX_VALUE_MIN;
	else if (current_mouse_x > guid_line_x_max) new_value = SFX_VALUE_MAX;
	else {
		new_value = (current_mouse_x - guid_line_x_min) * SFX_VALUE_MAX / sfx_guid_line.dst_rect.w;
	}

	if (sfx_volume_value != new_value) {
		sound_manager_play(resource_manager_getChunkFromPath("sounds/sfx_click1.ogg"), SOUND_MANAGER_CH_SFX2);
		sfx_volume_value = new_value;
	}

	tex_info_reset_sfx_volume();
}


static void tex_info_reset_music_volume()
{
	int w, h;
	int w_pos = tex_info_music_text.dst_rect_base.x;
	int h_pos = tex_info_music_text.dst_rect_base.y;

	char c_buf_vol[4] = { '\0' };
	game_utils_string_itoa(music_volume_value, c_buf_vol, 4, 10);

	tex_info_music_text.res_img = resource_manager_getFontTextureFromPath(c_buf_vol);
	int ret = GUI_QueryTexture(tex_info_music_text.res_img, NULL, NULL, &w, &h);
	if (ret == 0) {
		GUI_tex_info_init_rect(&tex_info_music_text, w, h, w_pos, h_pos);
	}

	// set bar positon
	int guid_line_x_min = music_guid_line.dst_rect_base.x;
	int guid_line_x_max = guid_line_x_min + music_guid_line.dst_rect_base.w;
	tex_info_music_slider.dst_rect_base.x = guid_line_x_min + (music_volume_value * music_guid_line.dst_rect_base.w / MUSIC_VALUE_MAX) - (tex_info_music_slider.src_rect.w/2);
	GUI_tex_info_reset(&tex_info_music_slider);
}
static void tex_info_reset_sfx_volume()
{
	int w, h;
	int w_pos = tex_info_sfx_text.dst_rect_base.x;
	int h_pos = tex_info_sfx_text.dst_rect_base.y;

	char c_buf_vol[4] = { '\0' };
	game_utils_string_itoa(sfx_volume_value, c_buf_vol, 4, 10);

	tex_info_sfx_text.res_img = resource_manager_getFontTextureFromPath(c_buf_vol);
	int ret = GUI_QueryTexture(tex_info_sfx_text.res_img, NULL, NULL, &w, &h);
	if (ret == 0) {
		GUI_tex_info_init_rect(&tex_info_sfx_text, w, h, w_pos, h_pos);
	}

	// set bar positon
	int guid_line_x_min = sfx_guid_line.dst_rect_base.x;
	int guid_line_x_max = guid_line_x_min + sfx_guid_line.dst_rect_base.w;
	tex_info_sfx_slider.dst_rect_base.x = guid_line_x_min + (sfx_volume_value * sfx_guid_line.dst_rect_base.w / SFX_VALUE_MAX) - (tex_info_sfx_slider.src_rect.w / 2);
	GUI_tex_info_reset(&tex_info_sfx_slider);
}

// init draw items
static void tex_info_init()
{
	int w, h;
	int w_pos = 0, h_pos = 0;

	// title
	tex_info[SCENE_SETTINGS_MENU_ID_TITLE].res_img = resource_manager_getFontTextureFromPath("Settings");
	int ret = GUI_QueryTexture(tex_info[SCENE_SETTINGS_MENU_ID_TITLE].res_img, NULL, NULL, &w, &h);
	if (ret == 0) {
		GUI_tex_info_init_rect(&tex_info[SCENE_SETTINGS_MENU_ID_TITLE], w, h, 20, 20);
		w_pos = 20 + 32;
		h_pos = 20 + h + 20;
	}

	// video (left binding "video < 9999x9999 >")
	tex_info[SCENE_SETTINGS_MENU_ID_LABEL_VIDEO].res_img = resource_manager_getFontTextureFromPath("Video");
	ret = GUI_QueryTexture(tex_info[SCENE_SETTINGS_MENU_ID_LABEL_VIDEO].res_img, NULL, NULL, &w, &h);
	if (ret == 0) {
		GUI_tex_info_init_rect(&tex_info[SCENE_SETTINGS_MENU_ID_LABEL_VIDEO], w, h, w_pos, h_pos);
		w_pos = w_pos + w + 100;
	}

	tex_info_video_button[VIDEO_BUTTON_ITEM_ID_LEFT].res_img = resource_manager_getTextureFromPath("{scale_mode:linear}images/gui/button/cursor_l.png");
	ret = GUI_QueryTexture(tex_info_video_button[VIDEO_BUTTON_ITEM_ID_LEFT].res_img, NULL, NULL, &w, &h);
	if (ret == 0) {
		GUI_tex_info_init_rect(&tex_info_video_button[VIDEO_BUTTON_ITEM_ID_LEFT], w, h, w_pos, h_pos);
		video_button_items[VIDEO_BUTTON_ITEM_LEFT].tex_info_id = VIDEO_BUTTON_ITEM_ID_LEFT;
		video_button_items[VIDEO_BUTTON_ITEM_LEFT].dst_rect = tex_info_video_button[VIDEO_BUTTON_ITEM_ID_LEFT].dst_rect;
		video_button_items[VIDEO_BUTTON_ITEM_LEFT].dst_rect_base = tex_info_video_button[VIDEO_BUTTON_ITEM_ID_LEFT].dst_rect_base;
		video_button_items[VIDEO_BUTTON_ITEM_LEFT].func = NULL;
		w_pos = w_pos + w + 20;
	}

	tex_info_video[VIDEO_ITEM_ID_640x480].res_img = resource_manager_getFontTextureFromPath("640x480");
	ret = GUI_QueryTexture(tex_info_video[VIDEO_ITEM_ID_640x480].res_img, NULL, NULL, &w, &h);
	if (ret == 0) {
		GUI_tex_info_init_rect(&tex_info_video[VIDEO_ITEM_ID_640x480], w, h, w_pos, h_pos);
	}
	tex_info_video[VIDEO_ITEM_ID_1280x720].res_img = resource_manager_getFontTextureFromPath("1280x720");
	ret = GUI_QueryTexture(tex_info_video[VIDEO_ITEM_ID_1280x720].res_img, NULL, NULL, &w, &h);
	if (ret == 0) {
		GUI_tex_info_init_rect(&tex_info_video[VIDEO_ITEM_ID_1280x720], w, h, w_pos, h_pos);
		w_pos = w_pos + w + 20;
	}

	tex_info_video_button[VIDEO_BUTTON_ITEM_ID_RIGHT].res_img = resource_manager_getTextureFromPath("{scale_mode:linear}images/gui/button/cursor.png");
	ret = GUI_QueryTexture(tex_info_video_button[VIDEO_BUTTON_ITEM_ID_RIGHT].res_img, NULL, NULL, &w, &h);
	if (ret == 0) {
		GUI_tex_info_init_rect(&tex_info_video_button[VIDEO_BUTTON_ITEM_ID_RIGHT], w, h, w_pos, h_pos);
		video_button_items[VIDEO_BUTTON_ITEM_RIGHT].tex_info_id = VIDEO_BUTTON_ITEM_ID_RIGHT;
		video_button_items[VIDEO_BUTTON_ITEM_RIGHT].dst_rect = tex_info_video_button[VIDEO_BUTTON_ITEM_ID_RIGHT].dst_rect;
		video_button_items[VIDEO_BUTTON_ITEM_RIGHT].dst_rect_base = tex_info_video_button[VIDEO_BUTTON_ITEM_ID_RIGHT].dst_rect_base;
		video_button_items[VIDEO_BUTTON_ITEM_RIGHT].func = NULL;
		w_pos = w_pos + w + 20;
	}

	// set group region
	int group_region_video_top    = tex_info[SCENE_SETTINGS_MENU_ID_LABEL_VIDEO].dst_rect_base.y - 10;
	int group_region_video_left   = tex_info[SCENE_SETTINGS_MENU_ID_LABEL_VIDEO].dst_rect_base.x - 10;
	int group_region_video_right  = tex_info_video_button[VIDEO_BUTTON_ITEM_ID_RIGHT].dst_rect_base.x + tex_info_video_button[VIDEO_BUTTON_ITEM_ID_RIGHT].dst_rect_base.w + 10;
	int group_region_video_bottom = tex_info_video_button[VIDEO_BUTTON_ITEM_ID_RIGHT].dst_rect_base.y + tex_info_video_button[VIDEO_BUTTON_ITEM_ID_RIGHT].dst_rect_base.h + 10;
	int group_region_video_w = group_region_video_right - group_region_video_left;
	int group_region_video_h = group_region_video_bottom - group_region_video_top;
	GUI_rect_region_init_rect(&group_region_video,
		group_region_video_left, group_region_video_top, group_region_video_w, group_region_video_h);


	// music (left binding "music vol 100 ------[]--")
	tex_info[SCENE_SETTINGS_MENU_ID_LABEL_MUSIC].res_img = resource_manager_getFontTextureFromPath("Music Vol");
	ret = GUI_QueryTexture(tex_info[SCENE_SETTINGS_MENU_ID_LABEL_MUSIC].res_img, NULL, NULL, &w, &h);
	if (ret == 0) {
		w_pos = 20 + 32;
		h_pos = h_pos + tex_info[SCENE_SETTINGS_MENU_ID_LABEL_VIDEO].dst_rect_base.h + 20;
		GUI_tex_info_init_rect(&tex_info[SCENE_SETTINGS_MENU_ID_LABEL_MUSIC], w, h, w_pos, h_pos);
		//w_pos = w_pos + w + 100;
	}

	tex_info_music_text.res_img = resource_manager_getFontTextureFromPath("  0");
	ret = GUI_QueryTexture(tex_info_music_text.res_img, NULL, NULL, &w, &h);
	if (ret == 0) {
		w_pos = tex_info_video[VIDEO_ITEM_ID_1280x720].dst_rect_base.x;
		GUI_tex_info_init_rect(&tex_info_music_text, w, h, w_pos, h_pos);
		w_pos = w_pos + w + 40;
	}

	tex_info_music_slider.res_img = resource_manager_getTextureFromPath("{scale_mode:linear}images/gui/button/bar.png");
	ret = GUI_QueryTexture(tex_info_music_slider.res_img, NULL, NULL, &w, &h);
	if (ret == 0) {
		GUI_tex_info_init_rect(&tex_info_music_slider, w, h, w_pos, h_pos);
	}

	int guid_line_w = (SCREEN_WIDTH - 200) - w_pos;
	int guid_line_h = 4;
	int guid_line_offset_x = w_pos;
	int guid_line_offset_y = h_pos + (h/2) - (guid_line_h/2);
	GUI_rect_region_init_rect(&music_guid_line, guid_line_offset_x, guid_line_offset_y, guid_line_w, guid_line_h);

	// set group region
	int group_region_music_top = tex_info[SCENE_SETTINGS_MENU_ID_LABEL_MUSIC].dst_rect_base.y - 10;
	int group_region_music_left = tex_info[SCENE_SETTINGS_MENU_ID_LABEL_MUSIC].dst_rect_base.x - 10;
	int group_region_music_right = music_guid_line.dst_rect_base.x + music_guid_line.dst_rect_base.w + 20;
	int group_region_music_bottom = tex_info_music_slider.dst_rect_base.y + tex_info_music_slider.dst_rect_base.h + 10;
	int group_region_music_w = group_region_music_right - group_region_music_left;
	int group_region_music_h = group_region_music_bottom - group_region_music_top;
	GUI_rect_region_init_rect(&group_region_music,
		group_region_music_left, group_region_music_top, group_region_music_w, group_region_music_h);

	// set click region
	music_slider_items.tex_info_id = -1;
	music_slider_items.func = NULL;
	music_slider_items.dst_rect_base = { music_guid_line.dst_rect_base.x - 20, tex_info_music_slider.dst_rect_base.y, music_guid_line.dst_rect_base.w + 40, tex_info_music_slider.dst_rect_base.h };
	GUI_gui_item_reset(&music_slider_items);


	// sfx (left binding "sfx vol 100 ------[]--")
	tex_info[SCENE_SETTINGS_MENU_ID_LABEL_SFX].res_img = resource_manager_getFontTextureFromPath("SFX Vol");
	ret = GUI_QueryTexture(tex_info[SCENE_SETTINGS_MENU_ID_LABEL_SFX].res_img, NULL, NULL, &w, &h);
	if (ret == 0) {
		w_pos = 20 + 32;
		h_pos = h_pos + tex_info[SCENE_SETTINGS_MENU_ID_LABEL_MUSIC].dst_rect_base.h + 20;
		GUI_tex_info_init_rect(&tex_info[SCENE_SETTINGS_MENU_ID_LABEL_SFX], w, h, w_pos, h_pos);
	}

	tex_info_sfx_text.res_img = resource_manager_getFontTextureFromPath("  0");
	ret = GUI_QueryTexture(tex_info_sfx_text.res_img, NULL, NULL, &w, &h);
	if (ret == 0) {
		w_pos = tex_info_video[VIDEO_ITEM_ID_1280x720].dst_rect_base.x;
		GUI_tex_info_init_rect(&tex_info_sfx_text, w, h, w_pos, h_pos);
		w_pos = w_pos + w + 40;
	}

	tex_info_sfx_slider.res_img = resource_manager_getTextureFromPath("{scale_mode:linear}images/gui/button/bar.png");
	ret = GUI_QueryTexture(tex_info_sfx_slider.res_img, NULL, NULL, &w, &h);
	if (ret == 0) {
		GUI_tex_info_init_rect(&tex_info_sfx_slider, w, h, w_pos, h_pos);
	}

	guid_line_w = (SCREEN_WIDTH - 200) - w_pos;
	guid_line_h = 4;
	guid_line_offset_x = w_pos;
	guid_line_offset_y = h_pos + (h / 2) - (guid_line_h / 2);
	GUI_rect_region_init_rect(&sfx_guid_line, guid_line_offset_x, guid_line_offset_y, guid_line_w, guid_line_h);

	// set group region
	int group_region_sfx_top = tex_info[SCENE_SETTINGS_MENU_ID_LABEL_SFX].dst_rect_base.y - 10;
	int group_region_sfx_left = tex_info[SCENE_SETTINGS_MENU_ID_LABEL_SFX].dst_rect_base.x - 10;
	int group_region_sfx_right = sfx_guid_line.dst_rect_base.x + sfx_guid_line.dst_rect_base.w + 20;
	int group_region_sfx_bottom = tex_info_sfx_slider.dst_rect_base.y + tex_info_sfx_slider.dst_rect_base.h + 10;
	int group_region_sfx_w = group_region_sfx_right - group_region_sfx_left;
	int group_region_sfx_h = group_region_sfx_bottom - group_region_sfx_top;
	GUI_rect_region_init_rect(&group_region_sfx,
		group_region_sfx_left, group_region_sfx_top, group_region_sfx_w, group_region_sfx_h);

	// set click region
	sfx_slider_items.tex_info_id = -1;
	sfx_slider_items.func = NULL;
	sfx_slider_items.dst_rect_base = { sfx_guid_line.dst_rect_base.x - 20, tex_info_sfx_slider.dst_rect_base.y, sfx_guid_line.dst_rect_base.w + 40, tex_info_sfx_slider.dst_rect_base.h };
	GUI_gui_item_reset(&sfx_slider_items);


	// button (right binding [default] [cancel] [ok])
	tex_info[SCENE_SETTINGS_MENU_ID_OK].res_img = resource_manager_getTextureFromPath("{scale_mode:linear}images/gui/button/ok.png");
	ret = GUI_QueryTexture(tex_info[SCENE_SETTINGS_MENU_ID_OK].res_img, NULL, NULL, &w, &h);
	if (ret == 0) {
		w_pos = SCREEN_WIDTH - w - 10;
		h_pos = SCREEN_HEIGHT - h - 10;
		GUI_tex_info_init_rect(&tex_info[SCENE_SETTINGS_MENU_ID_OK], w, h, w_pos, h_pos);
		button_items[BUTTON_ITEM_OK].tex_info_id = SCENE_SETTINGS_MENU_ID_OK;
		button_items[BUTTON_ITEM_OK].dst_rect = tex_info[SCENE_SETTINGS_MENU_ID_OK].dst_rect;
		button_items[BUTTON_ITEM_OK].dst_rect_base = tex_info[SCENE_SETTINGS_MENU_ID_OK].dst_rect_base;
		button_items[BUTTON_ITEM_OK].func = NULL;
	}

	tex_info[SCENE_SETTINGS_MENU_ID_CANCEL].res_img = resource_manager_getTextureFromPath("{scale_mode:linear}images/gui/button/cancel.png");
	ret = GUI_QueryTexture(tex_info[SCENE_SETTINGS_MENU_ID_CANCEL].res_img, NULL, NULL, &w, &h);
	if (ret == 0) {
		w_pos = w_pos - w - 10;
		//h_pos = dialog_bottom - h - 10;
		GUI_tex_info_init_rect(&tex_info[SCENE_SETTINGS_MENU_ID_CANCEL], w, h, w_pos, h_pos);
		button_items[BUTTON_ITEM_CANCEL].tex_info_id = SCENE_SETTINGS_MENU_ID_CANCEL;
		button_items[BUTTON_ITEM_CANCEL].dst_rect = tex_info[SCENE_SETTINGS_MENU_ID_CANCEL].dst_rect;
		button_items[BUTTON_ITEM_CANCEL].dst_rect_base = tex_info[SCENE_SETTINGS_MENU_ID_CANCEL].dst_rect_base;
		button_items[BUTTON_ITEM_CANCEL].func = NULL;
	}

	tex_info[SCENE_SETTINGS_MENU_ID_DEFAULT].res_img = resource_manager_getTextureFromPath("{scale_mode:linear}images/gui/button/default.png");
	ret = GUI_QueryTexture(tex_info[SCENE_SETTINGS_MENU_ID_DEFAULT].res_img, NULL, NULL, &w, &h);
	if (ret == 0) {
		w_pos = w_pos - w - 10;
		//h_pos = dialog_bottom - h - 10;
		GUI_tex_info_init_rect(&tex_info[SCENE_SETTINGS_MENU_ID_DEFAULT], w, h, w_pos, h_pos);
		button_items[BUTTON_ITEM_DEFAULT].tex_info_id = SCENE_SETTINGS_MENU_ID_DEFAULT;
		button_items[BUTTON_ITEM_DEFAULT].dst_rect = tex_info[SCENE_SETTINGS_MENU_ID_DEFAULT].dst_rect;
		button_items[BUTTON_ITEM_DEFAULT].dst_rect_base = tex_info[SCENE_SETTINGS_MENU_ID_DEFAULT].dst_rect_base;
		button_items[BUTTON_ITEM_DEFAULT].func = NULL;
	}
}

void scene_settings_menu_init() {
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
	resource_manager_load_dat((char*)"scenes/scene_settings_menu.dat");

	// set texture position
	tex_info_init();

	// set button data
	button_items[BUTTON_ITEM_DEFAULT].mouse_stat = 0;
	button_items[BUTTON_ITEM_DEFAULT].func = button_default;
	button_items[BUTTON_ITEM_CANCEL].mouse_stat = 0;
	button_items[BUTTON_ITEM_CANCEL].func = button_cancel;
	button_items[BUTTON_ITEM_OK].mouse_stat = 0;
	button_items[BUTTON_ITEM_OK].func = button_ok;
	button_index = 0;

	// set video button data
	video_button_items[VIDEO_BUTTON_ITEM_LEFT].mouse_stat = 0;
	video_button_items[VIDEO_BUTTON_ITEM_LEFT].func = video_button_left;
	video_button_items[VIDEO_BUTTON_ITEM_RIGHT].mouse_stat = 0;
	video_button_items[VIDEO_BUTTON_ITEM_RIGHT].func = video_button_right;
	video_item_index = VIDEO_ITEM_ID_1280x720;

	// set music data
	music_slider_items.mouse_stat = 0;
	music_slider_items.func = music_button_click;

	// set sfx data
	sfx_slider_items.mouse_stat = 0;
	sfx_slider_items.func = sfx_button_click;
}

SceneManagerFunc* scene_settings_menu_get_func() {
	return &scene_func;
}
