#pragma once
#include "game_common.h"

// -- initialize --
// init()
// start()          : g_start_time, g_latest_time
//
// -- update elapsed time --
// update()         : g_elapsed_time = g_current_time - g_latest_time
//
// -- get delta time --
// get_delta_time() : g_delta_time = (g_elapsed_time - ONE_FRAME_TIME) < 0 ? (g_elapsed_time - ONE_FRAME_TIME) : ONE_FRAME_TIME
//                    g_elapsed_time -= g_delta_time
//
// -- pause timer --
// update()         : g_delta_time = g_current_time - g_latest_time
// pause(true)      : g_pause_time
// pause(false)     : g_current_time = g_latest_time - g_delta_time
// update()         : g_delta_time = g_current_time - g_latest_time
//

#define DELTA_TIME_MIN 24	// 40fps
#define ONE_FRAME_TIME 16

extern Uint32 g_start_time;
extern Uint32 g_current_time;
extern Uint32 g_latest_time;
extern Uint32 g_elapsed_time;
extern Uint32 g_delta_time;
extern Uint32 g_pause_time;

extern void   game_timer_init();
extern Uint32 game_timer_start();
extern Uint32 game_timer_pause(bool on);
extern Uint32 game_timer_update();
extern Uint32 game_timer_get_delta_time();
extern Uint32 game_timer_test();
extern Uint32 game_timer_fps();
