#include "game_common.h"
#include "game_key_event.h"

#include "game_timer.h"

std::vector<int> g_key_event_enable;

static Uint8 keyStat[SDL_NUM_SCANCODES];
static int keyWait[SDL_NUM_SCANCODES];

void game_key_event_init()
{
	g_key_event_enable.clear();
	for (int i = 0; i < SDL_NUM_SCANCODES; i++) {
		keyStat[i] = SDL_RELEASED;
		keyWait[i] = 0;
	}
}

void game_key_event_set(SDL_Event* e)
{
	const Uint8* keyList = SDL_GetKeyboardState(NULL);
	if (e->type == SDL_KEYDOWN) {
		for (int i = 0; i < g_key_event_enable.size(); i++) {
			if (keyList[g_key_event_enable[i]]) {
				keyStat[g_key_event_enable[i]] = SDL_PRESSED;
			}
		}
	}

	if (e->type == SDL_KEYUP) {
		for (int i = 0; i < g_key_event_enable.size(); i++) {
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
