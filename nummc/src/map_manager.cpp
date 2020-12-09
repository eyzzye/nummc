#include <fstream>
#include <windows.h>      // for FindFirstFileA()
#include "game_common.h"
#include "map_manager.h"

#include "gui_common.h"
#include "game_timer.h"
#include "game_log.h"
#include "game_utils.h"
#include "resource_manager.h"
#include "unit_manager.h"
#include "game_window.h"

#ifdef _MAP_OFFSET_ENABLE_
int g_map_offset_x;
int g_map_offset_y;
#else
const int g_map_offset_x = 0;
const int g_map_offset_y = 0;
#endif
int g_map_x_max;    // block num
int g_map_y_max;    // block num
int g_tile_width;   // pixel
int g_tile_height;  // pixel

#define TILE_TAG_TILE       0
#define TILE_TAG_FRAME      1
#define TILE_TAG_IMG        2
#define TILE_TAG_COLLISION  3
#define TILE_TAG_END        4

#define MAP_TAG_MAP      0
#define MAP_TAG_TILESET  1
#define MAP_TAG_IMAGE    2
#define MAP_TAG_LAYER    3
#define MAP_TAG_DATA     4
#define MAP_TAG_END      5

static void load_tile_basic(std::string& line, tile_data_t* tile);
static void load_tile_frame(std::string& line, tile_data_t* tile);
static void load_tile_img(std::string& line, tile_data_t* tile);
static void load_tile_collision(std::string& line, tile_data_t* tile);

static void load_map(std::string& line);
static void load_tileset(std::string& line, int* tile_width, int* tile_height);
static void load_image(std::string& line, std::string& filename, int* width, int* height);
static void load_layer(std::string& line, std::string& name, int* width, int* height);
static void load_data(std::string& line);

static map_data_t map_data_list[MAP_TYPE_END];

static int layer_width;
static int layer_height;
static int tile_width;
static int tile_height;
static tile_instance_data_t* map_raw_data[MAP_TYPE_END];
static int read_tile_type;
static int read_tile_index;
static tile_instance_data_t map_wall[COLLISION_STATIC_WALL_NUM];

#define TILE_TEX_NUM  32
static tile_data_t tile_tex[TILE_TEX_NUM];
static std::string dir_path;


int map_manager_init()
{
#ifdef _MAP_OFFSET_ENABLE_
	g_map_offset_x = 0;
	g_map_offset_y = 0;
#endif
	g_map_x_max = 0;
	g_map_y_max = 0;
	g_tile_width = 0;
	g_tile_height = 0;

	for (int i = 0; i < MAP_TYPE_END; i++)
	{
		map_raw_data[i] = NULL;
	}
	memset(map_wall, 0, sizeof(map_wall));

	for (int i = 0; i < TILE_TEX_NUM; i++)
	{
		((tile_base_data_t*)&tile_tex[i])->type = UNIT_TYPE_TILE;
		((tile_base_data_t*)&tile_tex[i])->tile_type = TILE_TYPE_NONE;
		((tile_base_data_t*)&tile_tex[i])->col_shape = NULL;
		((tile_base_data_t*)&tile_tex[i])->anim = NULL;
	}
	return 0;
}

void map_manager_unload()
{
	for (int i = 0; i < MAP_TYPE_END; i++)
	{
		if (map_raw_data[i]) {
			if ((map_raw_data[i]->col_shape) && (map_raw_data[i]->col_shape->b2body)) {
				g_stage_world->DestroyBody(map_raw_data[i]->col_shape->b2body);
				map_raw_data[i]->col_shape->b2body = NULL;
			}

			delete [] map_raw_data[i];
			map_raw_data[i] = NULL;
		}
	}

	for (int i = 0; i < TILE_TEX_NUM; i++)
	{
		shape_data* tmp_tile = ((tile_base_data_t*)&tile_tex[i])->col_shape;
		if (tmp_tile) {
			delete tmp_tile;
			tmp_tile = NULL;
		}
	}
}

#ifdef _MAP_OFFSET_ENABLE_
void map_manager_set_offset(int x, int y) {
	if (x <= 0) {
		g_map_offset_x = 0;
	}
	else {
		int max_width = VIEW_STAGE(layer_width * tile_width);
		if (max_width > VIEW_SCALE(SCREEN_WIDTH)) max_width = max_width - VIEW_SCALE(SCREEN_WIDTH);
		g_map_offset_x = MIN(x, max_width);
	}

	if (y <= 0) {
		g_map_offset_y = 0;
	}
	else {
		int max_height = VIEW_STAGE(layer_height * tile_height);
		if (max_height > VIEW_SCALE(SCREEN_HEIGHT)) max_height = max_height - VIEW_SCALE(SCREEN_HEIGHT);
		g_map_offset_y = MIN(y, max_height);
	}
}
#endif

void map_manager_update() {
	for (int i = 0; i < TILE_TEX_NUM; i++)
	{
		tile_base_data_t* tile_base_data = (tile_base_data_t*)&tile_tex[i];
		if ((tile_base_data->tile_type == TILE_TYPE_BASE) && (tile_base_data->anim->anim_stat_base_list[ANIM_STAT_IDLE]->type == ANIM_TYPE_DYNAMIC)) {
			// set current_time
			tile_base_data->anim->anim_stat_list[ANIM_STAT_IDLE]->current_time += g_delta_time;

			int new_time = tile_base_data->anim->anim_stat_list[ANIM_STAT_IDLE]->current_time;
			int total_time = tile_base_data->anim->anim_stat_base_list[ANIM_STAT_IDLE]->total_time;
			if (new_time > total_time) {
				new_time = new_time % total_time;
				tile_base_data->anim->anim_stat_list[ANIM_STAT_IDLE]->current_time = new_time;
			}

			// set current_frame
			int sum_frame_time = 0;
			int frame_size = tile_base_data->anim->anim_stat_base_list[ANIM_STAT_IDLE]->frame_size;
			for (int i = 0; i < frame_size; i++) {
				sum_frame_time += tile_base_data->anim->anim_stat_base_list[ANIM_STAT_IDLE]->frame_list[i]->frame_time;
				if (new_time < sum_frame_time) {
					tile_base_data->anim->anim_stat_list[ANIM_STAT_IDLE]->current_frame = i;
					break;
				}
			}
		}
	}
}

void map_manager_display(int layer) {
	if (map_raw_data[layer] == NULL) return;

#ifdef _MAP_OFFSET_ENABLE_
	int offset_y = VIEW_STAGE_NORMAL(g_map_offset_y);
	int h_index_start = offset_y / tile_height;
	int h_index_end = (offset_y + SCREEN_HEIGHT) / tile_height;
	if ((offset_y + SCREEN_HEIGHT) % tile_height) h_index_end += 1;
	if (h_index_end > g_map_y_max) h_index_end = g_map_y_max;

	int offset_x = VIEW_STAGE_NORMAL(g_map_offset_x);
	int w_index_start = offset_x / tile_width;
	int w_index_end = (offset_x + SCREEN_WIDTH) / tile_width;
	if ((offset_x + SCREEN_WIDTH) % tile_width) w_index_end += 1;
	if (w_index_end > g_map_x_max) w_index_end = g_map_x_max;
#else
	int h_index_start = 0;
	int h_index_end = SCREEN_HEIGHT / tile_height;
	if (SCREEN_HEIGHT % tile_height) h_index_end += 1;
	if (h_index_end > g_map_y_max) h_index_end = g_map_y_max;

	int w_index_start = 0;
	int w_index_end = SCREEN_WIDTH / tile_width;
	if (SCREEN_WIDTH % tile_width) w_index_end += 1;
	if (w_index_end > g_map_x_max) w_index_end = g_map_x_max;
#endif

	SDL_Rect* src_rect;
	SDL_Rect dst_rect;
	for (int h = h_index_start; h < h_index_end; h++) {
		int map_index = h * layer_width + w_index_start;
		for (int w = w_index_start; w < w_index_end; w++) {
			// FIELD, BLOCK, EFFECT
			tile_instance_data_t* tile_data = (map_raw_data[layer] + map_index);
			int tile = tile_data->id;
			if (tile) {
				anim_frame_data_t* frame_data = NULL;
				tile_base_data_t* tile_base_data = (tile_base_data_t*)&tile_tex[tile];
				if (tile_base_data->anim->anim_stat_base_list[ANIM_STAT_IDLE]->type == ANIM_TYPE_STATIC) {
					frame_data = tile_base_data->anim->anim_stat_base_list[ANIM_STAT_IDLE]->frame_list[0];
				}
				else if (tile_base_data->anim->anim_stat_base_list[ANIM_STAT_IDLE]->type == ANIM_TYPE_DYNAMIC) {
					int frame_num = tile_base_data->anim->anim_stat_list[ANIM_STAT_IDLE]->current_frame;
					frame_data = tile_base_data->anim->anim_stat_base_list[ANIM_STAT_IDLE]->frame_list[frame_num];
				}

				if (frame_data) {
					SDL_Texture* tex = frame_data->tex;
					src_rect = &frame_data->src_rect;
					dst_rect = VIEW_STAGE_RECT(w * tile_width, h * tile_height, src_rect->w, src_rect->h);
					SDL_RenderCopy(g_ren, tex, src_rect, &dst_rect);
				}
			}
			map_index++;
		}
	}
}

shape_data* map_manager_get_col_shape(int x, int y) {
	shape_data* col_shape = NULL;
	int h_index = y / tile_height;
	int w_index = x / tile_width;
	int map_index = h_index * layer_width + w_index;
	col_shape = (map_raw_data[MAP_TYPE_BLOCK] + map_index)->col_shape;
	return col_shape;
}

void map_manager_create_instance() {
	if ((map_raw_data[MAP_TYPE_FIELD] == NULL) || (map_raw_data[MAP_TYPE_BLOCK] == NULL)) return;

	int map_index = 0;
	SDL_Rect* src_rect;
	SDL_Rect dst_rect;
	for (int h = 0; h < layer_height; h++) {
		for (int w = 0; w < layer_width; w++) {
			// FIELD
			tile_instance_data_t* tile_inst = map_raw_data[MAP_TYPE_FIELD] + map_index;
			int tile = tile_inst->id;
			tile_inst->type = UNIT_TYPE_TILE;
			tile_inst->tile_type = TILE_TYPE_INSTANCE;
			if (tile) {
				// basic info
				tile_inst->breakable = tile_tex[tile].breakable;

				anim_frame_data_t* frame_data = ((tile_base_data_t*)&tile_tex[tile])->anim->anim_stat_base_list[ANIM_STAT_IDLE]->frame_list[0];
				src_rect = &frame_data->src_rect;
				dst_rect = { w * src_rect->w, h * src_rect->w, src_rect->w, src_rect->h };
				tile_inst->obj = (void*)&tile_tex[tile];

				// create collision
				shape_data* base_shape = ((tile_base_data_t*)&tile_tex[tile])->col_shape;
				if (base_shape) {
					tile_inst->col_shape = collision_manager_create_static_shape(
						base_shape, tile_inst, g_tile_width, g_tile_height,
						&dst_rect.x, &dst_rect.y);
				}
				else {
					tile_inst->col_shape = NULL;
				}
			}
			else {
				tile_inst->obj = NULL;
				tile_inst->col_shape = NULL;
			}

			// BLOCK
			tile_inst = map_raw_data[MAP_TYPE_BLOCK] + map_index;
			tile = tile_inst->id;
			tile_inst->type = UNIT_TYPE_TILE;
			tile_inst->tile_type = TILE_TYPE_INSTANCE;
			if (tile) {
				// basic info
				tile_inst->breakable = tile_tex[tile].breakable;

				anim_frame_data_t* frame_data = ((tile_base_data_t*)&tile_tex[tile])->anim->anim_stat_base_list[ANIM_STAT_IDLE]->frame_list[0];
				src_rect = &frame_data->src_rect;
				dst_rect = { w * src_rect->w, h * src_rect->w, src_rect->w, src_rect->h };
				tile_inst->obj = (void*)&tile_tex[tile];

				shape_data* base_shape = ((tile_base_data_t*)&tile_tex[tile])->col_shape;
				if (base_shape) {
					tile_inst->col_shape = collision_manager_create_static_shape(
						base_shape, tile_inst, g_tile_width, g_tile_height,
						&dst_rect.x, &dst_rect.y);
				}
				else {
					tile_inst->col_shape = NULL;
				}
			}
			else {
				tile_inst->obj = NULL;
				tile_inst->col_shape = NULL;
			}

			map_index++;
		}
	}
}

void map_manager_create_wall() {
	int map_index = 0;
	int connected_region = 0;

	// top
	bool top_found = false;
	int left_pos = layer_width * layer_height;
	int right_pos = 0;
	int top_pos = layer_width * layer_height;
	int bottom_pos = 0;
	for (int h = 0; h < layer_height; h++) {
		for (int w = 0; w < layer_width; w++) {
			tile_instance_data_t* tile_inst = map_raw_data[MAP_TYPE_BLOCK] + map_index;
			int tile = tile_inst->id;
			if (tile && tile_inst->col_shape && tile_inst->col_shape->b2body) {
				if (map_index < left_pos) left_pos = map_index;
				if (right_pos < map_index) right_pos = map_index;
				if (map_index < top_pos) top_pos = map_index;

				connected_region += 1;
			}
			else {
				if (connected_region >= 3) {
					top_found = true;
					break;
				}
			}
			map_index++;
		}

		if (top_found) {
			break;
		}
		else {
			if (connected_region >= 3) {
				top_found = true;
				break;
			}
		}
	}

	if (connected_region >= 3) {
		tile_instance_data_t* tile_inst1 = map_raw_data[MAP_TYPE_BLOCK] + top_pos;
		tile_instance_data_t* tile_inst2 = map_raw_data[MAP_TYPE_BLOCK] + top_pos + (connected_region - 1);
		map_wall[COLLISION_STATIC_WALL_TOP].type = UNIT_TYPE_TILE;
		map_wall[COLLISION_STATIC_WALL_TOP].tile_type = TILE_TYPE_INSTANCE;
		map_wall[COLLISION_STATIC_WALL_TOP].col_shape = NULL; // dummy
		collision_manager_create_static_wall(COLLISION_STATIC_WALL_TOP, &map_wall[COLLISION_STATIC_WALL_TOP], tile_inst1->col_shape->b2body, tile_inst2->col_shape->b2body);

		for (map_index = top_pos + 1; map_index < top_pos + (connected_region - 1); map_index++) {
			tile_instance_data_t* tile_inst = map_raw_data[MAP_TYPE_BLOCK] + map_index;
			if (tile_inst->col_shape->b2body) {
				g_stage_world->DestroyBody(tile_inst->col_shape->b2body);
				tile_inst->col_shape->b2body = NULL;
			}
		}
	}

	// left
	bool left_found = false;
	connected_region = 0;
	int left_offset = left_pos % layer_width;
	for (int h = 0; h < layer_height; h++) {
		map_index = layer_width * h + left_offset;
		tile_instance_data_t* tile_inst = map_raw_data[MAP_TYPE_BLOCK] + map_index;

		int tile = tile_inst->id;
		if (tile && tile_inst->col_shape && tile_inst->col_shape->b2body) {
			if (bottom_pos < map_index) bottom_pos = map_index;

			connected_region += 1;
		}
		else {
			if (connected_region >= 3) {
				left_found = true;
				break;
			}
		}
	}

	if (connected_region >= 3) {
		tile_instance_data_t* tile_inst1 = map_raw_data[MAP_TYPE_BLOCK] + left_pos;
		tile_instance_data_t* tile_inst2 = map_raw_data[MAP_TYPE_BLOCK] + bottom_pos;
		map_wall[COLLISION_STATIC_WALL_LEFT].type = UNIT_TYPE_TILE;
		map_wall[COLLISION_STATIC_WALL_LEFT].tile_type = TILE_TYPE_INSTANCE;
		map_wall[COLLISION_STATIC_WALL_LEFT].col_shape = NULL; // dummy
		collision_manager_create_static_wall(COLLISION_STATIC_WALL_LEFT, &map_wall[COLLISION_STATIC_WALL_LEFT], tile_inst1->col_shape->b2body, tile_inst2->col_shape->b2body);

		for (int h = (left_pos / layer_width) + 1; h < (bottom_pos / layer_width); h++) {
			map_index = layer_width * h + left_offset;
			tile_instance_data_t* tile_inst = map_raw_data[MAP_TYPE_BLOCK] + map_index;
			if (tile_inst->col_shape->b2body) {
				g_stage_world->DestroyBody(tile_inst->col_shape->b2body);
				tile_inst->col_shape->b2body = NULL;
			}
		}
	}

	// right
	bool right_found = false;
	connected_region = 0;
	int right_offset = right_pos % layer_width;
	for (int h = 0; h < layer_height; h++) {
		map_index = layer_width * h + right_offset;
		tile_instance_data_t* tile_inst = map_raw_data[MAP_TYPE_BLOCK] + map_index;

		int tile = tile_inst->id;
		if (tile && tile_inst->col_shape && tile_inst->col_shape->b2body) {
			connected_region += 1;
		}
		else {
			if (connected_region >= 3) {
				right_found = true;
				break;
			}
		}
	}

	if (connected_region >= 3) {
		tile_instance_data_t* tile_inst1 = map_raw_data[MAP_TYPE_BLOCK] + right_pos;
		tile_instance_data_t* tile_inst2 = map_raw_data[MAP_TYPE_BLOCK] + right_pos + (connected_region - 1) * layer_width;
		map_wall[COLLISION_STATIC_WALL_RIGHT].type = UNIT_TYPE_TILE;
		map_wall[COLLISION_STATIC_WALL_RIGHT].tile_type = TILE_TYPE_INSTANCE;
		map_wall[COLLISION_STATIC_WALL_RIGHT].col_shape = NULL; // dummy
		collision_manager_create_static_wall(COLLISION_STATIC_WALL_RIGHT, &map_wall[COLLISION_STATIC_WALL_RIGHT], tile_inst1->col_shape->b2body, tile_inst2->col_shape->b2body);

		for (int h = (right_pos / layer_width) + 1; h < (right_pos / layer_width) + connected_region - 1; h++) {
			map_index = layer_width * h + right_offset;
			tile_instance_data_t* tile_inst = map_raw_data[MAP_TYPE_BLOCK] + map_index;
			if (tile_inst->col_shape->b2body) {
				g_stage_world->DestroyBody(tile_inst->col_shape->b2body);
				tile_inst->col_shape->b2body = NULL;
			}
		}
	}

	// bottom
	bool bottom_found = false;
	connected_region = 0;
	map_index = bottom_pos;
	for (int w = 0; w < layer_width; w++) {
		tile_instance_data_t* tile_inst = map_raw_data[MAP_TYPE_BLOCK] + map_index;

		int tile = tile_inst->id;
		if (tile && tile_inst->col_shape && tile_inst->col_shape->b2body) {
			connected_region += 1;
		}
		else {
			if (connected_region >= 3) {
				bottom_found = true;
				break;
			}
		}
		map_index++;
	}

	if (connected_region >= 3) {
		tile_instance_data_t* tile_inst1 = map_raw_data[MAP_TYPE_BLOCK] + bottom_pos;
		tile_instance_data_t* tile_inst2 = map_raw_data[MAP_TYPE_BLOCK] + bottom_pos + (connected_region - 1);
		map_wall[COLLISION_STATIC_WALL_BOTTOM].type = UNIT_TYPE_TILE;
		map_wall[COLLISION_STATIC_WALL_BOTTOM].tile_type = TILE_TYPE_INSTANCE;
		map_wall[COLLISION_STATIC_WALL_BOTTOM].col_shape = NULL; // dummy
		collision_manager_create_static_wall(COLLISION_STATIC_WALL_BOTTOM, &map_wall[COLLISION_STATIC_WALL_BOTTOM], tile_inst1->col_shape->b2body, tile_inst2->col_shape->b2body);

		for (map_index = bottom_pos + 1; map_index < bottom_pos + (connected_region - 1); map_index++) {
			tile_instance_data_t* tile_inst = map_raw_data[MAP_TYPE_BLOCK] + map_index;
			if (tile_inst->col_shape->b2body) {
				g_stage_world->DestroyBody(tile_inst->col_shape->b2body);
				tile_inst->col_shape->b2body = NULL;
			}
		}
	}
}

void map_manager_break_block(int x, int y, int w /* block */, int h /* block */) {
	int h_index = y / tile_height;
	int w_index = x / tile_width;

	int start_h_index = MAX(0, h_index - h);
	int start_w_index = MAX(0, w_index - w);
	int end_h_index = MIN(layer_height, h_index + h + 1);
	int end_w_index = MIN(layer_width, w_index + w + 1);

	for (int j = start_h_index; j < end_h_index; j++) {
		int map_index = j * layer_width + start_w_index;
		for (int i = start_w_index; i < end_w_index; i++) {
			tile_instance_data_t* tile_inst = map_raw_data[MAP_TYPE_BLOCK] + map_index;
			int tile = tile_inst->id;
			if ((tile) && (tile_inst->breakable == TILE_BREAKABLE_TRUE)) {
				int x_pos = i * g_tile_width;
				int y_pos = j * g_tile_height;

				// delete BLOCK
				if (tile_inst->col_shape->b2body) {
					g_stage_world->DestroyBody(tile_inst->col_shape->b2body);
					tile_inst->col_shape->b2body = NULL;
					tile_inst->id = 0;
				}

				// create trush item
				std::string effect_path = "units/effect/trash/trash.unit";
				unit_manager_create_effect(x_pos, y_pos, unit_manager_search_effect(effect_path));

				// create smoke effect
				effect_path = "units/effect/smoke/smoke.unit";
				unit_manager_create_effect(x_pos, y_pos, unit_manager_search_effect(effect_path));
			}
			map_index++;
		}
	}

}

int map_manager_load_tile(std::string path, tile_data_t* tile)
{
	bool read_flg[TILE_TAG_END] = { false };

	std::ifstream inFile(g_base_path + "data/" + path);
	if (inFile.is_open()) {
		std::string line;
		while (std::getline(inFile, line)) {
			if (line == "") continue;
			if (line[0] == '#') continue;

			if (line == "[tile]") {
				read_flg[TILE_TAG_TILE] = true;
				tile_base_data_t* tile_base = (tile_base_data_t*)tile;
				tile_base->type = UNIT_TYPE_TILE;
				tile_base->tile_type = TILE_TYPE_BASE;
				tile_base->breakable = TILE_BREAKABLE_FALSE;
			}
			if (line == "[/tile]") { read_flg[TILE_TAG_TILE] = false; continue; }
			if (line == "[frame]") {
				read_flg[TILE_TAG_FRAME] = true;
				tile_base_data_t* tile_base = (tile_base_data_t*)tile;
				tile_base->anim = animation_manager_new_anim_data();
				animation_manager_new_anim_stat_base_data(tile_base->anim);
				continue;
			}
			if (line == "[/frame]") { read_flg[TILE_TAG_FRAME] = false; continue; }
			if (line == "[img]") { read_flg[TILE_TAG_IMG] = true;  continue; }
			if (line == "[/img]") { read_flg[TILE_TAG_IMG] = false; continue; }
			if (line == "[collision]") { read_flg[TILE_TAG_COLLISION] = true;  continue; }
			if (line == "[/collision]") { read_flg[TILE_TAG_COLLISION] = false; continue; }

			if (read_flg[TILE_TAG_TILE]) {
				load_tile_basic(line, tile);
			}
			if (read_flg[TILE_TAG_FRAME]) {
				load_tile_frame(line, tile);
			}
			if (read_flg[TILE_TAG_IMG]) {
				load_tile_img(line, tile);
			}
			if (read_flg[TILE_TAG_COLLISION]) {
				load_tile_collision(line, tile);
			}
		}
		inFile.close();
	}
	else {
		LOG_ERROR("map_manager_load_tile %s error\n", path.c_str());
		return 1;
	}
	return 0;
}

static void load_tile_basic(std::string& line, tile_data_t* tile) {
	std::string key;
	std::string value;
	game_utils_split_key_value(line, key, value);

	if (key == "breakable") tile->breakable = value == "yes" ? TILE_BREAKABLE_TRUE : TILE_BREAKABLE_FALSE;
}

static void load_tile_frame(std::string& line, tile_data_t* tile) {
	std::string key;
	std::string value;
	game_utils_split_key_value(line, key, value);

	tile_base_data_t* tile_base = (tile_base_data_t*)tile;
	if (key == "duration") {
		if (value == "*") {
			tile_base->anim->anim_stat_base_list[ANIM_STAT_IDLE]->type = ANIM_TYPE_STATIC;
			anim_frame_data_t* anim_frame_data = animation_manager_new_anim_frame();
			tile_base->anim->anim_stat_base_list[ANIM_STAT_IDLE]->frame_list[0] = anim_frame_data;
			anim_frame_data->frame_time = 0;
			tile_base->anim->anim_stat_base_list[ANIM_STAT_IDLE]->total_time = 0;
			tile_base->anim->anim_stat_base_list[ANIM_STAT_IDLE]->frame_size = 1;
		}
		else {
			std::vector<int> int_list;
			game_utils_split_conmma(value, int_list);

			tile_base->anim->anim_stat_base_list[ANIM_STAT_IDLE]->type = ANIM_TYPE_DYNAMIC;
			tile_base->anim->anim_stat_base_list[ANIM_STAT_IDLE]->total_time = 0;
			for (int i = 0; i < int_list.size(); i++) {
				anim_frame_data_t* anim_frame_data = animation_manager_new_anim_frame();
				tile_base->anim->anim_stat_base_list[ANIM_STAT_IDLE]->frame_list[i] = anim_frame_data;
				anim_frame_data->frame_time = int_list[i];
				tile_base->anim->anim_stat_base_list[ANIM_STAT_IDLE]->total_time += int_list[i];
			}
			tile_base->anim->anim_stat_base_list[ANIM_STAT_IDLE]->frame_size = (int)int_list.size();
		}
	}
}

static void load_tile_img(std::string& line, tile_data_t* tile) {
	std::string key;
	std::string value;
	game_utils_split_key_value(line, key, value);

	tile_base_data_t* tile_base = (tile_base_data_t*)tile;
	if (key == "layer") {
		tile_base->anim->anim_stat_base_list[ANIM_STAT_IDLE]->tex_layer = atoi(value.c_str());
	}
	if (key == "dir_path") {
		dir_path = value;
	}
	if (key == "path") {
		std::vector<std::string> str_list;
		game_utils_split_conmma(value, str_list);

		std::string image_filename = dir_path + "/";
		for (int i = 0; i < str_list.size(); i++) {
			anim_frame_data_t* anim_frame_data = tile_base->anim->anim_stat_base_list[ANIM_STAT_IDLE]->frame_list[i];
			anim_frame_data->tex = resource_manager_getTextureFromPath(image_filename + str_list[i]);
		}
	}
	if (key == "effect") {
		std::vector<std::string> str_list;
		game_utils_split_conmma(value, str_list);

		for (int fi = 0; fi < str_list.size(); fi++) {
			if (str_list[fi].substr(0, 10) == "tile_clip(") {
				int sep_index[4] = { 0 };
				int sep_i = 0;
				for (int i = 10; i < str_list[fi].size(); i++) {
					if (str_list[fi][i] == ':') {
						sep_index[sep_i] = i;
						sep_i++;
						continue;
					}
					if (str_list[fi][i] == ')') {
						sep_index[sep_i] = i;
						sep_i++;
						break;
					}
				}
				if (sep_i != 4) {
					LOG_ERROR("load_tile_img effect title_clip error\n");
					return; // format error
				}

				int x = atoi(str_list[fi].substr(10, sep_index[0] - 10).c_str());
				int y = atoi(str_list[fi].substr(sep_index[0] + 1, sep_index[1] - sep_index[0]).c_str());
				int w = atoi(str_list[fi].substr(sep_index[1] + 1, sep_index[2] - sep_index[1]).c_str());
				int h = atoi(str_list[fi].substr(sep_index[2] + 1, sep_index[3] - sep_index[2]).c_str());

				anim_frame_data_t* anim_frame_data = tile_base->anim->anim_stat_base_list[ANIM_STAT_IDLE]->frame_list[fi];
				anim_frame_data->src_rect = { x, y, w, h };
			}
		}
	}
}

static void load_tile_collision(std::string& line, tile_data_t* tile) {
	std::string key, value;
	game_utils_split_key_value(line, key, value);

	if (value == "") value = "0";
	if (key == "type") {
		if (value == "BOX_S") {
			shape_box_data* new_shape = (shape_box_data*)(new shape_data);
			new_shape->type = COLLISION_TYPE_BOX_S;
			((tile_base_data_t*)tile)->col_shape = (shape_data*)new_shape;
		}
		else if (value == "ROUND_S") {
			shape_round_data* new_shape = (shape_round_data*)(new shape_data);
			new_shape->type = COLLISION_TYPE_ROUND_S;
			((tile_base_data_t*)tile)->col_shape = (shape_data*)new_shape;
		}
	}
	if (key == "group") collision_manager_set_group(((tile_base_data_t*)tile)->col_shape, value);

	if (key == "x") {
		if (((tile_base_data_t*)tile)->col_shape->type == COLLISION_TYPE_BOX_S) {
			((shape_box_data*)((tile_base_data_t*)tile)->col_shape)->offset_x = atoi(value.c_str());
		}
		else if (((tile_base_data_t*)tile)->col_shape->type == COLLISION_TYPE_ROUND_S)
		{
			((shape_round_data*)((tile_base_data_t*)tile)->col_shape)->offset_x = atoi(value.c_str());
		}
	}
	if (key == "y") {
		if (((tile_base_data_t*)tile)->col_shape->type == COLLISION_TYPE_BOX_S) {
			((shape_box_data*)((tile_base_data_t*)tile)->col_shape)->offset_y = atoi(value.c_str());
		}
		else if (((tile_base_data_t*)tile)->col_shape->type == COLLISION_TYPE_ROUND_S)
		{
			((shape_round_data*)((tile_base_data_t*)tile)->col_shape)->offset_y = atoi(value.c_str());
		}
	}

	if (key == "w") ((shape_box_data*)((tile_base_data_t*)tile)->col_shape)->w = atoi(value.c_str());
	if (key == "h") ((shape_box_data*)((tile_base_data_t*)tile)->col_shape)->h = atoi(value.c_str());
	if (key == "r") ((shape_round_data*)((tile_base_data_t*)tile)->col_shape)->r = atoi(value.c_str());
}


int map_manager_load(std::string path)
{
	int img_width = 0;
	int img_height = 0;
	std::string image_filename = "";
	std::string layer_name = "";

	layer_width = 0;
	layer_height = 0;
	tile_width = 0;
	tile_height = 0;

	bool read_flg[MAP_TAG_END] = { false };

	std::ifstream inFile(g_base_path + "data/" + path);
	if (inFile.is_open()) {
		std::string line;
		while (std::getline(inFile, line)) {
			int seek_index = 0;
			for (seek_index = 0; seek_index < line.size(); seek_index++) {
				if (line[seek_index] != ' ') {
					break;
				}
			}
			if (seek_index == line.size()) continue; // not found string

			if (line.substr(seek_index, 5) == "<map ") {
				read_flg[MAP_TAG_MAP] = true;
				std::string map_element = line.substr(seek_index, line.size() - seek_index);
				load_map(map_element);
				read_flg[MAP_TAG_MAP] = false;
				continue;
			}

			if (line.substr(seek_index, 9) == "<tileset ") {
				read_flg[MAP_TAG_TILESET] = true;
				std::string tileset_element = line.substr(seek_index, line.size() - seek_index);
				load_tileset(tileset_element, &tile_width, &tile_height);
				g_tile_width = tile_width; g_tile_height = tile_height;
				read_flg[MAP_TAG_TILESET] = false;
				continue;
			}

			if (line.substr(seek_index, 7) == "<image ") {
				read_flg[MAP_TAG_IMAGE] = true;
				std::string image_element = line.substr(seek_index, line.size() - seek_index);
				load_image(image_element, image_filename, &img_width, &img_height);
				read_flg[MAP_TAG_IMAGE] = false;
				continue;
			}

			if (line.substr(seek_index, 7) == "<layer ") {
				read_flg[MAP_TAG_LAYER] = true;
				std::string layer_element = line.substr(seek_index, line.size() - seek_index);
				load_layer(layer_element, layer_name, &layer_width, &layer_height);
				g_map_x_max = layer_width;
				g_map_y_max = layer_height;
				read_flg[MAP_TAG_LAYER] = false;
				continue;
			}

			if (line.substr(seek_index, 6) == "<data ") {
				read_flg[MAP_TAG_DATA] = true;
				size_t map_size = (size_t)layer_width * layer_height;
				if (layer_name == "1") {
					map_raw_data[MAP_TYPE_FIELD] = new tile_instance_data_t[map_size];
					read_tile_type = MAP_TYPE_FIELD;

					map_field_data_t* tmp = (map_field_data_t*)&map_data_list[MAP_TYPE_FIELD];
					tmp->type = MAP_TYPE_FIELD;
					tmp->layer = MAP_TYPE_FIELD;
					tmp->x = layer_width;
					tmp->y = layer_height;
					tmp->map_raw_data = map_raw_data[MAP_TYPE_FIELD];
				}
				else if (layer_name == "2") {
					map_raw_data[MAP_TYPE_BLOCK] = new tile_instance_data_t[map_size];
					read_tile_type = MAP_TYPE_BLOCK;

					map_block_data_t* tmp = (map_block_data_t*)&map_data_list[MAP_TYPE_BLOCK];
					tmp->type = MAP_TYPE_BLOCK;
					tmp->layer = MAP_TYPE_BLOCK;
					tmp->x = layer_width;
					tmp->y = layer_height;
					tmp->map_raw_data = map_raw_data[MAP_TYPE_BLOCK];
				}
				else if (layer_name == "3") {
					map_raw_data[MAP_TYPE_EFFECT] = new tile_instance_data_t[map_size];
					read_tile_type = MAP_TYPE_EFFECT;

					map_effect_data_t* tmp = (map_effect_data_t*)&map_data_list[MAP_TYPE_EFFECT];
					tmp->type = MAP_TYPE_EFFECT;
					tmp->layer = MAP_TYPE_EFFECT;
					tmp->x = layer_width;
					tmp->y = layer_height;
					tmp->map_raw_data = map_raw_data[MAP_TYPE_EFFECT];
				}

				read_tile_index = 0;
				continue;
			}
			if (line.substr(seek_index, 7) == "</data>") {
				read_flg[MAP_TAG_DATA] = false;
				continue;
			}
			if (read_flg[MAP_TAG_DATA]) {
				load_data(line);
			}
		}
		inFile.close();
	}
	else {
		LOG_ERROR("map_manager_load %s error\n", path.c_str());
		return 1;
	}

	// load *.tile
	std::string tile_files = g_base_path + "data/" + game_utils_upper_folder(path) + "/*.tile";
	WIN32_FIND_DATAA find_file_data;
	HANDLE h_find = FindFirstFileA(tile_files.c_str(), &find_file_data);
	if (h_find == INVALID_HANDLE_VALUE) {
		LOG_ERROR("map_manager_load FindFirstFileA() error\n", path.c_str());
		return 1;
	}

	do {
		if (!(find_file_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
			std::string filename = find_file_data.cFileName;
			std::string index_str = filename.substr(0, filename.size() - 5);
			int index = atoi(index_str.c_str());
			map_manager_load_tile(game_utils_upper_folder(path) + "/" + filename, &tile_tex[index]);
		}
	} while (FindNextFile(h_find, &find_file_data) != 0);
	FindClose(h_find);

	return 0;
}

static void load_map(std::string& line) {
	/* do nothing */
}

static void load_tileset(std::string& line, int* tile_width, int* tile_height) {

	int seek_index = 0;
	int end_index = 0;

	for (seek_index = 0; seek_index < line.size(); seek_index++) {
		if (line[seek_index] == ' ') continue;
		if (seek_index == line.size()) break; // not found string

		if (line.substr(seek_index, 8) == "<tileset") {
			seek_index += 8;
			continue;
		}
		if (line.substr(seek_index, 11) == "tilewidth=\"") {
			seek_index += 11;
			for (int i = seek_index; i < line.size(); i++) {
				if (line[i] == '"') {
					end_index = i;
					break;
				}
			}
			std::string val = line.substr(seek_index, (size_t)end_index - seek_index);
			*tile_width = atoi(val.c_str());

			seek_index = end_index;
			continue;
		}
		if (line.substr(seek_index, 12) == "tileheight=\"") {
			seek_index += 12;
			for (int i = seek_index; i < line.size(); i++) {
				if (line[i] == '"') {
					end_index = i;
					break;
				}
			}
			std::string val = line.substr(seek_index, (size_t)end_index - seek_index);
			*tile_height = atoi(val.c_str());

			seek_index = end_index;
			break;
		}
	}
}

static void load_image(std::string& line, std::string& filename, int* width, int* height) {
	int seek_index = 0;
	int end_index = 0;

	for (seek_index = 0; seek_index < line.size(); seek_index++) {
		if (line[seek_index] == ' ') continue;
		if (seek_index == line.size()) break; // not found string

		if (line.substr(seek_index, 6) == "<image") {
			seek_index += 6;
			continue;
		}
		if (line.substr(seek_index, 8) == "source=\"") {
			seek_index += 8;
			for (int i = seek_index; i < line.size(); i++) {
				if (line[i] == '"') {
					end_index = i;
					break;
				}
			}
			filename = line.substr(seek_index, (size_t)end_index - seek_index);

			seek_index = end_index;
			continue;
		}
		if (line.substr(seek_index, 7) == "width=\"") {
			seek_index += 7;
			for (int i = seek_index; i < line.size(); i++) {
				if (line[i] == '"') {
					end_index = i;
					break;
				}
			}
			std::string val = line.substr(seek_index, (size_t)end_index - seek_index);
			*width = atoi(val.c_str());

			seek_index = end_index;
			continue;
		}
		if (line.substr(seek_index, 8) == "height=\"") {
			seek_index += 8;
			for (int i = seek_index; i < line.size(); i++) {
				if (line[i] == '"') {
					end_index = i;
					break;
				}
			}
			std::string val = line.substr(seek_index, (size_t)end_index - seek_index);
			*height = atoi(val.c_str());

			seek_index = end_index;
			break;
		}
	}
}

static void load_layer(std::string& line, std::string& name, int* width, int* height) {
	int seek_index = 0;
	int end_index = 0;

	for (seek_index = 0; seek_index < line.size(); seek_index++) {
		if (line[seek_index] == ' ') continue;
		if (seek_index == line.size()) break; // not found string

		if (line.substr(seek_index, 6) == "<layer") {
			seek_index += 6;
			continue;
		}

		if (line.substr(seek_index, 6) == "name=\"") {
			seek_index += 6;
			for (int i = seek_index; i < line.size(); i++) {
				if (line[i] == '"') {
					end_index = i;
					break;
				}
			}
			name = line.substr(seek_index, (size_t)end_index - seek_index);

			seek_index = end_index;
			continue;
		}
		if (line.substr(seek_index, 7) == "width=\"") {
			seek_index += 7;
			for (int i = seek_index; i < line.size(); i++) {
				if (line[i] == '"') {
					end_index = i;
					break;
				}
			}
			std::string val = line.substr(seek_index, (size_t)end_index - seek_index);
			*width = atoi(val.c_str());

			seek_index = end_index;
			continue;
		}
		if (line.substr(seek_index, 8) == "height=\"") {
			seek_index += 8;
			for (int i = seek_index; i < line.size(); i++) {
				if (line[i] == '"') {
					end_index = i;
					break;
				}
			}
			std::string val = line.substr(seek_index, (size_t)end_index - seek_index);
			*height = atoi(val.c_str());

			seek_index = end_index;
			break;
		}
	}
}

static void load_data(std::string& line) {
	std::vector<int> int_list;
	game_utils_split_conmma(line, int_list);

	for (int i = 0; i < int_list.size(); i++) {
		(map_raw_data[read_tile_type] + read_tile_index)->id = int_list[i];
		read_tile_index++;
	}
}
