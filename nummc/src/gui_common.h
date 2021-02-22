#pragma once
#include "game_common.h"
#include "resource_manager.h"

// button click stat
#define GUI_BUTTON_ACTIVE 0x00000001
#define GUI_BUTTON_CLICK  0x00000002

#define GUI_SELECT_WAIT_TIMER 150

typedef struct _rect_region_t rect_region_t;
typedef struct _tex_info_t tex_info_t;
typedef struct _gui_item_t gui_item_t;

struct _rect_region_t {
	SDL_Rect dst_rect;
	SDL_Rect dst_rect_base; // size on SCREEN_WIDTH x SCREEN_HEIGHT
};

struct _tex_info_t {
	ResourceImg* res_img;
	SDL_Rect src_rect;
	SDL_Rect dst_rect;
	SDL_Rect dst_rect_base;	// size on SCREEN_WIDTH x SCREEN_HEIGHT
};

struct _gui_item_t {
	int tex_info_id;
	SDL_Rect dst_rect;
	SDL_Rect dst_rect_base; // size on SCREEN_WIDTH x SCREEN_HEIGHT
	Uint32 mouse_stat;
	void_func* func;
};

extern void GUI_RenderCopy(ResourceImg* res_img, const SDL_Rect* srcrect, const SDL_Rect* dstrect);
extern void GUI_RenderCopyEx(ResourceImg* res_img, const SDL_Rect* srcrect, const SDL_Rect* dstrect, const double angle, const SDL_Point* center, int flip);
extern int GUI_QueryTexture(ResourceImg* res_img, Uint32* format, int* access, int* w, int* h);

extern void GUI_rect_region_init_rect(rect_region_t* rect_region, int offset_x, int offset_y, int w, int h);
extern void GUI_rect_region_reset(rect_region_t* rect_region);

extern void GUI_tex_info_init_rect(tex_info_t* tex_info, int src_w, int src_h, int dst_offset_x, int dst_offset_y);
extern void GUI_tex_info_reset(tex_info_t* tex_info);
extern void GUI_tex_info_reset(tex_info_t* tex_info, int tex_info_size);
extern void GUI_tex_info_draw(tex_info_t* tex_info);
extern void GUI_tex_info_draw(tex_info_t* tex_info, int tex_info_size);

extern void GUI_gui_item_reset(gui_item_t* gui_item, int gui_item_size);
extern void GUI_gui_item_reset(gui_item_t* gui_item);
