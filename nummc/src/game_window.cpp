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
std::vector<SDL_DisplayMode> g_mode_list;

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

	g_mode_list.clear();
	g_mode_list.resize(displayNum);
	for (int i = 0; i < displayNum; i++) {
		SDL_GetDisplayMode(displayIndex, i, &g_mode_list[i]);
	}

	return 0;
}

int game_window_set_resolution(SDL_DisplayMode *mode)
{
	int ret = 0;

	// reset scale
	reset_screen_size(mode->w, mode->h);

	SDL_DisplayMode current_mode;
	SDL_GetWindowDisplayMode(g_win, &current_mode);
	if ((current_mode.h == mode->h) && (current_mode.w == mode->w)) {
		return 0;
	}

	ret = SDL_SetWindowDisplayMode(g_win, mode);
	SDL_SetWindowSize(g_win, mode->w, mode->h);

	return ret;
}

int game_window_set_resolution(int width, int height)
{
	int ret = 0;
	int mode_index = -1;

	game_window_get_resolution();
	for (int i = 0; i < g_mode_list.size(); i++) {
		if ((g_mode_list[i].w == width) && (g_mode_list[i].h == height))
		{
			mode_index = i;
			break;
		}
	}
	if (mode_index < 0) return 1;

	ret = game_window_set_resolution(&g_mode_list[mode_index]);
	return ret;
}
