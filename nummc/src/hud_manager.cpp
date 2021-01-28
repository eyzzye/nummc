#include "game_common.h"
#include "hud_manager.h"

#include "resource_manager.h"
#include "game_window.h"
#include "game_utils.h"
#include "game_log.h"
#include "gui_common.h"
#include "unit_manager.h"
#include "inventory_manager.h"
#include "stage_manager.h"

const int g_hud_offset_x = 0;
const int g_hud_offset_y = 144;

// start_x, start_y, end_x, end_y
#define HORIZONTAL_LINES_SIZE  11
static int horizontal_points[HORIZONTAL_LINES_SIZE][4] = {
		{  2,   2, 476,   2},

		// charge gage
		{282,   9, 286,   9},
		{282,  15, 286,  15},
		{282,  22, 286,  22},
		{282,  28, 286,  28},
		{282,  35, 286,  35},
		{282,  42, 286,  42},
		{282,  48, 286,  48},
		{282,  55, 286,  55},
		{282,  61, 286,  61},

		{  2,  68, 476,  68},
};

// start_x, start_y, end_x, end_y
#define VERTICAL_LINES_SIZE  6
static int vertical_points[VERTICAL_LINES_SIZE][4] = {
		{  2,   2,   2,  68},

		{158,   2, 158,  68},	// E region
		{222,   2, 222,  68},	// Q region
		{286,   2, 286,  68},	// space region
		{350,   2, 350,  68},	// HP region
	
	    {476,   2, 476,  68},
};

static SDL_Rect hud_background;
static SDL_Rect horizontal_lines[HORIZONTAL_LINES_SIZE];
static SDL_Rect vertical_lines[VERTICAL_LINES_SIZE];

#define HP_GAGE_BASE   5
#define HP_GAGE_SIZE  12
#define HP_HORIZONTAL_LINES_SIZE  (2 * HP_GAGE_SIZE)
#define HP_VERTICAL_LINES_SIZE    (3 * HP_GAGE_SIZE)
#define HP_VERTICAL_FILL_SIZE     (2 * HP_GAGE_SIZE)
static SDL_Rect hp_horizontal_lines[HP_HORIZONTAL_LINES_SIZE];
static SDL_Rect hp_vertical_lines[HP_VERTICAL_LINES_SIZE];
static SDL_Rect hp_vertical_fill[HP_VERTICAL_FILL_SIZE];

// draw data
#define HUD_LABEL_ID_E      0
#define HUD_LABEL_ID_E_BOM  1
#define HUD_LABEL_ID_E_X    2
#define HUD_LABEL_ID_Q      3
#define HUD_LABEL_ID_SPACE  4
#define HUD_LABEL_ID_HP     5
#define HUD_LABEL_ID_END    6

static tex_info_t tex_info[HUD_LABEL_ID_END];

#define HUD_ICON_ID_E_UPPER  0
#define HUD_ICON_ID_E_LOWER  1
#define HUD_ICON_ID_Q        2
#define HUD_ICON_ID_SPACE    3
#define HUD_ICON_ID_END      4

static tex_info_t tex_icon_info[HUD_ICON_ID_END];
static int current_stocker_version;

#define HUD_PROGRESS_SIZE   20
static tex_info_t tex_progress_info[HUD_PROGRESS_SIZE];

#define HUD_Q_VAL_START_INDEX  0	// horizontal_points[0][1]  = 2
#define HUD_Q_VAL_END_INDEX   10	// horizontal_points[10][1] = 68
rect_region_t q_val_rect;

#define HUD_P_STAT_ICON_ID_FIRE_UP    0
#define HUD_P_STAT_ICON_ID_FREEZE_UP  1
#define HUD_P_STAT_ICON_ID_END        2

static tex_info_t tex_p_stat_icon_info[HUD_P_STAT_ICON_ID_END];

// boss HP
#define HUD_E_BOSS_ID_HP      0
#define HUD_E_BOSS_ID_HP_MAX  1
#define HUD_E_BOSS_ID_END     2
#define HUD_E_BOSS_HP_BAR_WIDTH   256
#define HUD_E_BOSS_HP_BAR_HEIGHT  (32 - 4 * 2)
static rect_region_t boss_hp_val_rect[HUD_E_BOSS_ID_END];
static tex_info_t tex_e_boss_icon_info;
static int e_boss_hp_max;
static int e_boss_hp;

static void hud_tex_info_init_rect(tex_info_t* tex_info, int src_w, int src_h, int dst_offset_x, int dst_offset_y);
static void hud_tex_info_reset(tex_info_t* tex_info, int tex_info_size);
static void hud_tex_info_draw(tex_info_t* tex_info, int tex_info_size);
static void tex_info_init();

int hud_manager_init()
{
	tex_info_init();
	return 0;
}

void hud_manager_unload()
{
}

void hud_manager_reset()
{
	// frame
	int line_size = VIEW_STAGE(2);

	for (int i = 0; i < HORIZONTAL_LINES_SIZE; i++) {
		horizontal_lines[i] = {
			VIEW_STAGE_HUD_X(horizontal_points[i][0]),
			VIEW_STAGE_HUD_Y(horizontal_points[i][1]),
			VIEW_STAGE_HUD_X(horizontal_points[i][2]) - VIEW_STAGE_HUD_X(horizontal_points[i][0]) + line_size,
			VIEW_STAGE_HUD_Y(horizontal_points[i][3]) - VIEW_STAGE_HUD_Y(horizontal_points[i][1]) + line_size
		};
	}

	for (int i = 0; i < VERTICAL_LINES_SIZE; i++) {
		vertical_lines[i] = {
			VIEW_STAGE_HUD_X(vertical_points[i][0]),
			VIEW_STAGE_HUD_Y(vertical_points[i][1]),
			VIEW_STAGE_HUD_X(vertical_points[i][2]) - VIEW_STAGE_HUD_X(vertical_points[i][0]) + line_size,
			VIEW_STAGE_HUD_Y(vertical_points[i][3]) - VIEW_STAGE_HUD_Y(vertical_points[i][1]) + line_size
		};
	}

	// HP Gage
	int hp_line_width = VIEW_STAGE(8);
	int start_x = 360;
	int start_y = 30;
	for (int i = 0; i < HP_GAGE_SIZE; i++) {
		hp_horizontal_lines[2 * i] = {
			VIEW_STAGE_HUD_X(start_x), VIEW_STAGE_HUD_Y(start_y),
			hp_line_width + line_size, line_size
		};
		hp_horizontal_lines[2 * i + 1] = {
			VIEW_STAGE_HUD_X(start_x), VIEW_STAGE_HUD_Y(start_y + 12),
			hp_line_width + line_size, line_size
		};

		if (i != 5) {
			start_x += 14;
		}
		else {
			start_x = 360;
			start_y = 48;
		}
	}

	int hp_line_height = VIEW_STAGE(12);
	start_x = 360;
	start_y = 30;
	for (int i = 0; i < HP_GAGE_SIZE; i++) {
		hp_vertical_lines[3 * i] = {
			VIEW_STAGE_HUD_X(start_x), VIEW_STAGE_HUD_Y(start_y),
			line_size, hp_line_height + line_size
		};
		hp_vertical_lines[3 * i + 1] = {
			VIEW_STAGE_HUD_X(start_x + 5), VIEW_STAGE_HUD_Y(start_y),
			line_size, hp_line_height + line_size
		};
		hp_vertical_lines[3 * i + 2] = {
			VIEW_STAGE_HUD_X(start_x + 10), VIEW_STAGE_HUD_Y(start_y),
			line_size, hp_line_height + line_size
		};

		if (i != 5) {
			start_x += 14;
		}
		else {
			start_x = 360;
			start_y = 48;
		}
	}

	// HP Fill
	int hp_fill_width = VIEW_STAGE(3);
	int hp_fill_height = VIEW_STAGE(10);
	start_x = 362;
	start_y = 32;
	for (int i = 0; i < HP_GAGE_SIZE; i++) {
		hp_vertical_fill[2 * i] = {
			VIEW_STAGE_HUD_X(start_x), VIEW_STAGE_HUD_Y(start_y),
			hp_fill_width, hp_fill_height
		};
		hp_vertical_fill[2 * i + 1] = {
			VIEW_STAGE_HUD_X(start_x + 5), VIEW_STAGE_HUD_Y(start_y),
			hp_fill_width, hp_fill_height
		};

		if (i != 5) {
			start_x += 14;
		}
		else {
			start_x = 362;
			start_y = 50;
		}
	}

	// background
	hud_background = { VIEW_STAGE_HUD_X(2), VIEW_STAGE_HUD_Y(2), VIEW_STAGE(476 - 2 + 1), VIEW_STAGE(68 - 2 + 1) };

	// label
	hud_tex_info_reset(tex_info, HUD_LABEL_ID_END);
	hud_tex_info_reset(tex_icon_info, HUD_ICON_ID_END);
	hud_tex_info_reset(tex_progress_info, HUD_PROGRESS_SIZE);

	// player stat icon
	hud_tex_info_reset(tex_p_stat_icon_info, HUD_P_STAT_ICON_ID_END);

	// enemy boss stat icon
	SDL_Rect* tmp_rect = &tex_e_boss_icon_info.dst_rect_base;
	tex_e_boss_icon_info.dst_rect = VIEW_STAGE_RECT(tmp_rect->x, tmp_rect->y, tmp_rect->w, tmp_rect->h);
	tmp_rect = &boss_hp_val_rect[HUD_E_BOSS_ID_HP].dst_rect_base;
	boss_hp_val_rect[HUD_E_BOSS_ID_HP].dst_rect = VIEW_STAGE_RECT(tmp_rect->x, tmp_rect->y, tmp_rect->w, tmp_rect->h);
	tmp_rect = &boss_hp_val_rect[HUD_E_BOSS_ID_HP_MAX].dst_rect_base;
	boss_hp_val_rect[HUD_E_BOSS_ID_HP_MAX].dst_rect = VIEW_STAGE_RECT(tmp_rect->x, tmp_rect->y, tmp_rect->w, tmp_rect->h);

	// init hud info
	current_stocker_version = -1;
}

//
// tex_info
//
static void hud_tex_info_init_rect(tex_info_t* tex_info, int src_w, int src_h, int dst_offset_x, int dst_offset_y) {
	tex_info->src_rect = { 0, 0, src_w, src_h };
	tex_info->dst_rect = VIEW_STAGE_HUD_RECT(dst_offset_x, dst_offset_y, src_w, src_h);
	tex_info->dst_rect_base = { dst_offset_x, dst_offset_y, src_w, src_h };
}

static void hud_tex_info_reset(tex_info_t* tex_info, int tex_info_size) {
	SDL_Rect* tmp_rect;
	for (int i = 0; i < tex_info_size; i++) {
		tmp_rect = &tex_info[i].dst_rect_base;
		tex_info[i].dst_rect = VIEW_STAGE_HUD_RECT(tmp_rect->x, tmp_rect->y, tmp_rect->w, tmp_rect->h);
	}
}

static void hud_tex_info_draw(tex_info_t* tex_info, int tex_info_size) {
	for (int i = 0; i < tex_info_size; i++) {
		SDL_RenderCopy(g_ren, tex_info[i].tex, &tex_info[i].src_rect, &tex_info[i].dst_rect);
	}
}

static void hud_tex_info_reset_e_val(int number)
{
	int w, h, w_pos, h_pos;
	int number_upper = number / 10;
	int number_lower = number % 10;

	tex_icon_info[HUD_ICON_ID_E_UPPER].tex = game_utils_render_number_font_tex(number_upper);
	int ret = SDL_QueryTexture(tex_icon_info[HUD_ICON_ID_E_UPPER].tex, NULL, NULL, &w, &h);
	if (ret == 0) {
		w_pos = (158 + 2) + ((222 - (158 + 2)) / 2) + 2;
		h_pos = (4 + 24) + (68 - (4 + 24)) / 2 - (h / 4);
		tex_icon_info[HUD_ICON_ID_E_UPPER].src_rect = { 0, 0, w, h };
		tex_icon_info[HUD_ICON_ID_E_UPPER].dst_rect = VIEW_STAGE_HUD_RECT(w_pos, h_pos, w / 2, h / 2);
		tex_icon_info[HUD_ICON_ID_E_UPPER].dst_rect_base = { w_pos, h_pos, w / 2, h / 2 };
	}

	tex_icon_info[HUD_ICON_ID_E_LOWER].tex = game_utils_render_number_font_tex(number_lower);
	ret = SDL_QueryTexture(tex_icon_info[HUD_ICON_ID_E_LOWER].tex, NULL, NULL, &w, &h);
	if (ret == 0) {
		w_pos += 10;
		tex_icon_info[HUD_ICON_ID_E_LOWER].src_rect = { 0, 0, w, h };
		tex_icon_info[HUD_ICON_ID_E_LOWER].dst_rect = VIEW_STAGE_HUD_RECT(w_pos, h_pos, w / 2, h / 2);
		tex_icon_info[HUD_ICON_ID_E_LOWER].dst_rect_base = { w_pos, h_pos, w / 2, h / 2 };
	}
}

static void hud_tex_info_reset_q_val(unit_items_data_t* unit_item_data)
{
	int w = 32, h = 32;
	if (unit_item_data) {
		tex_icon_info[HUD_ICON_ID_Q].tex = unit_item_data->anim->anim_stat_base_list[ANIM_STAT_IDLE]->frame_list[0]->tex;
		int ret = SDL_QueryTexture(tex_icon_info[HUD_ICON_ID_Q].tex, NULL, NULL, &w, &h);
	}
	else {
		tex_icon_info[HUD_ICON_ID_Q].tex = NULL;
	}

	int w_pos = (222 + 2) + ((286 - (222 + 2)) / 2) - (w / 2);
	int h_pos = 4 + 24;
	hud_tex_info_init_rect(&tex_icon_info[HUD_ICON_ID_Q], w, h, w_pos, h_pos);

	// q val
	w = 286 - 222;
	int charge_val = inventory_manager_get_charge_val();
	if (charge_val > 0) {
		h = horizontal_points[HUD_Q_VAL_END_INDEX][1] - horizontal_points[HUD_Q_VAL_END_INDEX - charge_val][1];
		q_val_rect.dst_rect = VIEW_STAGE_HUD_RECT(222, horizontal_points[HUD_Q_VAL_END_INDEX - charge_val][1], w, h);
		q_val_rect.dst_rect_base = { 222, horizontal_points[HUD_Q_VAL_END_INDEX - charge_val][1], w, h };
	}
	else {
		h = 0;
		q_val_rect.dst_rect = VIEW_STAGE_HUD_RECT(222, horizontal_points[HUD_Q_VAL_END_INDEX][1], w, h);
		q_val_rect.dst_rect_base = { 222, horizontal_points[HUD_Q_VAL_END_INDEX][1], w, h };
	}
}

static void hud_tex_info_reset_space_val(unit_items_data_t* unit_item_data)
{
	int w = 32, h = 32;
	if (unit_item_data) {
		tex_icon_info[HUD_ICON_ID_SPACE].tex = unit_item_data->anim->anim_stat_base_list[ANIM_STAT_IDLE]->frame_list[0]->tex;
		int ret = SDL_QueryTexture(tex_icon_info[HUD_ICON_ID_SPACE].tex, NULL, NULL, &w, &h);
	}
	else {
		tex_icon_info[HUD_ICON_ID_SPACE].tex = NULL;
	}

	int w_pos = (286 + 2) + ((350 - (286 + 2)) / 2) - (w / 2);
	int h_pos = 4 + 24;
	hud_tex_info_init_rect(&tex_icon_info[HUD_ICON_ID_SPACE], w, h, w_pos, h_pos);
}

static void hud_tex_info_reset_progress(std::string stage_id)
{
	int w, h, w_pos, h_pos;

	// display [final]
	if (stage_id == "final") {
		tex_progress_info[0].tex = resource_manager_getTextureFromPath("images/gui/hud/final.png");
		int ret = SDL_QueryTexture(tex_progress_info[0].tex, NULL, NULL, &w, &h);
		if (ret == 0) {
			// progress center pos
			w_pos = (2) + (158 - 2) / 2  - w / 2;
			h_pos = (4) + ( 68 - 4) / 2  - h / 2;
			hud_tex_info_init_rect(&tex_progress_info[0], w, h, w_pos, h_pos);
		}

		// clear old tex
		for (int i = 1; i < 10; i++) {
			int index = i * 2 - 1;
			tex_progress_info[index].tex = NULL;
			index = i * 2;
			tex_progress_info[index].tex = NULL;
		}

		return;
	}

	// display stage [0 -> 1 -> ... -> 9]
	int stage_number = atoi(stage_id.c_str());
	if (stage_number > 9) stage_number = 9;

	tex_progress_info[0].tex = game_utils_render_number_font_tex(0);
	int ret = SDL_QueryTexture(tex_progress_info[0].tex, NULL, NULL, &w, &h);
	if (ret == 0) {
		w_pos = 2 + 8;
		h_pos = (4) + (68 - (4)) / 2 - h / 2;
		tex_progress_info[0].src_rect = { 0, 0, w, h };
		tex_progress_info[0].dst_rect = VIEW_STAGE_HUD_RECT(w_pos, h_pos, w / 2, h / 2);
		tex_progress_info[0].dst_rect_base = { w_pos, h_pos, w / 2, h / 2 };
	}

	for (int i = 1; i < stage_number + 1; i++) {
		if (i == 5) {
			w_pos = 2 + 8;
			h_pos += h / 2;
		}

		int index = i * 2 - 1;
		tex_progress_info[index].tex = resource_manager_getTextureFromPath("images/gui/font/arrow.png");
		ret = SDL_QueryTexture(tex_progress_info[index].tex, NULL, NULL, &w, &h);
		if (ret == 0) {
			w_pos += 12;
			tex_progress_info[index].src_rect = { 0, 0, w, h };
			tex_progress_info[index].dst_rect = VIEW_STAGE_HUD_RECT(w_pos, h_pos, w / 2, h / 2);
			tex_progress_info[index].dst_rect_base = { w_pos, h_pos, w / 2, h / 2 };
		}

		index = i * 2;
		tex_progress_info[index].tex = game_utils_render_number_font_tex(i);
		int ret = SDL_QueryTexture(tex_progress_info[index].tex, NULL, NULL, &w, &h);
		if (ret == 0) {
			w_pos += 12;
			tex_progress_info[index].src_rect = { 0, 0, w, h };
			tex_progress_info[index].dst_rect = VIEW_STAGE_HUD_RECT(w_pos, h_pos, w / 2, h / 2);
			tex_progress_info[index].dst_rect_base = { w_pos, h_pos, w / 2, h / 2 };
		}
	}

	// clear old tex
	for (int i = stage_number + 1; i < 10; i++) {
		int index = i * 2 - 1;
		tex_progress_info[index].tex = NULL;
		index = i * 2;
		tex_progress_info[index].tex = NULL;
	}
}

static void tex_info_init()
{
	int w, h;
	int w_pos = 0, h_pos = 0;

	// label
	tex_info[HUD_LABEL_ID_E].tex = resource_manager_getTextureFromPath("images/gui/hud/e_label.png");
	int ret = SDL_QueryTexture(tex_info[HUD_LABEL_ID_E].tex, NULL, NULL, &w, &h);
	if (ret == 0) {
		w_pos = 182;
		h_pos = 4;
		hud_tex_info_init_rect(&tex_info[HUD_LABEL_ID_E], w, h, w_pos, h_pos);
	}

	// E_BOM label (half scale icon)
	tex_info[HUD_LABEL_ID_E_BOM].tex = resource_manager_getTextureFromPath("images/units/items/stock/simple_bom/bom_idle1.png");
	ret = SDL_QueryTexture(tex_info[HUD_LABEL_ID_E_BOM].tex, NULL, NULL, &w, &h);
	if (ret == 0) {
		w_pos = (158 + 2) + 2;
		h_pos = (4 + 24) + (68 - (4 + 24)) / 2 - (h / 4);
		tex_info[HUD_LABEL_ID_E_BOM].src_rect = { 0, 0, w, h };
		tex_info[HUD_LABEL_ID_E_BOM].dst_rect = VIEW_STAGE_HUD_RECT(w_pos, h_pos, w / 2, h / 2);
		tex_info[HUD_LABEL_ID_E_BOM].dst_rect_base = { w_pos, h_pos, w / 2, h / 2 };
	}

	// E_X label
	tex_info[HUD_LABEL_ID_E_X].tex = resource_manager_getTextureFromPath("images/gui/hud/cross_label.png");
	ret = SDL_QueryTexture(tex_info[HUD_LABEL_ID_E_X].tex, NULL, NULL, &w, &h);
	if (ret == 0) {
		w_pos += 16;
		hud_tex_info_init_rect(&tex_info[HUD_LABEL_ID_E_X], w, h, w_pos, h_pos);
	}

	tex_info[HUD_LABEL_ID_Q].tex = resource_manager_getTextureFromPath("images/gui/hud/q_label.png");
	ret = SDL_QueryTexture(tex_info[HUD_LABEL_ID_Q].tex, NULL, NULL, &w, &h);
	if (ret == 0) {
		w_pos = 245;
		h_pos = 4;
		hud_tex_info_init_rect(&tex_info[HUD_LABEL_ID_Q], w, h, w_pos, h_pos);
	}

	tex_info[HUD_LABEL_ID_SPACE].tex = resource_manager_getTextureFromPath("images/gui/hud/space_label.png");
	ret = SDL_QueryTexture(tex_info[HUD_LABEL_ID_SPACE].tex, NULL, NULL, &w, &h);
	if (ret == 0) {
		w_pos = 288;
		h_pos = 4;
		hud_tex_info_init_rect(&tex_info[HUD_LABEL_ID_SPACE], w, h, w_pos, h_pos);
	}

	tex_info[HUD_LABEL_ID_HP].tex = resource_manager_getTextureFromPath("images/gui/hud/hp_label.png");
	ret = SDL_QueryTexture(tex_info[HUD_LABEL_ID_HP].tex, NULL, NULL, &w, &h);
	if (ret == 0) {
		w_pos = 356;
		h_pos = 4;
		hud_tex_info_init_rect(&tex_info[HUD_LABEL_ID_HP], w, h, w_pos, h_pos);

		// player stat icon pos
		w_pos += w + 2;
		h_pos += 4;
	}

	// player stat icon
	tex_p_stat_icon_info[HUD_P_STAT_ICON_ID_FIRE_UP].tex = resource_manager_getTextureFromPath("images/gui/hud/fire_up_reg.png");
	ret = SDL_QueryTexture(tex_p_stat_icon_info[HUD_P_STAT_ICON_ID_FIRE_UP].tex, NULL, NULL, &w, &h);
	if (ret == 0) {
		hud_tex_info_init_rect(&tex_p_stat_icon_info[HUD_P_STAT_ICON_ID_FIRE_UP], w, h, w_pos, h_pos);

		w_pos += w + 2;
		//h_pos = 4 + 4;
	}

	tex_p_stat_icon_info[HUD_P_STAT_ICON_ID_FREEZE_UP].tex = resource_manager_getTextureFromPath("images/gui/hud/freeze_up_reg.png");
	ret = SDL_QueryTexture(tex_p_stat_icon_info[HUD_P_STAT_ICON_ID_FREEZE_UP].tex, NULL, NULL, &w, &h);
	if (ret == 0) {
		hud_tex_info_init_rect(&tex_p_stat_icon_info[HUD_P_STAT_ICON_ID_FREEZE_UP], w, h, w_pos, h_pos);
	}

	// enemy boss stat
	tex_e_boss_icon_info.tex = resource_manager_getTextureFromPath("images/gui/hud/boss_icon.png");
	ret = SDL_QueryTexture(tex_e_boss_icon_info.tex, NULL, NULL, &w, &h);
	if (ret == 0) {
		w_pos = 224 - HUD_E_BOSS_HP_BAR_WIDTH / 2;
		h_pos = 0; // stage region top
		tex_e_boss_icon_info.src_rect = { 0, 0, w, h };
		tex_e_boss_icon_info.dst_rect = VIEW_STAGE_RECT(w_pos, h_pos, w, h);
		tex_e_boss_icon_info.dst_rect_base = { w_pos, h_pos, w, h };

		w_pos += w;
	}

	h_pos = 4; // stage region top (margin 4)
	boss_hp_val_rect[HUD_E_BOSS_ID_HP].dst_rect          = VIEW_STAGE_RECT( w_pos, h_pos, HUD_E_BOSS_HP_BAR_WIDTH, HUD_E_BOSS_HP_BAR_HEIGHT);
	boss_hp_val_rect[HUD_E_BOSS_ID_HP].dst_rect_base     = { w_pos, h_pos, HUD_E_BOSS_HP_BAR_WIDTH, HUD_E_BOSS_HP_BAR_HEIGHT };
	boss_hp_val_rect[HUD_E_BOSS_ID_HP_MAX].dst_rect      = VIEW_STAGE_RECT( w_pos, h_pos, HUD_E_BOSS_HP_BAR_WIDTH, HUD_E_BOSS_HP_BAR_HEIGHT);
	boss_hp_val_rect[HUD_E_BOSS_ID_HP_MAX].dst_rect_base = { w_pos, h_pos, HUD_E_BOSS_HP_BAR_WIDTH, HUD_E_BOSS_HP_BAR_HEIGHT };
	e_boss_hp_max = 0;
	e_boss_hp = HUD_E_BOSS_HP_BAR_WIDTH;

	// E icon
	hud_tex_info_reset_e_val(0);

	// Q icon
	hud_tex_info_reset_q_val(NULL);

	// SPACE icon
	hud_tex_info_reset_space_val(NULL);

	// progress label
	hud_tex_info_reset_progress(g_stage_data->id);
}

void hud_manager_update()
{
	if (current_stocker_version != g_stocker.version) {
		hud_tex_info_reset_e_val(g_stocker.weapon_item->item_count);
		hud_tex_info_reset_q_val(inventory_manager_get_charge_item());
		hud_tex_info_reset_space_val(inventory_manager_get_special_item());
		current_stocker_version = g_stocker.version;
	}
}

void hud_manager_update_e_boss_stat(int hp_max, int hp)
{
	int hp_val;
	if ((hp_max <= 0) || (hp <= 0)) { // reset
		e_boss_hp_max = 0;
		e_boss_hp = HUD_E_BOSS_HP_BAR_WIDTH;
		hp_val = HUD_E_BOSS_HP_BAR_WIDTH;
	}
	else {
		e_boss_hp_max = hp_max;
		e_boss_hp = hp;
		hp_val = HUD_E_BOSS_HP_BAR_WIDTH * hp / e_boss_hp_max;
	}

	boss_hp_val_rect[HUD_E_BOSS_ID_HP].dst_rect_base.w = hp_val;
	boss_hp_val_rect[HUD_E_BOSS_ID_HP].dst_rect.w = VIEW_STAGE(hp_val);
}

void hud_manager_display()
{
	// clear background
	SDL_SetRenderDrawColor(g_ren, 255, 255, 255, 255);
	SDL_RenderFillRect(g_ren, &hud_background);

	// charge val fill
	if (q_val_rect.dst_rect.h > 0) {
		SDL_SetRenderDrawColor(g_ren, 75, 196, 244, 255);
		SDL_RenderFillRect(g_ren, &q_val_rect.dst_rect);
	}

	SDL_SetRenderDrawColor(g_ren, 0, 0, 0, 255);
	SDL_RenderFillRects(g_ren, horizontal_lines, (int)LENGTH_OF(horizontal_lines));
	SDL_RenderFillRects(g_ren, vertical_lines, (int)LENGTH_OF(vertical_lines));

	// label
	hud_tex_info_draw(tex_info, HUD_LABEL_ID_END);

	// hp gage
	int hp_max = g_player.hp_max / (2* HP_GAGE_BASE);
	if (hp_max > HP_GAGE_SIZE) hp_max = HP_GAGE_SIZE;
	SDL_RenderFillRects(g_ren, hp_horizontal_lines, 2 * hp_max);
	SDL_RenderFillRects(g_ren, hp_vertical_lines, 3 * hp_max);

	// hp fill
	int hp_current = g_player.hp / HP_GAGE_BASE;
	if (hp_current > 2 * HP_GAGE_SIZE) hp_current = 2 * HP_GAGE_SIZE;
	SDL_SetRenderDrawColor(g_ren, 255, 0, 0, 255);
	SDL_RenderFillRects(g_ren, hp_vertical_fill, hp_current);

	// progress
	hud_tex_info_draw(tex_progress_info, HUD_PROGRESS_SIZE);

	// icon
	hud_tex_info_draw(tex_icon_info, HUD_ICON_ID_END);

	// player stat icon
	if (g_player.resistance_stat & UNIT_EFFECT_FLAG_P_FIRE_UP) {
		int i = HUD_P_STAT_ICON_ID_FIRE_UP;
		SDL_RenderCopy(g_ren, tex_p_stat_icon_info[i].tex, &tex_p_stat_icon_info[i].src_rect, &tex_p_stat_icon_info[i].dst_rect);
	}
	if (g_player.resistance_stat & UNIT_EFFECT_FLAG_P_FREEZE_UP) {
		int i = HUD_P_STAT_ICON_ID_FREEZE_UP;
		SDL_RenderCopy(g_ren, tex_p_stat_icon_info[i].tex, &tex_p_stat_icon_info[i].src_rect, &tex_p_stat_icon_info[i].dst_rect);
	}

	// enemy boss stat icon
	if (e_boss_hp_max > 0) {
		// boss icon
		SDL_RenderCopy(g_ren, tex_e_boss_icon_info.tex, &tex_e_boss_icon_info.src_rect, &tex_e_boss_icon_info.dst_rect);
		// hp max
		SDL_SetRenderDrawColor(g_ren, 255, 255, 255, 192);
		SDL_RenderFillRect(g_ren, &boss_hp_val_rect[HUD_E_BOSS_ID_HP_MAX].dst_rect);
		// hp
		SDL_SetRenderDrawColor(g_ren, 224, 0, 0, 255);
		SDL_RenderFillRect(g_ren, &boss_hp_val_rect[HUD_E_BOSS_ID_HP].dst_rect);
	}
}
