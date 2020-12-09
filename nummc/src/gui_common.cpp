#include "game_common.h"
#include "gui_common.h"
#include "game_window.h"
#include "game_log.h"

//
// rect_region
//
void GUI_rect_region_init_rect(rect_region_t* rect_region, int offset_x, int offset_y, int w, int h) {
	rect_region->dst_rect = VIEW_SCALE_RECT(offset_x, offset_y, w, h);
	rect_region->dst_rect_base = { offset_x, offset_y, w, h };
}

void GUI_rect_region_reset(rect_region_t* rect_region) {
	SDL_Rect* tmp_rect;
	tmp_rect = &rect_region->dst_rect_base;
	rect_region->dst_rect = VIEW_SCALE_RECT(tmp_rect->x, tmp_rect->y, tmp_rect->w, tmp_rect->h);
}

//
// tex_info
//
void GUI_tex_info_init_rect(tex_info_t* tex_info, int src_w, int src_h, int dst_offset_x, int dst_offset_y) {
	tex_info->src_rect = { 0, 0, src_w, src_h };
	tex_info->dst_rect = VIEW_SCALE_RECT(dst_offset_x, dst_offset_y, src_w, src_h);
	tex_info->dst_rect_base = { dst_offset_x, dst_offset_y, src_w, src_h };
}

void GUI_tex_info_reset(tex_info_t* tex_info) {
	SDL_Rect* tmp_rect;
	tmp_rect = &tex_info->dst_rect_base;
	tex_info->dst_rect = VIEW_SCALE_RECT(tmp_rect->x, tmp_rect->y, tmp_rect->w, tmp_rect->h);
}

void GUI_tex_info_reset(tex_info_t* tex_info, int tex_info_size) {
	SDL_Rect* tmp_rect;
	for (int i = 0; i < tex_info_size; i++) {
		tmp_rect = &tex_info[i].dst_rect_base;
		tex_info[i].dst_rect = VIEW_SCALE_RECT(tmp_rect->x, tmp_rect->y, tmp_rect->w, tmp_rect->h);
	}
}

void GUI_tex_info_draw(tex_info_t* tex_info) {
	SDL_RenderCopy(g_ren, tex_info->tex, &tex_info->src_rect, &tex_info->dst_rect);
}

void GUI_tex_info_draw(tex_info_t* tex_info, int tex_info_size) {
	for (int i = 0; i < tex_info_size; i++) {
		SDL_RenderCopy(g_ren, tex_info[i].tex, &tex_info[i].src_rect, &tex_info[i].dst_rect);
	}
}

//
// gui_item
//
void GUI_gui_item_reset(gui_item_t* gui_item, int gui_item_size) {
	SDL_Rect* tmp_rect;
	for (int i = 0; i < gui_item_size; i++) {
		tmp_rect = &gui_item[i].dst_rect_base;
		gui_item[i].dst_rect = VIEW_SCALE_RECT(tmp_rect->x, tmp_rect->y, tmp_rect->w, tmp_rect->h);
	}
}
void GUI_gui_item_reset(gui_item_t* gui_item) {
	SDL_Rect* tmp_rect;
	tmp_rect = &gui_item->dst_rect_base;
	gui_item->dst_rect = VIEW_SCALE_RECT(tmp_rect->x, tmp_rect->y, tmp_rect->w, tmp_rect->h);
}
