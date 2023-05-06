#include <stdio.h>
#include "game_common.h"
#include "game_window.h"

#include "game_save.h"
#include "game_log.h"

#ifdef _ANDROID
#define GAME_WINDOW_FULLSCREEN
#endif

#define VIEW_SCALE_BASE_DEFAULT   2
#define VIEW_STAGE_SCALE_DEFAULT  2

std::mutex g_render_mtx;
SDL_Window* g_win = NULL;
int g_win_current_w = SCREEN_WIDTH;
int g_win_current_h = SCREEN_HEIGHT;
SDL_Renderer* g_ren = NULL;
int g_view_scale = VIEW_SCALE_BASE_DEFAULT;
int g_view_stage_scale = VIEW_STAGE_SCALE_DEFAULT;
SDL_Rect g_screen_size = { 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT };

#define DISPLAY_MODE_LIST_SIZE  (64)
static SDL_DisplayMode display_mode_list[DISPLAY_MODE_LIST_SIZE];
static int display_mode_list_size;

static void reset_screen_size(int w, int h) {
	int scale_w = 2 * w / SCREEN_WIDTH;
	int scale_h = 2 * h / SCREEN_HEIGHT;
	g_view_scale = scale_w > scale_h ? scale_h : scale_w;

	// reset screen size
	int padding_x = g_win_current_w - VIEW_SCALE(SCREEN_WIDTH);
	padding_x = padding_x < 0 ? 0 : (padding_x / 2);
	int padding_y = g_win_current_h - VIEW_SCALE(SCREEN_HEIGHT);
	padding_y = padding_y < 0 ? 0 : (padding_y / 2);
	g_screen_size = { padding_x, padding_y, VIEW_SCALE(SCREEN_WIDTH), VIEW_SCALE(SCREEN_HEIGHT) };
	LOG_DEBUG("reset_screen_size(): padding x=%d y=%d\n", padding_x, padding_y);
}

int game_window_create()
{
	g_view_stage_scale = VIEW_STAGE_SCALE_DEFAULT;

	int w = 0, h = 0;
	game_save_get_config_resolution(&w, &h);
	LOG_DEBUG("SDL_CreateWindow(): w=%d h=%d\n", w, h);

#ifdef GAME_WINDOW_FULLSCREEN
	g_win = SDL_CreateWindow("nummc", 100, 100, 0, 0, SDL_WINDOW_SHOWN | SDL_WINDOW_FULLSCREEN);
#else
	g_win = SDL_CreateWindow("nummc", 100, 100, w, h, SDL_WINDOW_SHOWN);
#endif
	if (g_win == NULL) {
		LOG_ERROR("SDL_CreateWindow Error: %s\n", SDL_GetError());
		return 1;
	}

#ifdef GAME_WINDOW_FULLSCREEN
	// set g_win_current_w, h
	SDL_DisplayMode current_mode;
	SDL_GetWindowDisplayMode(g_win, &current_mode);
	g_win_current_w = current_mode.w;
	g_win_current_h = current_mode.h;
	SDL_SetWindowSize(g_win, g_win_current_w, g_win_current_h);
	reset_screen_size(w, h);
	LOG_DEBUG("game_window_create() set g_win_current: w=%d h=%d\n", g_win_current_w, g_win_current_h);
#else
	// set g_win_current_w, h
	g_win_current_w = w;
	g_win_current_h = h;
	reset_screen_size(w, h);
#endif

	g_ren = SDL_CreateRenderer(g_win, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
	if (g_ren == NULL) {
		LOG_ERROR("SDL_CreateRenderer Error: %s\n", SDL_GetError());
		game_window_destory();
		return 1;
	}

	// blending mode
	SDL_SetRenderDrawBlendMode(g_ren, SDL_BLENDMODE_BLEND);

	return 0;
}

void game_window_destory()
{
	if (g_ren != NULL) {
		SDL_DestroyRenderer(g_ren);
		g_ren = NULL;
	}

	if (g_win != NULL) {
		SDL_DestroyWindow(g_win);
		g_win = NULL;
	}
}

int game_window_get_resolution()
{
	int displayIndex = SDL_GetWindowDisplayIndex(g_win);
	int displayNum = SDL_GetNumDisplayModes(displayIndex);

	memset(display_mode_list, 0, sizeof(display_mode_list));
	display_mode_list_size = displayNum;
	for (int i = 0; i < displayNum; i++) {
		if (i >= DISPLAY_MODE_LIST_SIZE) {
			display_mode_list_size = DISPLAY_MODE_LIST_SIZE;
			break;
		}
		SDL_GetDisplayMode(displayIndex, i, &display_mode_list[i]);
	}

	return 0;
}

int game_window_set_resolution(SDL_DisplayMode *mode, int width, int height)
{
	int ret = 0;

	SDL_DisplayMode current_mode;
	SDL_GetWindowDisplayMode(g_win, &current_mode);
	if ((current_mode.h == mode->h) && (current_mode.w == mode->w)) {
		LOG_DEBUG("game_window_set_resolution(): skip w=%d h=%d\n", current_mode.w, current_mode.h);
	}
	else {
		ret = SDL_SetWindowDisplayMode(g_win, mode);

#ifdef GAME_WINDOW_FULLSCREEN
		g_win_current_w = current_mode.w;
		g_win_current_h = current_mode.h;
		SDL_SetWindowSize(g_win, g_win_current_w, g_win_current_h);
#endif
	}

#ifndef GAME_WINDOW_FULLSCREEN // window mode
	g_win_current_w = width;
	g_win_current_h = height;
	SDL_SetWindowSize(g_win, width, height);
	LOG_DEBUG("game_window_set_resolution(): set w=%d h=%d\n", width, height);
#endif

	// reset scale
	reset_screen_size(width, height);

	return ret;
}

int game_window_set_resolution(int width, int height)
{
	int ret = 0;
	int mode_index = -1;
	int sub_mode_index = -1;

	game_window_get_resolution();
	for (int i = 0; i < display_mode_list_size; i++) {
		if ((display_mode_list[i].w == width) && (display_mode_list[i].h == height))
		{
			mode_index = i;
			break;
		}
		else if ((display_mode_list[i].w > width) && (display_mode_list[i].h > height)) {
			sub_mode_index = i;
		}
	}
	if (mode_index < 0) {
		if (sub_mode_index < 0) {
			return 1;
		}
		else {
			mode_index = sub_mode_index;
		}
	}

	ret = game_window_set_resolution(&display_mode_list[mode_index], width, height);
	return ret;
}
