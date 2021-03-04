#include "game_common.h"
#include "game_loop.h"

#include "game_timer.h"
#include "game_key_event.h"
#include "scene_manager.h"
#include "resource_manager.h"
#include "sound_manager.h"
#include "unit_manager.h"
#include "dialog_message.h"
#include "game_log.h"
#include "game_utils.h"

static bool quit = false;

static void game_exit()
{
	sound_manager_close();
	scene_manager_unload_event();
	resource_manager_unload();
}

void game_loop_exit()
{
	game_exit();
	quit = true;
}

void game_loop_main()
{
	SDL_Event e;
	Uint32 dt;
	Uint32 fps;

	// common functions
	resource_manager_init();
	sound_manager_init();
	scene_manager_init();
	game_key_event_init();
	game_timer_init();
	game_timer_start();

	// common dialog
	dialog_message_init();

	// load TopMenu
	scene_manager_load(SCENE_ID_TOP_MENU, false);

	quit = false;
	while (!quit) {
		dt = game_timer_update();
		fps = game_timer_fps();
		//LOG_DEBUG("fps: %d\n", fps);
		//LOG_DEBUG("dt:  %d\n", dt);

		// key event
		scene_manager_pre_event();
		while (SDL_PollEvent(&e) != 0) {
			if (e.type == SDL_QUIT) {
				game_exit();
				quit = true;
			}
			else {
				scene_manager_key_event(&e);
			}
		}
		if (quit) continue;

		if (game_timer_get_delta_time() == ONE_FRAME_TIME) {
			// main process
			scene_manager_main_event();
			if (quit) continue;  // called game_loop_exit() in main_event()
		}

		// draw
		scene_manager_pre_draw();
		scene_manager_draw();
		scene_manager_after_draw();

		Uint32 next_dt = game_timer_test();
		if (ONE_FRAME_TIME > next_dt) {
			Sleep(ONE_FRAME_TIME - next_dt);
			//LOG_DEBUG("next_dt:  %d\n", next_dt);
		}

		//Sleep(25); // for low-spec testing
	}
}
