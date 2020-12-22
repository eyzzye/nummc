#include "game_common.h"
#include "game_timer.h"

Uint32 g_start_time;
Uint32 g_current_time;
Uint32 g_latest_time;
Uint32 g_delta_time;
Uint32 g_pause_time;

static int current_fps;
static int frame_count;
static Uint32 elapse_time_for_fps;

static void game_timer_fps_init()
{
	current_fps = 0;
	frame_count = 0;
	elapse_time_for_fps = 0;
}

void game_timer_init()
{
	g_start_time = 0;
	g_current_time = 0;
	g_latest_time = 0;
	g_delta_time = 0;
	g_pause_time = 0;
}

Uint32 game_timer_start()
{
	g_start_time = SDL_GetTicks();
	g_latest_time = g_start_time;
	game_timer_fps_init();
	return g_start_time;
}

Uint32 game_timer_pause(bool on)
{
	if (on) {
		g_pause_time = SDL_GetTicks();
	}
	else {
		g_latest_time = SDL_GetTicks();
		g_current_time = MAX(0, g_latest_time - g_delta_time);
	}
	return 0;
}

Uint32 game_timer_update()
{
	g_current_time = SDL_GetTicks();
	g_delta_time = MIN(DELTA_TIME_MIN, MAX(0, g_current_time - g_latest_time));
	g_latest_time = g_current_time;
	return g_delta_time;
}

Uint32 game_timer_test()
{
	return MAX(0, SDL_GetTicks() - g_latest_time);
}

Uint32 game_timer_fps()
{
	elapse_time_for_fps += g_delta_time;
	frame_count++;
	if (elapse_time_for_fps > 1000) {
		current_fps = frame_count / (elapse_time_for_fps / 1000);
		if (current_fps > 10000) current_fps = 0;
		elapse_time_for_fps = 0;
		frame_count = 0;
	}
	return current_fps;
}
