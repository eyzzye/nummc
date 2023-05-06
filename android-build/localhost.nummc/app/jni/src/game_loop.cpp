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
#include "gui_loading.h"

#ifdef _ANDROID
#include "gui_touch_control.h"
#endif

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
#ifdef _GAME_TIMER_FPS_ENABLE
	Uint32 fps;
#endif

	// common functions
	resource_manager_init();
	sound_manager_init();
	scene_manager_init();
	game_key_event_init();
	game_timer_init();
	game_timer_start();

	// show loading & init all scene
    gui_loading_init();
    scene_manager_start_loading();

	// common dialog
	dialog_message_init();

#ifdef _ANDROID
	// gui_touch_control
	gui_touch_control_init();
#endif

	// load TopMenu
	scene_manager_load(SCENE_ID_TOP_MENU);

	quit = false;
	while (!quit) {
#ifdef _GAME_TIMER_FPS_ENABLE
		fps = game_timer_fps();
		LOG_DEBUG("fps: %d\n", fps);
#endif
		dt = game_timer_update();
		//LOG_DEBUG("dt:  %d\n", dt);

		// key event
		scene_manager_pre_event();
		while (SDL_PollEvent(&e) != 0) {
			if (e.type == SDL_QUIT) {
				game_exit();
				quit = true;
			}
#ifdef _ANDROID
			else if ((e.type == SDL_FINGERDOWN) || (e.type == SDL_FINGERUP) || (e.type == SDL_FINGERMOTION)) {
				gui_touch_control_touch_event_set(&e);
			}
#endif
			else {
				scene_manager_key_event(&e);
			}
		}
		if (quit) continue;

#ifdef _ANDROID
		gui_touch_control_key_send();
#endif

		int frame_loop_num = 0;
		while(game_timer_get_delta_time() == ONE_FRAME_TIME) {
			if(frame_loop_num < 32) {
				// main process
				scene_manager_main_event();
			}
			//else {
			//	// over 32 frame, skip
			//}

			if (quit) continue;  // called game_loop_exit() in main_event()
			frame_loop_num += 1;
		}
		//if (frame_loop_num > 2) {
		//	LOG_DEBUG("frame_loop_num:  %d\n", frame_loop_num);
		//}

		// draw
		scene_manager_pre_draw();
		scene_manager_draw();
		scene_manager_after_draw();

		//game_utils_sleep(25); // for low-spec testing

		Uint32 next_dt = game_timer_test();
		if (DELTA_TIME_MIN > next_dt) {
			game_utils_sleep(DELTA_TIME_MIN - next_dt);
			//LOG_DEBUG("next_dt:  %d\n", next_dt);
		}
	}
}
