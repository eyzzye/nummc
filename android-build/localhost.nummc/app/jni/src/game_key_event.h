#pragma once
#include "game_common.h"

extern int g_key_event_enable[];
extern Uint8 keyStat[SDL_NUM_SCANCODES];
extern int keyWait[SDL_NUM_SCANCODES];

extern void game_key_event_init();
extern void game_key_event_set_key(int key_code);
extern void game_key_event_set(SDL_Event* e);
extern bool game_key_event_get(int key_code, int wait_timer = 0);
