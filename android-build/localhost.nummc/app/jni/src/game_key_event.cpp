#include "game_common.h"
#include "game_key_event.h"
#include <memory.h>

#include "game_timer.h"

#define KEY_EVENT_ENABLE_SIZE_MAX  32
int g_key_event_enable[KEY_EVENT_ENABLE_SIZE_MAX];
static int g_key_event_enable_size;

Uint8 keyStat[SDL_NUM_SCANCODES];
int keyWait[SDL_NUM_SCANCODES];

void game_key_event_init()
{
	memset(g_key_event_enable, 0, sizeof(g_key_event_enable));
	g_key_event_enable_size = 0;

	for (int i = 0; i < SDL_NUM_SCANCODES; i++) {
		keyStat[i] = SDL_RELEASED;
		keyWait[i] = 0;
	}
}

void game_key_event_set_key(int key_code)
{
	g_key_event_enable[g_key_event_enable_size] = key_code;
	g_key_event_enable_size += 1;
}

void game_key_event_set(SDL_Event* e)
{
	const Uint8* keyList = SDL_GetKeyboardState(NULL);
	if (e->type == SDL_KEYDOWN) {
		for (int i = 0; i < g_key_event_enable_size; i++) {
			if (keyList[g_key_event_enable[i]]) {
				keyStat[g_key_event_enable[i]] = SDL_PRESSED;
			}
		}
	}

	if (e->type == SDL_KEYUP) {
		for (int i = 0; i < g_key_event_enable_size; i++) {
			if (!keyList[g_key_event_enable[i]]) {
				keyStat[g_key_event_enable[i]] = SDL_RELEASED;
			}
		}
	}
}

bool game_key_event_get(int key_code, int wait_timer)
{
	if (keyWait[key_code] <= 0) {
		if (keyStat[key_code]) {
			keyWait[key_code] = wait_timer;
			return true;
		}

	} else {
		keyWait[key_code] -= g_delta_time;
		if (keyWait[key_code] <= 0) {
			if (keyStat[key_code]) {
				// key repeat
				keyWait[key_code] = wait_timer;
				return true;
			}
			else {
				keyWait[key_code] = 0;
			}
		}
	}
	return false;
}
