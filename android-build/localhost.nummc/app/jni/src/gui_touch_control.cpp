#include "game_common.h"
#include "gui_common.h"
#include "gui_touch_control.h"

#include "game_log.h"
#include "game_key_event.h"
#include "game_mouse_event.h"
#include "game_window.h"
#include "resource_manager.h"
#include "game_utils.h"
#include "game_timer.h"

// stick LS only
#define GUI_TOUCH_CTRL_LS_ONLY

// view touch ctrl
#define VIEW_TOUCH_CTRL_SCALE(X) ((X) * view_touch_ctrl_scale)
#define VIEW_TOUCH_CTRL_SCALE_DEFAULT  2
static int view_touch_ctrl_scale = VIEW_TOUCH_CTRL_SCALE_DEFAULT;

//
// draw data
//
#ifdef GUI_TOUCH_CTRL_LS_ONLY
#define GUI_TOUCH_CTRL_ID_ESCAPE  0
#define GUI_TOUCH_CTRL_ID_RETURN  1
#define GUI_TOUCH_CTRL_ID_E       2
#define GUI_TOUCH_CTRL_ID_Q       3
#define GUI_TOUCH_CTRL_ID_SPACE   4
#define GUI_TOUCH_CTRL_ID_UP      5
#define GUI_TOUCH_CTRL_ID_DOWN    6
#define GUI_TOUCH_CTRL_ID_LEFT    7
#define GUI_TOUCH_CTRL_ID_RIGHT   8
#define GUI_TOUCH_CTRL_ID_END     9
#else
#define GUI_TOUCH_CTRL_ID_ESCAPE  0
#define GUI_TOUCH_CTRL_ID_RETURN  1
#define GUI_TOUCH_CTRL_ID_E       2
#define GUI_TOUCH_CTRL_ID_Q       3
#define GUI_TOUCH_CTRL_ID_SPACE   4
#define GUI_TOUCH_CTRL_ID_END     5
#endif

static tex_info_t tex_info[GUI_TOUCH_CTRL_ID_END];

//
// button data
//
#ifdef GUI_TOUCH_CTRL_LS_ONLY
#define BUTTON_ITEM_ESCAPE  0
#define BUTTON_ITEM_RETURN  1
#define BUTTON_ITEM_E       2
#define BUTTON_ITEM_Q       3
#define BUTTON_ITEM_SPACE   4
#define BUTTON_ITEM_UP      5
#define BUTTON_ITEM_DOWN    6
#define BUTTON_ITEM_LEFT    7
#define BUTTON_ITEM_RIGHT   8
#define BUTTON_ITEM_SIZE    9
#else
#define BUTTON_ITEM_ESCAPE  0
#define BUTTON_ITEM_RETURN  1
#define BUTTON_ITEM_E       2
#define BUTTON_ITEM_Q       3
#define BUTTON_ITEM_SPACE   4
#define BUTTON_ITEM_SIZE    5
#endif

#define BUTTON_ITEM_NONE       (-1)
#define BUTTON_ITEM_FOCUS_ON   (-2)

static gui_item_t button_items[BUTTON_ITEM_SIZE];
static int button_keyWait[BUTTON_ITEM_SIZE];

// block key repating
#define GUI_TOUCH_CTRL_BTN_WAIT_TIMER 300
static const int button_wait_timer = GUI_TOUCH_CTRL_BTN_WAIT_TIMER;
static int button_current_time;

typedef struct _touch_ctrl_tex_info_list_t touch_ctrl_tex_info_list_t;
struct _touch_ctrl_tex_info_list_t {
	int id;
	int button_id;
	const char* path;
	int grid_x;			// place at ((64+10) * grid_x)	// or place at (w - (64+10) * -grid_x)
	int grid_y;			// place at ((64+10) * grid_y)	// or place at (h - (64+10) * -grid_y)
						// (64+10) = button size:64 + margin:10
};

#ifdef GUI_TOUCH_CTRL_LS_ONLY
static touch_ctrl_tex_info_list_t tex_info_list[GUI_TOUCH_CTRL_ID_END] = {
	{GUI_TOUCH_CTRL_ID_ESCAPE,	BUTTON_ITEM_ESCAPE,	"{scale_mode:linear}images/gui/touch_ctrl/esc.png",		 -2, -5},
	{GUI_TOUCH_CTRL_ID_RETURN,	BUTTON_ITEM_RETURN,	"{scale_mode:linear}images/gui/touch_ctrl/ent.png",		 -2, -4},
	{GUI_TOUCH_CTRL_ID_E,		BUTTON_ITEM_E,		"{scale_mode:linear}images/gui/touch_ctrl/e.png",		 -8, -1},
	{GUI_TOUCH_CTRL_ID_Q,		BUTTON_ITEM_Q,		"{scale_mode:linear}images/gui/touch_ctrl/q.png",		 -7, -1},
	{GUI_TOUCH_CTRL_ID_SPACE,	BUTTON_ITEM_SPACE,	"{scale_mode:linear}images/gui/touch_ctrl/space.png",	 -6, -1},
    {GUI_TOUCH_CTRL_ID_UP,		BUTTON_ITEM_UP,		"{scale_mode:linear}images/gui/touch_ctrl/up.png",		 -2, -2},
    {GUI_TOUCH_CTRL_ID_DOWN,	BUTTON_ITEM_DOWN,	"{scale_mode:linear}images/gui/touch_ctrl/down.png",	 -2, -1},
    {GUI_TOUCH_CTRL_ID_LEFT,	BUTTON_ITEM_LEFT,	"{scale_mode:linear}images/gui/touch_ctrl/left.png",	 -3, -1},
    {GUI_TOUCH_CTRL_ID_RIGHT,	BUTTON_ITEM_RIGHT,	"{scale_mode:linear}images/gui/touch_ctrl/right.png",	 -1, -1},
};
#else
static touch_ctrl_tex_info_list_t tex_info_list[GUI_TOUCH_CTRL_ID_END] = {
	{GUI_TOUCH_CTRL_ID_ESCAPE,	BUTTON_ITEM_ESCAPE,	"{scale_mode:linear}images/gui/touch_ctrl/esc.png",		 -2, -5},
	{GUI_TOUCH_CTRL_ID_RETURN,	BUTTON_ITEM_RETURN,	"{scale_mode:linear}images/gui/touch_ctrl/ent.png",		 -2, -4},
	{GUI_TOUCH_CTRL_ID_E,		BUTTON_ITEM_E,		"{scale_mode:linear}images/gui/touch_ctrl/e.png",		 -8, -1},
	{GUI_TOUCH_CTRL_ID_Q,		BUTTON_ITEM_Q,		"{scale_mode:linear}images/gui/touch_ctrl/q.png",		 -7, -1},
	{GUI_TOUCH_CTRL_ID_SPACE,	BUTTON_ITEM_SPACE,	"{scale_mode:linear}images/gui/touch_ctrl/space.png",	 -6, -1},
};
#endif

#ifdef GUI_TOUCH_CTRL_LS_ONLY
static const int button_item_scancode_tbl[BUTTON_ITEM_SIZE] =
{
	SDL_SCANCODE_ESCAPE,	// BUTTON_ITEM_ESCAPE  0
	SDL_SCANCODE_RETURN,	// BUTTON_ITEM_RETURN  1
	SDL_SCANCODE_E,			// BUTTON_ITEM_E       2
	SDL_SCANCODE_Q,			// BUTTON_ITEM_Q       3
	SDL_SCANCODE_SPACE,		// BUTTON_ITEM_SPACE   4
    SDL_SCANCODE_UP,		// BUTTON_ITEM_UP      5
    SDL_SCANCODE_DOWN,		// BUTTON_ITEM_DOWN    6
    SDL_SCANCODE_LEFT,		// BUTTON_ITEM_LEFT    7
    SDL_SCANCODE_RIGHT,		// BUTTON_ITEM_RIGHT   8
};
#else
static const int button_item_scancode_tbl[BUTTON_ITEM_SIZE] =
{
	SDL_SCANCODE_ESCAPE,	// BUTTON_ITEM_ESCAPE  0
	SDL_SCANCODE_RETURN,	// BUTTON_ITEM_RETURN  1
	SDL_SCANCODE_E,			// BUTTON_ITEM_E       2
	SDL_SCANCODE_Q,			// BUTTON_ITEM_Q       3
	SDL_SCANCODE_SPACE,		// BUTTON_ITEM_SPACE   4
};
#endif

//
// touch event buffer
//
typedef struct _gui_touch_control_event_t gui_touch_control_event_t;
struct _gui_touch_control_event_t {
	SDL_FingerID finger_id;
	int x;
	int y;
	int stat;
	int focus_button_id;
};

#define TOUCH_EVENT_FINGER_ID_NONE (-1)
#define TOUCH_EVENT_STAT_NONE      (-1)
#define TOUCH_EVENT_BUF_SIZE 5
gui_touch_control_event_t touch_event_buf[TOUCH_EVENT_BUF_SIZE];

//
// stick key (prefix "s_")
//

// draw data
#ifdef GUI_TOUCH_CTRL_LS_ONLY
#define GUI_TOUCH_CTRL_ID_LS    0
#define GUI_TOUCH_CTRL_ID_S_END 1
#else
#define GUI_TOUCH_CTRL_ID_LS    0
#define GUI_TOUCH_CTRL_ID_RS    1
#define GUI_TOUCH_CTRL_ID_S_END 2
#endif

#define GUI_TOUCH_CTRL_ID_STICK (0x10)

#ifdef GUI_TOUCH_CTRL_LS_ONLY
#define GUI_TOUCH_CTRL_ID_S_LS  (GUI_TOUCH_CTRL_ID_STICK | GUI_TOUCH_CTRL_ID_LS)
#else
#define GUI_TOUCH_CTRL_ID_S_LS  (GUI_TOUCH_CTRL_ID_STICK | GUI_TOUCH_CTRL_ID_LS)
#define GUI_TOUCH_CTRL_ID_S_RS  (GUI_TOUCH_CTRL_ID_STICK | GUI_TOUCH_CTRL_ID_RS)
#endif

static tex_info_t tex_info_s[GUI_TOUCH_CTRL_ID_S_END];
static tex_info_t tex_info_s_mark[GUI_TOUCH_CTRL_ID_S_END];

// button data
#ifdef GUI_TOUCH_CTRL_LS_ONLY
#define BUTTON_ITEM_LS     0
#define BUTTON_ITEM_S_SIZE 1
#else
#define BUTTON_ITEM_LS     0
#define BUTTON_ITEM_RS     1
#define BUTTON_ITEM_S_SIZE 2
#endif

// stick direction data
#define GUI_TOUCH_CTRL_DIRECT_UP    0
#define GUI_TOUCH_CTRL_DIRECT_DOWN  1
#define GUI_TOUCH_CTRL_DIRECT_LEFT  2
#define GUI_TOUCH_CTRL_DIRECT_RIGHT 3
#define GUI_TOUCH_CTRL_DIRECT_END   4

// region to react
#define GUI_TOUCH_CTRL_S_REGION_MIN  (VIEW_TOUCH_CTRL_SCALE(24))
#define GUI_TOUCH_CTRL_S_REGION_MAX  (VIEW_TOUCH_CTRL_SCALE(96))
#define GUI_TOUCH_CTRL_S_COMBO_X_MIN (VIEW_TOUCH_CTRL_SCALE(16))
#define GUI_TOUCH_CTRL_S_COMBO_Y_MIN (VIEW_TOUCH_CTRL_SCALE(16))

static gui_item_t button_items_s[BUTTON_ITEM_S_SIZE];
static int button_items_s_stat[BUTTON_ITEM_S_SIZE][GUI_TOUCH_CTRL_DIRECT_END];
static int button_items_s_new_stat[BUTTON_ITEM_S_SIZE][GUI_TOUCH_CTRL_DIRECT_END];

#ifdef GUI_TOUCH_CTRL_LS_ONLY
static touch_ctrl_tex_info_list_t tex_info_s_list[GUI_TOUCH_CTRL_ID_S_END] = {
	{GUI_TOUCH_CTRL_ID_LS,	BUTTON_ITEM_LS,	"{scale_mode:linear}images/gui/touch_ctrl/left_stick.png",		 0, -3},
};
#else
static touch_ctrl_tex_info_list_t tex_info_s_list[GUI_TOUCH_CTRL_ID_S_END] = {
	{GUI_TOUCH_CTRL_ID_LS,	BUTTON_ITEM_LS,	"{scale_mode:linear}images/gui/touch_ctrl/left_stick.png",		 0, -3},
	{GUI_TOUCH_CTRL_ID_RS,	BUTTON_ITEM_RS,	"{scale_mode:linear}images/gui/touch_ctrl/right_stick.png",		-3, -3},
};
#endif

#ifdef GUI_TOUCH_CTRL_LS_ONLY
static const int s_scancode_tbl[BUTTON_ITEM_S_SIZE][GUI_TOUCH_CTRL_DIRECT_END] =
{
	// LSB
	{
		SDL_SCANCODE_W, //GUI_TOUCH_CTRL_DIRECT_UP    0
		SDL_SCANCODE_S, //GUI_TOUCH_CTRL_DIRECT_DOWN  1
		SDL_SCANCODE_A, //GUI_TOUCH_CTRL_DIRECT_LEFT  2
		SDL_SCANCODE_D, //GUI_TOUCH_CTRL_DIRECT_RIGHT 3
	}
};
#else
static const int s_scancode_tbl[BUTTON_ITEM_S_SIZE][GUI_TOUCH_CTRL_DIRECT_END] =
{
	// LSB
	{
		SDL_SCANCODE_W, //GUI_TOUCH_CTRL_DIRECT_UP    0
		SDL_SCANCODE_S, //GUI_TOUCH_CTRL_DIRECT_DOWN  1
		SDL_SCANCODE_A, //GUI_TOUCH_CTRL_DIRECT_LEFT  2
		SDL_SCANCODE_D, //GUI_TOUCH_CTRL_DIRECT_RIGHT 3
	}
	// RSB
	,{
		SDL_SCANCODE_UP,    //GUI_TOUCH_CTRL_DIRECT_UP    0
		SDL_SCANCODE_DOWN,  //GUI_TOUCH_CTRL_DIRECT_DOWN  1
		SDL_SCANCODE_LEFT,  //GUI_TOUCH_CTRL_DIRECT_LEFT  2
		SDL_SCANCODE_RIGHT, //GUI_TOUCH_CTRL_DIRECT_RIGHT 3
	}
};
#endif

static void send_pressed(int button_index) {
	if ((button_current_time - button_keyWait[button_index]) > GUI_TOUCH_CTRL_BTN_WAIT_TIMER) {
		button_keyWait[button_index] = button_current_time;
		keyStat[button_item_scancode_tbl[button_index]] = SDL_PRESSED;
		//LOG_DEBUG_CONSOLE("PRESSED=%d", button_index);
	}
}

static void send_released(int button_index) {
	keyStat[button_item_scancode_tbl[button_index]] = SDL_RELEASED;
}

// stick key emulator
static void GUI_t_ctrl_key_send() {
	for (int i = 0; i < BUTTON_ITEM_SIZE; i++) {
		int active_idx    = -1;
		int down_idx      = -1;
		int up_idx        = -1;
		int focus_out_idx = BUTTON_ITEM_NONE; // init:-1, focus_on:-2, foucus_out:button_id

		for (int te_i = 0; te_i < TOUCH_EVENT_BUF_SIZE; te_i++) {
			// skip empty buffer
			if (touch_event_buf[te_i].finger_id < 0) {
				continue;
			}

			// in the region of button rect
			if (game_utils_decision_internal(&tex_info[button_items[i].tex_info_id].dst_rect,
											 touch_event_buf[te_i].x, touch_event_buf[te_i].y)) {

				// set focus_on
				if (focus_out_idx != BUTTON_ITEM_FOCUS_ON) { focus_out_idx = BUTTON_ITEM_FOCUS_ON; }

				// set event index (coming first is high priority)
				if (touch_event_buf[te_i].stat == SDL_FINGERMOTION) {
					// set foucus button_id
					if(touch_event_buf[te_i].focus_button_id == BUTTON_ITEM_NONE) {
						touch_event_buf[te_i].focus_button_id = i;
					}

					if (active_idx < 0) {
						active_idx = te_i;
					}
					// multiple finger skip (clear stat)
					else {
						touch_event_buf[te_i].stat = TOUCH_EVENT_STAT_NONE;
					}
				}
				else if (touch_event_buf[te_i].stat == SDL_FINGERDOWN) {
					// set foucus button_id
					if(touch_event_buf[te_i].focus_button_id == BUTTON_ITEM_NONE) {
						touch_event_buf[te_i].focus_button_id = i;
					}

					if (down_idx < 0) {
						down_idx = te_i;
					}
					// multiple finger skip (clear stat)
					else {
						touch_event_buf[te_i].stat = TOUCH_EVENT_STAT_NONE;
					}
				}
				else if (touch_event_buf[te_i].stat == SDL_FINGERUP) {
					if (up_idx < 0) {
						up_idx = te_i;
					}
					// multiple finger skip (clear stat)
					else {
						touch_event_buf[te_i].stat = TOUCH_EVENT_STAT_NONE;
					}
				}
			}
			else {
				// check focus out
				if(touch_event_buf[te_i].focus_button_id == i) { // current button item
					// check multiple touch
					if (focus_out_idx == BUTTON_ITEM_NONE) { // (focus_out_idx != BUTTON_ITEM_FOCUS_ON)
						focus_out_idx = touch_event_buf[te_i].focus_button_id;
					}
					touch_event_buf[te_i].focus_button_id = BUTTON_ITEM_NONE;
				}
			}
		}

		if (active_idx >= 0) {
			button_items[i].mouse_stat |= GUI_BUTTON_ACTIVE;
			//button_items[i].mouse_stat |= GUI_BUTTON_CLICK;

			send_pressed(i);

			// clear stat (completed event)
			touch_event_buf[active_idx].stat = TOUCH_EVENT_STAT_NONE;
		}

		if (down_idx >= 0) {
			button_items[i].mouse_stat |= GUI_BUTTON_ACTIVE;
			button_items[i].mouse_stat |= GUI_BUTTON_CLICK;

			send_pressed(i);

			// clear stat (completed event)
			touch_event_buf[down_idx].stat = TOUCH_EVENT_STAT_NONE;
		}

		if ((up_idx >= 0) && (down_idx < 0)) { // multiple finger priority: down > up
			if (button_items[i].mouse_stat & GUI_BUTTON_ACTIVE) {
				button_items[i].mouse_stat &= ~GUI_BUTTON_CLICK;
				button_items[i].mouse_stat &= ~GUI_BUTTON_ACTIVE;

				send_released(i);
			}
		}

		// all finger focus out
		if (focus_out_idx >= 0) {
			if (button_items[i].mouse_stat & GUI_BUTTON_ACTIVE) {
				button_items[i].mouse_stat &= ~GUI_BUTTON_CLICK;
				button_items[i].mouse_stat &= ~GUI_BUTTON_ACTIVE;

				send_released(focus_out_idx);
			}
		}
	}
}

static bool GUI_t_ctrl_set_stick_stat(int button_s_index, int x, int y, bool single_direction)
{
	bool ret = false;

	// get center
	float x_center = tex_info_s[button_items_s[button_s_index].tex_info_id].dst_rect.x
					  + (tex_info_s[button_items_s[button_s_index].tex_info_id].dst_rect.w / 2.0f);
	float y_center = tex_info_s[button_items_s[button_s_index].tex_info_id].dst_rect.y
					  + (tex_info_s[button_items_s[button_s_index].tex_info_id].dst_rect.h / 2.0f);
	float delta_x = x - x_center;
	float delta_y = y - y_center;
	float abs_x = (delta_x >= 0) ? delta_x : -delta_x;
	float abs_y = (delta_y >= 0) ? delta_y : -delta_y;
	float dist2 = delta_x * delta_x + delta_y * delta_y;

	// clear new_stat
	button_items_s_new_stat[button_s_index][GUI_TOUCH_CTRL_DIRECT_UP]    &= ~GUI_BUTTON_ACTIVE;
	button_items_s_new_stat[button_s_index][GUI_TOUCH_CTRL_DIRECT_DOWN]  &= ~GUI_BUTTON_ACTIVE;
	button_items_s_new_stat[button_s_index][GUI_TOUCH_CTRL_DIRECT_LEFT]  &= ~GUI_BUTTON_ACTIVE;
	button_items_s_new_stat[button_s_index][GUI_TOUCH_CTRL_DIRECT_RIGHT] &= ~GUI_BUTTON_ACTIVE;

	// in the region (circle area)
	if ((GUI_TOUCH_CTRL_S_REGION_MIN * GUI_TOUCH_CTRL_S_REGION_MIN < dist2) &&
	    (GUI_TOUCH_CTRL_S_REGION_MAX * GUI_TOUCH_CTRL_S_REGION_MAX > dist2)) {

		// set return value
		ret = true;

		// |x| > |y|
		if (abs_x >= abs_y) {
			// set x-direction stat ACTIVE
			if (delta_x >= 0) {
				button_items_s_new_stat[button_s_index][GUI_TOUCH_CTRL_DIRECT_RIGHT] |= GUI_BUTTON_ACTIVE;
			}
			else {
				button_items_s_new_stat[button_s_index][GUI_TOUCH_CTRL_DIRECT_LEFT] |= GUI_BUTTON_ACTIVE;
			}

			// set y-direction stat ACTIVE
			if (!single_direction) {
				if (abs_y > GUI_TOUCH_CTRL_S_COMBO_Y_MIN) {
					if (delta_y >= 0) {
						button_items_s_new_stat[button_s_index][GUI_TOUCH_CTRL_DIRECT_DOWN] |= GUI_BUTTON_ACTIVE;
					}
					else {
						button_items_s_new_stat[button_s_index][GUI_TOUCH_CTRL_DIRECT_UP] |= GUI_BUTTON_ACTIVE;
					}
				}
			}
		}
		// |y| > |x|
		else {
			// set y-direction stat ACTIVE
			if (delta_y >= 0) {
				button_items_s_new_stat[button_s_index][GUI_TOUCH_CTRL_DIRECT_DOWN] |= GUI_BUTTON_ACTIVE;
			}
			else {
				button_items_s_new_stat[button_s_index][GUI_TOUCH_CTRL_DIRECT_UP] |= GUI_BUTTON_ACTIVE;
			}

			// set x-direction stat ACTIVE
			if (!single_direction) {
				if (abs_x > GUI_TOUCH_CTRL_S_COMBO_X_MIN) {
					if (delta_x >= 0) {
						button_items_s_new_stat[button_s_index][GUI_TOUCH_CTRL_DIRECT_RIGHT] |= GUI_BUTTON_ACTIVE;
					}
					else {
						button_items_s_new_stat[button_s_index][GUI_TOUCH_CTRL_DIRECT_LEFT] |= GUI_BUTTON_ACTIVE;
					}
				}
			}
		}
	}

	return ret;
}

static void send_pressed_s(int button_s_index, int direction) {
	keyStat[s_scancode_tbl[button_s_index][direction]] = SDL_PRESSED;
}

static void send_released_s(int button_s_index, int direction) {
	keyStat[s_scancode_tbl[button_s_index][direction]] = SDL_RELEASED;
}

// stick key emulator
static void GUI_t_ctrl_s_key_send() {
	for (int i = 0; i < BUTTON_ITEM_S_SIZE; i++) {
		int s_active_idx = -1;
		int s_down_idx   = -1;
		int s_up_idx     = -1;
		int s_focus_out_idx = BUTTON_ITEM_NONE; // init:-1, focus_on:-2, foucus_out:forcus_id

		bool single_direction;
		if (i == BUTTON_ITEM_LS) {
			single_direction = false;
		}
#ifndef GUI_TOUCH_CTRL_LS_ONLY	// enable RSB
		else if (i == BUTTON_ITEM_RS) {
			single_direction = true;
		}
#endif

		for (int te_i = 0; te_i < TOUCH_EVENT_BUF_SIZE; te_i++) {
			// check active buffer
			if ( (s_active_idx < 0) &&
				 (touch_event_buf[te_i].finger_id >= 0) &&
				 (touch_event_buf[te_i].stat != TOUCH_EVENT_STAT_NONE) ) {

				// in the region of circle area
				if (GUI_t_ctrl_set_stick_stat(i,
											  touch_event_buf[te_i].x,
											  touch_event_buf[te_i].y,
											  single_direction)) {
					// set focus_on
					if (s_focus_out_idx != BUTTON_ITEM_FOCUS_ON) { s_focus_out_idx = BUTTON_ITEM_FOCUS_ON; }

					// set event index (coming first is high priority)
					if (touch_event_buf[te_i].stat == SDL_FINGERMOTION) {
						// set foucus button_id
						if(touch_event_buf[te_i].focus_button_id == BUTTON_ITEM_NONE) {
							touch_event_buf[te_i].focus_button_id = (GUI_TOUCH_CTRL_ID_STICK | i);
						}

						if (s_active_idx < 0) {
							s_active_idx = te_i;
						}
						// multiple finger skip (clear stat)
						else {
							touch_event_buf[te_i].stat = TOUCH_EVENT_STAT_NONE;
						}
					}
					else if (touch_event_buf[te_i].stat == SDL_FINGERDOWN) {
						// set foucus button_id
						if(touch_event_buf[te_i].focus_button_id == BUTTON_ITEM_NONE) {
							touch_event_buf[te_i].focus_button_id = (GUI_TOUCH_CTRL_ID_STICK | i);
						}

						if (s_down_idx < 0) {
							s_down_idx = te_i;
						}
						// multiple finger skip (clear stat)
						else {
							touch_event_buf[te_i].stat = TOUCH_EVENT_STAT_NONE;
						}
					}
					else if (touch_event_buf[te_i].stat == SDL_FINGERUP) {
						if (s_up_idx < 0) {
							s_up_idx = te_i;
						}
						// multiple finger skip (clear stat)
						else {
							touch_event_buf[te_i].stat = TOUCH_EVENT_STAT_NONE;
						}
					}
				}
				else {
					// check focus out
					if(touch_event_buf[te_i].focus_button_id == (GUI_TOUCH_CTRL_ID_STICK | i)) { // current button item
						// check multiple touch
						if (s_focus_out_idx == BUTTON_ITEM_NONE) { // (s_focus_out_idx != BUTTON_ITEM_FOCUS_ON)
							s_focus_out_idx = touch_event_buf[te_i].focus_button_id;
						}
						touch_event_buf[te_i].focus_button_id = BUTTON_ITEM_NONE;
					}
                }
			}
			else if ( (s_active_idx >= 0) &&
					  (touch_event_buf[te_i].finger_id >= 0) &&
					  (touch_event_buf[te_i].stat != TOUCH_EVENT_STAT_NONE) ) {

				// multiple finger skip  (clear stat)
				touch_event_buf[te_i].stat = TOUCH_EVENT_STAT_NONE;
			}
		}

		if (s_active_idx >= 0) {
			// all-directions
			for (int di = 0; di < GUI_TOUCH_CTRL_DIRECT_END; di++) {
				if (button_items_s_new_stat[i][di] & GUI_BUTTON_ACTIVE) {
					button_items_s_stat[i][di] |= GUI_BUTTON_ACTIVE;
					//button_items_s_stat[i][di] |= GUI_BUTTON_CLICK;

					send_pressed_s(i, di);

					// set mark
					tex_info_s_mark[i].dst_rect.x = touch_event_buf[s_active_idx].x - (tex_info_s_mark[i].dst_rect_base.w / 2);
					tex_info_s_mark[i].dst_rect.y = touch_event_buf[s_active_idx].y - (tex_info_s_mark[i].dst_rect_base.h / 2);
				}
				// ON->OFF
				else if (button_items_s_stat[i][di] & GUI_BUTTON_ACTIVE) {
					button_items_s_stat[i][di] &= ~GUI_BUTTON_CLICK;
					button_items_s_stat[i][di] &= ~GUI_BUTTON_ACTIVE;

					send_released_s(i, di);
				}
			}

			// clear stat (completed event)
			touch_event_buf[s_active_idx].stat = TOUCH_EVENT_STAT_NONE;
		}

		if (s_down_idx >= 0) {
			// all-directions
			for (int di = 0; di < GUI_TOUCH_CTRL_DIRECT_END; di++) {
				if (button_items_s_new_stat[i][di] & GUI_BUTTON_ACTIVE) {
					button_items_s_stat[i][di] |= GUI_BUTTON_ACTIVE;
					button_items_s_stat[i][di] |= GUI_BUTTON_CLICK;

					send_pressed_s(i, di);

					// set mark
					tex_info_s_mark[i].dst_rect.x = touch_event_buf[s_down_idx].x - (tex_info_s_mark[i].dst_rect_base.w / 2);
					tex_info_s_mark[i].dst_rect.y = touch_event_buf[s_down_idx].y - (tex_info_s_mark[i].dst_rect_base.h / 2);
				}
			}

			// clear stat (completed event)
			touch_event_buf[s_down_idx].stat = TOUCH_EVENT_STAT_NONE;
		}

		if ((s_up_idx >= 0) && (s_down_idx < 0)) { // multiple finger priority: down > up
			// all-directions
			for (int di = 0; di < GUI_TOUCH_CTRL_DIRECT_END; di++) {
				if (button_items_s_stat[i][di] & GUI_BUTTON_ACTIVE) {
					button_items_s_stat[i][di] &= ~GUI_BUTTON_CLICK;
					button_items_s_stat[i][di] &= ~GUI_BUTTON_ACTIVE;

					send_released_s(i, di);

					// reset mark
					tex_info_s_mark[i].dst_rect.x = tex_info_s_mark[i].dst_rect_base.x;
					tex_info_s_mark[i].dst_rect.y = tex_info_s_mark[i].dst_rect_base.y;
				}
			}
		}

		// all finger focus out
		if (s_focus_out_idx >= 0) {
			for (int di = 0; di < GUI_TOUCH_CTRL_DIRECT_END; di++) {
				if (button_items_s_stat[i][di] & GUI_BUTTON_ACTIVE) {
					button_items_s_stat[i][di] &= ~GUI_BUTTON_ACTIVE;
					button_items_s_stat[i][di] &= ~GUI_BUTTON_CLICK;

					send_released_s(i, di);

					// reset mark
					tex_info_s_mark[i].dst_rect.x = tex_info_s_mark[i].dst_rect_base.x;
					tex_info_s_mark[i].dst_rect.y = tex_info_s_mark[i].dst_rect_base.y;
				}
			}
		}
	}
}

// init tex_info for touch_ctrl
static void GUI_t_ctrl_tex_info_init_rect(tex_info_t* tex_info, int src_w, int src_h, int dst_grid_x, int dst_grid_y) {
	int tmp_offset_x, tmp_offset_y;

	tex_info->src_rect = { 0, 0, src_w, src_h };

	// x-direction
	if (dst_grid_x < 0) {
		tmp_offset_x = g_win_current_w - (VIEW_TOUCH_CTRL_SCALE(64+10)) * (-dst_grid_x);
	} else {
		tmp_offset_x = (VIEW_TOUCH_CTRL_SCALE(64+10)) * dst_grid_x + VIEW_TOUCH_CTRL_SCALE(10);
	}

	// y-direction
	if (dst_grid_y < 0) {
		tmp_offset_y = g_win_current_h - (VIEW_TOUCH_CTRL_SCALE(64+10)) * (-dst_grid_y);
	} else {
		tmp_offset_y = (VIEW_TOUCH_CTRL_SCALE(64+10)) * dst_grid_y + VIEW_TOUCH_CTRL_SCALE(10);
	}

	tex_info->dst_rect = {tmp_offset_x, tmp_offset_y, VIEW_TOUCH_CTRL_SCALE(src_w), VIEW_TOUCH_CTRL_SCALE(src_h)};
	tex_info->dst_rect_base = { tex_info->dst_rect.x, tex_info->dst_rect.y, tex_info->dst_rect.w, tex_info->dst_rect.h };
}

// init draw items
static void tex_info_init()
{
	int ret = 0;
	int w, h;

	for (int i = 0; i < GUI_TOUCH_CTRL_ID_END; i++) {
		tex_info[tex_info_list[i].id].res_img = resource_manager_getTextureFromPath(
			tex_info_list[i].path, RESOURCE_MANAGER_TYPE_STATIC);
		ret = GUI_QueryTexture(tex_info[tex_info_list[i].id].res_img, NULL, NULL, &w, &h);
		if (ret == 0) {
			GUI_t_ctrl_tex_info_init_rect(&tex_info[tex_info_list[i].id], w, h, tex_info_list[i].grid_x, tex_info_list[i].grid_y);
			button_items[tex_info_list[i].button_id].tex_info_id = tex_info_list[i].id;
			button_items[tex_info_list[i].button_id].dst_rect = tex_info[tex_info_list[i].id].dst_rect;
			button_items[tex_info_list[i].button_id].dst_rect_base = tex_info[tex_info_list[i].id].dst_rect_base;
			button_items[tex_info_list[i].button_id].func = NULL; // nonuse
		}
	}

	// stick key
	for (int i = 0; i < GUI_TOUCH_CTRL_ID_S_END; i++) {
		tex_info_s[tex_info_s_list[i].id].res_img = resource_manager_getTextureFromPath(
			tex_info_s_list[i].path, RESOURCE_MANAGER_TYPE_STATIC);
		ret = GUI_QueryTexture(tex_info_s[tex_info_s_list[i].id].res_img, NULL, NULL, &w, &h);
		if (ret == 0) {
			GUI_t_ctrl_tex_info_init_rect(&tex_info_s[tex_info_s_list[i].id], w, h, tex_info_s_list[i].grid_x, tex_info_s_list[i].grid_y);
			button_items_s[tex_info_s_list[i].button_id].tex_info_id = tex_info_s_list[i].id;
			button_items_s[tex_info_s_list[i].button_id].dst_rect = tex_info_s[tex_info_s_list[i].id].dst_rect;
			button_items_s[tex_info_s_list[i].button_id].dst_rect_base = tex_info_s[tex_info_s_list[i].id].dst_rect_base;
			button_items_s[tex_info_s_list[i].button_id].func = NULL; // nonuse
		}
	}

	// stick mark
	for (int i = 0; i < GUI_TOUCH_CTRL_ID_S_END; i++) {
		tex_info_s_mark[i].res_img = resource_manager_getTextureFromPath(
			"{scale_mode:linear}images/gui/touch_ctrl/stick_mark.png", RESOURCE_MANAGER_TYPE_STATIC);
		ret = GUI_QueryTexture(tex_info_s_mark[i].res_img, NULL, NULL, &w, &h);
		if (ret == 0) {
			int mark_offset_x = tex_info_s[i].dst_rect.x + (tex_info_s[i].dst_rect.w / 2) - (VIEW_TOUCH_CTRL_SCALE(w) / 2);
			int mark_offset_y = tex_info_s[i].dst_rect.y + (tex_info_s[i].dst_rect.h / 2) - (VIEW_TOUCH_CTRL_SCALE(h) / 2);
			tex_info_s_mark[i].src_rect      = { 0, 0, w, h };
			tex_info_s_mark[i].dst_rect      = { mark_offset_x, mark_offset_y, VIEW_TOUCH_CTRL_SCALE(w), VIEW_TOUCH_CTRL_SCALE(h) };
			tex_info_s_mark[i].dst_rect_base = { mark_offset_x, mark_offset_y, VIEW_TOUCH_CTRL_SCALE(w), VIEW_TOUCH_CTRL_SCALE(h) };
		}
	}
}

void gui_touch_control_init() {
	view_touch_ctrl_scale = VIEW_TOUCH_CTRL_SCALE_DEFAULT;

	tex_info_init();

	button_current_time = SDL_GetTicks();
	for (int i=0; i < BUTTON_ITEM_SIZE; i++) {
		button_items[i].mouse_stat = 0;
		button_keyWait[i] = button_current_time;
		//button_items[i].func = handler_func;
	}

	// stick key
	for (int i=0; i < BUTTON_ITEM_S_SIZE; i++) {
		button_items_s[i].mouse_stat = 0;
		//button_items[i].func = handler_func;
		for (int di=0; di < GUI_TOUCH_CTRL_DIRECT_END; di++) {
			button_items_s_stat[i][di] = 0;
			button_items_s_new_stat[i][di] = 0;
		}
	}

	// touch event buffer
	for (int te_i = 0; te_i < TOUCH_EVENT_BUF_SIZE; te_i++) {
		touch_event_buf[te_i].finger_id = TOUCH_EVENT_FINGER_ID_NONE;
		touch_event_buf[te_i].x = -1;
		touch_event_buf[te_i].y = -1;
		touch_event_buf[te_i].stat = TOUCH_EVENT_STAT_NONE;
		touch_event_buf[te_i].focus_button_id = BUTTON_ITEM_NONE;
	}
}

// set touch_event_buf
void gui_touch_control_touch_event_set(SDL_Event* e) {
	int found_idx     = -1;
	int registerd_idx = -1;
	int empty_idx     = -1;

	// search same id or find empty buf
	for (int te_i = 0; te_i < TOUCH_EVENT_BUF_SIZE; te_i++) {
		// same id
		if (e->tfinger.fingerId == touch_event_buf[te_i].finger_id) {
			registerd_idx = te_i;
			break;
		}
		// empty buf
		else if ((empty_idx < 0) && (touch_event_buf[te_i].finger_id == TOUCH_EVENT_FINGER_ID_NONE)) {
			empty_idx = te_i;
		}
	}

	// error return
	if ((registerd_idx < 0) && (empty_idx < 0)) {
		LOG_ERROR("gui_touch_control_touch_event_set(): event buffer overflow.\n");
		return;
	}
	// set found index
	else {
		if (registerd_idx >= 0)  {
			found_idx = registerd_idx;
		}
		else {
			found_idx = empty_idx;
			touch_event_buf[found_idx].finger_id = e->tfinger.fingerId;
        }
	}

	// set touch event buffer
	if (e->type == SDL_FINGERMOTION) {
		touch_event_buf[found_idx].x = (int)(g_win_current_w * e->tfinger.x); // ratio -> screen position;
		touch_event_buf[found_idx].y = (int)(g_win_current_h * e->tfinger.y); // ratio -> screen position;
		touch_event_buf[found_idx].stat = SDL_FINGERMOTION;
	}
	else if (e->type == SDL_FINGERDOWN) {
		touch_event_buf[found_idx].x = (int)(g_win_current_w * e->tfinger.x); // ratio -> screen position;
		touch_event_buf[found_idx].y = (int)(g_win_current_h * e->tfinger.y); // ratio -> screen position;
		touch_event_buf[found_idx].stat = SDL_FINGERDOWN;
	}
	else if (e->type == SDL_FINGERUP) {
		touch_event_buf[found_idx].x = (int)(g_win_current_w * e->tfinger.x); // ratio -> screen position;
		touch_event_buf[found_idx].y = (int)(g_win_current_h * e->tfinger.y); // ratio -> screen position;
		touch_event_buf[found_idx].stat = SDL_FINGERUP;
	}
}

// key emulator
void gui_touch_control_key_send() {
	// clear touch_event_buf at end, if flag is true
	bool buf_clear_flg[TOUCH_EVENT_BUF_SIZE];
	for (int te_i = 0; te_i < TOUCH_EVENT_BUF_SIZE; te_i++) {
		buf_clear_flg[te_i] = (touch_event_buf[te_i].stat == SDL_FINGERUP) ? true : false;
	}

	// update timer
	button_current_time = SDL_GetTicks();

	// button key send
	GUI_t_ctrl_key_send();

	// stick key
	GUI_t_ctrl_s_key_send();

	//  clear event_buf
	for (int te_i = 0; te_i < TOUCH_EVENT_BUF_SIZE; te_i++) {
		if (buf_clear_flg[te_i]) {
			touch_event_buf[te_i].finger_id = TOUCH_EVENT_FINGER_ID_NONE;
			touch_event_buf[te_i].x = -1;
			touch_event_buf[te_i].y = -1;
			touch_event_buf[te_i].stat = TOUCH_EVENT_STAT_NONE;
			touch_event_buf[te_i].focus_button_id = BUTTON_ITEM_NONE;
		}
	}
}

void gui_touch_control_draw() {
	// draw button
	GUI_tex_info_draw(tex_info, GUI_TOUCH_CTRL_ID_END);
	GUI_tex_info_draw(tex_info_s, GUI_TOUCH_CTRL_ID_S_END);

	// draw selected
	SDL_SetRenderDrawColor(g_ren, 0, 0, 0, 32);
	for (int btn_i = 0; btn_i < BUTTON_ITEM_SIZE; btn_i++) {
		if (button_items[btn_i].mouse_stat & GUI_BUTTON_ACTIVE) {
			SDL_RenderFillRect(g_ren, &button_items[btn_i].dst_rect);
		}
	}

	// stick mark
	GUI_tex_info_draw(tex_info_s_mark, GUI_TOUCH_CTRL_ID_S_END);
}
