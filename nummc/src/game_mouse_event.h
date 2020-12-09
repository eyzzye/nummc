#pragma once
#include "game_common.h"

#define GAME_MOUSE_LEFT    (0)
#define GAME_MOUSE_MIDDLE  (1)
#define GAME_MOUSE_RIGHT   (2)
#define GAME_MOUSE_LR      (3)
#define GAME_MOUSE_BTN_NUM (4)

#define GAME_MOUSE_OFF  (0)
#define GAME_MOUSE_DOWN (1)
#define GAME_MOUSE_HOLD (2)
#define GAME_MOUSE_UP   (3)

#define GAME_MOUSE_CLICK   (0x00000001)
#define GAME_MOUSE_DCLICK  (0x00000002)
#define GAME_MOUSE_REPEAT  (0x00000004)
#define GAME_MOUSE_RELEASE (0x00000008)
#define GAME_MOUSE_DRAG    (0x00000010)
#define GAME_MOUSE_DROP    (0x00000020)

extern void game_mouse_event_init(int block_time = 0,
	int repeat_start_time = 400,
	int repeat_time = 200,
	int drag_time = 150,
	int drag_start_length = 5);

extern void game_mouse_event_reset();
extern void game_mouse_event_set(SDL_Event* e);

extern bool game_mouse_event_get_motion(int* x, int* y);
extern Uint32 game_mouse_event_get(int button_type);
extern void game_mouse_event_get_drag(int* x, int* y);
extern void game_mouse_event_get_repeat(int button_type, int* x, int* y);
