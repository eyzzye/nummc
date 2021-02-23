#pragma once
#include <vector>
#include <mutex>
#include "game_common.h"
#include "hud_manager.h"

#define SCREEN_WIDTH   1280
#define SCREEN_HEIGHT   720

#define VIEW_SCALE_BASE 2
#define VIEW_SCALE(X) ((X) * g_view_scale / VIEW_SCALE_BASE)
#define VIEW_SCALE_X(_X) ((_X) * g_view_scale / VIEW_SCALE_BASE + g_screen_size.x)
#define VIEW_SCALE_Y(_Y) ((_Y) * g_view_scale / VIEW_SCALE_BASE + g_screen_size.y)
#define VIEW_SCALE_RECT(X_,Y_,W_,H_) \
         { (X_) * g_view_scale / VIEW_SCALE_BASE + g_screen_size.x, (Y_) * g_view_scale / VIEW_SCALE_BASE + g_screen_size.y, \
           (W_) * g_view_scale / VIEW_SCALE_BASE, (H_) * g_view_scale / VIEW_SCALE_BASE }

// view stage
#define VIEW_STAGE(X) ((X) * g_view_stage_scale * g_view_scale / VIEW_SCALE_BASE)
#define VIEW_STAGE_NORMAL(X) ((X) * VIEW_SCALE_BASE / (g_view_stage_scale * g_view_scale))
#define VIEW_STAGE_X(_X) ((_X) * g_view_stage_scale * g_view_scale / VIEW_SCALE_BASE + g_screen_size.x + VIEW_SCALE(g_hud_offset_x) - g_map_offset_x)
#define VIEW_STAGE_Y(_Y) ((_Y) * g_view_stage_scale * g_view_scale / VIEW_SCALE_BASE + g_screen_size.y + VIEW_SCALE(g_hud_offset_y) - g_map_offset_y)
#define VIEW_STAGE_RECT(X_,Y_,W_,H_) \
         { (X_) * g_view_stage_scale * g_view_scale / VIEW_SCALE_BASE + g_screen_size.x + VIEW_SCALE(g_hud_offset_x) - g_map_offset_x, \
           (Y_) * g_view_stage_scale * g_view_scale / VIEW_SCALE_BASE + g_screen_size.y + VIEW_SCALE(g_hud_offset_y) - g_map_offset_y, \
           (W_) * g_view_stage_scale * g_view_scale / VIEW_SCALE_BASE,                                                                 \
           (H_) * g_view_stage_scale * g_view_scale / VIEW_SCALE_BASE }

// view stage hud
#define VIEW_STAGE_HUD_X(_X) ((_X) * g_view_stage_scale * g_view_scale / VIEW_SCALE_BASE + g_screen_size.x)
#define VIEW_STAGE_HUD_Y(_Y) ((_Y) * g_view_stage_scale * g_view_scale / VIEW_SCALE_BASE + g_screen_size.y)
#define VIEW_STAGE_HUD_RECT(X_,Y_,W_,H_) \
         { (X_) * g_view_stage_scale * g_view_scale / VIEW_SCALE_BASE + g_screen_size.x, \
           (Y_) * g_view_stage_scale * g_view_scale / VIEW_SCALE_BASE + g_screen_size.y, \
           (W_) * g_view_stage_scale * g_view_scale / VIEW_SCALE_BASE,                   \
           (H_) * g_view_stage_scale * g_view_scale / VIEW_SCALE_BASE }

extern std::mutex g_render_mtx;
extern SDL_Window* g_win;
extern SDL_Renderer* g_ren;
extern int g_view_scale; // double size (1:640x480, 2:1280x720)
extern int g_view_stage_scale;
extern SDL_Rect g_screen_size;

extern int  game_window_create();
extern void game_window_destory();
extern int  game_window_get_resolution();
extern int  game_window_set_resolution(SDL_DisplayMode* mode);
extern int game_window_set_resolution(int width, int height);
