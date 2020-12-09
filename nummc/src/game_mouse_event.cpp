#include <memory.h>
#include "game_common.h"
#include "game_mouse_event.h"

#include "game_timer.h"
#include "game_utils.h"

typedef struct _mouse_click_info mouse_click_info;
typedef struct _mouse_repeat_info mouse_repeat_info;
typedef struct _mouse_drag_info mouse_drag_info;

struct _mouse_click_info {
	int button;      // GAME_MOUSE_LEFT, MIDDLE, RIGHT, LR
	int sdl_stat;    // SDL_PRESSED or SDL_RELEASED
	int stat;        // GAME_MOUSE_OFF, DOWN, HOLD, UP
	int clicks;      // 1 for single-click, 2 for double-click
	int block_time;
	int x;
	int y;
};

struct _mouse_repeat_info {
	int button;
	int time;
	int x;
	int y;
};

struct _mouse_drag_info {
	int button;
	int time;
	int x;
	int y;
};

static int BLOCK_TIME;
static int REPEAT_START_TIME;
static int REPEAT_TIME;
static int DRAG_TIME;
static int DRAG_START_LENGTH;

static bool motion_dirt;
static int mouse_x;
static int mouse_y;
static mouse_click_info mouseClick[GAME_MOUSE_BTN_NUM];
static mouse_repeat_info mouseRepeat[GAME_MOUSE_BTN_NUM];
static mouse_drag_info mouseDrag;

void game_mouse_event_init(int block_time, int repeat_start_time, int repeat_time, int drag_time, int drag_start_length)
{
	BLOCK_TIME = block_time;
	REPEAT_START_TIME = repeat_start_time;
	REPEAT_TIME = repeat_time;
	DRAG_TIME = drag_time;
	DRAG_START_LENGTH = drag_start_length;

	motion_dirt = false;

	memset(mouseClick, 0, sizeof(mouse_click_info) * 4);
	mouseClick[GAME_MOUSE_LEFT].button   = GAME_MOUSE_LEFT;
	mouseClick[GAME_MOUSE_MIDDLE].button = GAME_MOUSE_MIDDLE;
	mouseClick[GAME_MOUSE_RIGHT].button  = GAME_MOUSE_RIGHT;
	mouseClick[GAME_MOUSE_LR].button     = GAME_MOUSE_LR;

	memset(mouseRepeat, 0, sizeof(mouse_repeat_info) * 4);
	mouseRepeat[GAME_MOUSE_LEFT].button   = GAME_MOUSE_LEFT;
	mouseRepeat[GAME_MOUSE_MIDDLE].button = GAME_MOUSE_MIDDLE;
	mouseRepeat[GAME_MOUSE_RIGHT].button  = GAME_MOUSE_RIGHT;
	mouseRepeat[GAME_MOUSE_LR].button     = GAME_MOUSE_LR;

	memset(&mouseDrag, 0, sizeof(mouse_drag_info));
}

void game_mouse_event_reset()
{
	// clear motion
	motion_dirt = false;

	// clear mouse up
	if (mouseClick[GAME_MOUSE_LEFT].stat   == GAME_MOUSE_UP) mouseClick[GAME_MOUSE_LEFT].stat   = GAME_MOUSE_OFF;
	if (mouseClick[GAME_MOUSE_MIDDLE].stat == GAME_MOUSE_UP) mouseClick[GAME_MOUSE_MIDDLE].stat = GAME_MOUSE_OFF;
	if (mouseClick[GAME_MOUSE_RIGHT].stat  == GAME_MOUSE_UP) mouseClick[GAME_MOUSE_RIGHT].stat  = GAME_MOUSE_OFF;
	if (mouseClick[GAME_MOUSE_LR].stat     == GAME_MOUSE_UP) mouseClick[GAME_MOUSE_LR].stat     = GAME_MOUSE_OFF;

	// set mouse down->hold
	if (mouseClick[GAME_MOUSE_LEFT].stat   == GAME_MOUSE_DOWN) mouseClick[GAME_MOUSE_LEFT].stat   = GAME_MOUSE_HOLD;
	if (mouseClick[GAME_MOUSE_MIDDLE].stat == GAME_MOUSE_DOWN) mouseClick[GAME_MOUSE_MIDDLE].stat = GAME_MOUSE_HOLD;
	if (mouseClick[GAME_MOUSE_RIGHT].stat  == GAME_MOUSE_DOWN) mouseClick[GAME_MOUSE_RIGHT].stat  = GAME_MOUSE_HOLD;
	if (mouseClick[GAME_MOUSE_LR].stat     == GAME_MOUSE_DOWN) mouseClick[GAME_MOUSE_LR].stat     = GAME_MOUSE_HOLD;
}

void game_mouse_event_set(SDL_Event* e)
{
	if (e->type == SDL_MOUSEMOTION) {
		SDL_GetMouseState(&mouse_x, &mouse_y);
		motion_dirt = true;
	}
	if (e->type == SDL_MOUSEBUTTONDOWN) {
		if (e->button.button == SDL_BUTTON_LEFT) {
			mouseClick[GAME_MOUSE_LEFT].sdl_stat = SDL_PRESSED;
			mouseClick[GAME_MOUSE_LEFT].stat = GAME_MOUSE_DOWN;
			mouseClick[GAME_MOUSE_LEFT].clicks = e->button.clicks & 0x01 ? 1 : 2;
		}
		if (e->button.button == SDL_BUTTON_MIDDLE) {
			mouseClick[GAME_MOUSE_MIDDLE].sdl_stat = SDL_PRESSED;
			mouseClick[GAME_MOUSE_MIDDLE].stat = GAME_MOUSE_DOWN;
			mouseClick[GAME_MOUSE_MIDDLE].clicks = e->button.clicks & 0x01 ? 1 : 2;
		}
		if (e->button.button == SDL_BUTTON_RIGHT) {
			mouseClick[GAME_MOUSE_RIGHT].sdl_stat = SDL_PRESSED;
			mouseClick[GAME_MOUSE_RIGHT].stat = GAME_MOUSE_DOWN;
			mouseClick[GAME_MOUSE_RIGHT].clicks = e->button.clicks & 0x01 ? 1 : 2;
		}

		// Left & Right
		if ((mouseClick[GAME_MOUSE_LEFT].sdl_stat == SDL_PRESSED) && (mouseClick[GAME_MOUSE_RIGHT].sdl_stat == SDL_PRESSED)) {
			mouseClick[GAME_MOUSE_LR].sdl_stat = SDL_PRESSED;
			mouseClick[GAME_MOUSE_LR].stat = GAME_MOUSE_DOWN;
			mouseClick[GAME_MOUSE_LR].clicks = 1; // always single-click
		}
	}
	if (e->type == SDL_MOUSEBUTTONUP) {
		if (e->button.button == SDL_BUTTON_LEFT) {
			mouseClick[GAME_MOUSE_LEFT].sdl_stat = SDL_RELEASED;
			mouseClick[GAME_MOUSE_LEFT].stat = GAME_MOUSE_UP;
		}
		if (e->button.button == SDL_BUTTON_MIDDLE) {
			mouseClick[GAME_MOUSE_MIDDLE].sdl_stat = SDL_RELEASED;
			mouseClick[GAME_MOUSE_MIDDLE].stat = GAME_MOUSE_UP;
		}
		if (e->button.button == SDL_BUTTON_RIGHT) {
			mouseClick[GAME_MOUSE_RIGHT].sdl_stat = SDL_RELEASED;
			mouseClick[GAME_MOUSE_RIGHT].stat = GAME_MOUSE_UP;
		}

		// Left & Right
		if (mouseClick[GAME_MOUSE_LR].stat == GAME_MOUSE_HOLD) {
			if ((mouseClick[GAME_MOUSE_LEFT].sdl_stat == SDL_RELEASED) || (mouseClick[GAME_MOUSE_RIGHT].sdl_stat == SDL_RELEASED)) {
				mouseClick[GAME_MOUSE_LR].sdl_stat = SDL_RELEASED;
				mouseClick[GAME_MOUSE_LR].stat = GAME_MOUSE_UP;
			}
		}
	}
}

bool game_mouse_event_get_motion(int *x, int *y)
{
	*x = mouse_x;
	*y = mouse_y;
	return motion_dirt;
}

Uint32 game_mouse_event_get(int button_type)
{
	Uint32 mouse_stat = 0;

	// mouse button
	if (mouseClick[button_type].block_time <= 0) {
		if (mouseClick[button_type].stat == GAME_MOUSE_DOWN) {
			mouse_stat |= GAME_MOUSE_CLICK;
			if (mouseClick[button_type].clicks == 2) mouse_stat |= GAME_MOUSE_DCLICK;
			mouseClick[button_type].block_time = BLOCK_TIME;
		}
	}
	else {
		mouseClick[button_type].block_time -= g_delta_time;

		if (mouseClick[button_type].block_time <= 0) {
			if (mouseClick[button_type].stat == GAME_MOUSE_HOLD) {
				mouseClick[button_type].block_time = BLOCK_TIME;
			}
			else {
				mouseClick[button_type].block_time = 0;
			}
		}
	}

	// mouse repeat
	if (mouseClick[button_type].stat == GAME_MOUSE_DOWN) {
		mouseRepeat[button_type].time = REPEAT_START_TIME;
		mouseRepeat[button_type].x = mouse_x;
		mouseRepeat[button_type].y = mouse_y;
	}
	else if (mouseClick[button_type].stat == GAME_MOUSE_HOLD) {
		mouseRepeat[button_type].time -= g_delta_time;

		if (mouseRepeat[button_type].time <= 0) {
			mouse_stat |= GAME_MOUSE_REPEAT;
			mouseRepeat[button_type].time = REPEAT_TIME;
		}
	}
	else if (mouseRepeat[button_type].time > 0) {
		// stat == MOUSE_UP or MOUSE_NONE
		mouseRepeat[button_type].time = 0;
	}

	// mouse drag
	if ((mouseClick[button_type].stat == GAME_MOUSE_HOLD) && (mouseDrag.button == button_type)) mouseDrag.time += g_delta_time;
	if (mouse_stat & GAME_MOUSE_CLICK) {
		mouseDrag.button = button_type;
		mouseDrag.time = 0;
		mouseDrag.x = mouse_x;
		mouseDrag.y = mouse_y;
	}
	if ((mouseClick[button_type].sdl_stat == SDL_RELEASED) && (mouseDrag.button == button_type) && (mouseDrag.time > 0)) {
		mouseDrag.time = 0;
		mouse_stat |= GAME_MOUSE_DROP;
	}
	if ((mouseDrag.button == button_type) && (mouseDrag.time > 0)) {
		if ((ABS(mouseDrag.x - mouse_x) + ABS(mouseDrag.y - mouse_y)) > DRAG_START_LENGTH) {
			mouse_stat |= GAME_MOUSE_DRAG;
		}
	}

	// mouse up
	if (mouseClick[button_type].stat == GAME_MOUSE_UP) {
		mouse_stat |= GAME_MOUSE_RELEASE;
	}

	return mouse_stat;
}

void game_mouse_event_get_drag(int* x, int* y)
{
	*x = mouseDrag.x;
	*y = mouseDrag.y;
}

void game_mouse_event_get_repeat(int button_type, int* x, int* y)
{
	*x = mouseRepeat[button_type].x;
	*y = mouseRepeat[button_type].y;
}
