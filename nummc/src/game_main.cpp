#include <stdio.h>
#include <stdlib.h>
#include "game_common.h"

#include "game_window.h"
#include "game_loop.h"
#include "game_utils.h"
#include "game_save.h"
#include "game_log.h"

std::string g_base_path;

static int init_sdl2()
{
	if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) != 0) {
		LOG_ERROR_CONSOLE("SDL_Init Error: %s\n", SDL_GetError());
		return 1;
	}
	if ((IMG_Init(IMG_INIT_PNG) & IMG_INIT_PNG) != IMG_INIT_PNG) {
		LOG_ERROR_CONSOLE("IMG_Init Error: %s\n", SDL_GetError());
		SDL_Quit();
		return 1;
	}
	if (Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 2048) < 0) {
		LOG_ERROR_CONSOLE("Mix_OpenAudio Error: %s\n", SDL_GetError());
		IMG_Quit();
		SDL_Quit();
		return 1;
	}
	if (TTF_Init() != 0) {
		LOG_ERROR_CONSOLE("TTF_Init Error: %s\n", SDL_GetError());
		Mix_Quit();
		IMG_Quit();
		SDL_Quit();
		return 1;
	}
	return 0;
}

static void quit_sdl2()
{
	TTF_Quit();
	Mix_Quit();
	IMG_Quit();
	SDL_Quit();
}

int SDL_main(int argc, char** argv)
{
	int error = 0;
	if (init_sdl2() != 0) {
		return 1;
	}

	// execute path
	char* tmp_path = SDL_GetBasePath();
	if (tmp_path) {
		g_base_path = tmp_path;
		g_base_path = game_utils_replace_string(g_base_path, '\\', '/');
		LOG_DEBUG_CONSOLE("BasePath: %s\n", g_base_path.c_str());
		SDL_free(tmp_path);
	}
	else {
		LOG_ERROR_CONSOLE("GetBasePath Error: %s\n", SDL_GetError());
		quit_sdl2();
		return 1;
	}

	if (game_save_init() != 0) {
		quit_sdl2();
		return 1;
	}

	// open log file
	if (game_log_init() != 0) {
		game_save_close();
		quit_sdl2();
		return 1;
	}

	error = game_window_create();
	if (error) {
		quit_sdl2();
		return 1;
	}

	// main loop
	LOG_ERROR(GAME_VERSION);
	LOG_ERROR("ERROR START>>\n");
	LOG_DEBUG("LOOP START>>\n");
	game_loop_main();
	LOG_DEBUG("<<LOOP END\n");
	LOG_ERROR("<<ERROR END\n");

	game_window_destory();
	game_log_close();
	game_save_close();
	quit_sdl2();
	return 0;
}
