#include <stdio.h>
#include "game_common.h"
#include "game_window.h"

#include "game_save.h"
#include "game_log.h"

#define VIEW_SCALE_BASE_DEFAULT   2
#define VIEW_STAGE_SCALE_DEFAULT  2

std::mutex g_render_mtx;
SDL_Window* g_win = NULL;
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
	int padding_x = w - VIEW_SCALE(SCREEN_WIDTH);
	padding_x = padding_x < 0 ? 0 : (padding_x / 2);
	int padding_y = h - VIEW_SCALE(SCREEN_HEIGHT);
	padding_y = padding_y < 0 ? 0 : (padding_y / 2);
	g_screen_size = { padding_x, padding_y, VIEW_SCALE(SCREEN_WIDTH), VIEW_SCALE(SCREEN_HEIGHT) };
}

int game_window_create()
{
	int w = 0, h = 0;
	game_save_get_config_resolution(&w, &h);
	reset_screen_size(w, h);
	g_view_stage_scale = VIEW_STAGE_SCALE_DEFAULT;

	g_win = SDL_CreateWindow("nummc", 100, 100, w, h, SDL_WINDOW_SHOWN);
	if (g_win == NULL) {
		LOG_ERROR("SDL_CreateWindow Error: %s\n", SDL_GetError());
		return 1;
	}

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

	// reset scale
	reset_screen_size(width, height);

	SDL_DisplayMode current_mode;
	SDL_GetWindowDisplayMode(g_win, &current_mode);
	if ((current_mode.h == mode->h) && (current_mode.w == mode->w)) {
		return 0;
	}
	else {
		ret = SDL_SetWindowDisplayMode(g_win, mode);
	}

	SDL_SetWindowSize(g_win, width, height);

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
