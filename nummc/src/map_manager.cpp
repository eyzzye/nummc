#include <windows.h>      // for FindFirstFileA()
#include "game_common.h"
#include "map_manager.h"

#include "gui_common.h"
#include "game_timer.h"
#include "game_log.h"
#include "game_utils.h"
#include "memory_manager.h"
#include "resource_manager.h"
#include "unit_manager.h"
#include "game_window.h"
#include "stage_manager.h"
#include "sound_manager.h"

// global variables
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

// local define
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

// tile functions
static int map_manager_load_tile(char* path, tile_data_t* tile);
static void map_manager_create_tile_instance(tile_instance_data_t* tile_inst, int w, int h, bool skip_create_col);
static void map_manager_delete_tile_instance_col(tile_instance_data_t* tile_inst);
static void load_tile_basic(char* line, tile_data_t* tile);
static void load_tile_frame(char* line, tile_data_t* tile);
static void load_tile_img(char* line, tile_data_t* tile);

// section map functions
static int map_manager_load_data_element(char* path);
static void load_map_element(char* line);
static void load_tileset(char* line, int* tile_width, int* tile_height);
static void load_image(char* line, char* filename, int* width, int* height);
static void load_layer(char* line, char* name, int* width, int* height);
static void load_data(char* line);
static void load_data_to_stage_data(char* line, int tile_type);

//
// static variables
//
static map_data_t map_data_list[MAP_TYPE_END];

static int layer_width;
static int layer_height;
static int tile_width;
static int tile_height;

// map_raw_data
static tile_instance_data_t* map_raw_data[MAP_TYPE_END];
static tile_instance_data_t map_wall[COLLISION_STATIC_WALL_NUM]; // for invisible col_shape

// map_raw_data_buffer
#define MAP_RAW_DATA_BUFFER_SIZE  (MAP_WIDTH_NUM_MAX * MAP_HEIGHT_NUM_MAX * MAP_TYPE_END)
static tile_instance_data_t map_raw_data_buffer[MAP_RAW_DATA_BUFFER_SIZE];

// tile data
static tile_data_t tile_tex[TILE_TEX_NUM];

// unit path
static const char* trush_effect_path     = "units/effect/trash/trash.unit";
static const char* door_boss_effect_path = "units/effect/door/boss.unit";
static const char* door_nest_effect_path = "units/effect/door/nest.unit";
static const char* smoke_effect_path     = "units/effect/smoke/smoke.unit";
static const char* bom_event_item_path   = "units/items/bom/event/event.unit";
static const char* go_next_path          = "units/trap/go_next/go_next.unit";

// tmp variables
static char dir_path[GAME_FULL_PATH_MAX];
static int dir_path_size;
static int read_tile_type;
static int read_tile_index;
static int write_section_map_index;

//
// stage map variables
//
typedef struct _stage_map_index_t stage_map_index_t;
struct _stage_map_index_t
{
	int x;
	int y;
};
#define TMP_STAGE_MAP_INDEX_SIZE  (STAGE_MAP_WIDTH_NUM * STAGE_MAP_HEIGHT_NUM)
static stage_map_index_t tmp_stage_map_index[TMP_STAGE_MAP_INDEX_SIZE];
static int tmp_stage_map_index_count = 0;

// stage map functions
#define DIRECTION_NUM_IGNORE  (-1)
static bool get_stage_map_connected_type(int x, int y, int* dst_connection_count);
static int set_stage_map_index(int x, int y, stage_map_index_t* dst_stage_map_index, bool normal_flag, int direction_num);
static void generate_stage_map();

//
// extern I/F
//
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

	// map_raw_data_buffer
	memset(map_raw_data_buffer, 0, sizeof(map_raw_data_buffer));

	// map_raw_data
	for (int i = 0; i < MAP_TYPE_END; i++) {
		map_raw_data[i] = &map_raw_data_buffer[MAP_WIDTH_NUM_MAX * MAP_HEIGHT_NUM_MAX * i];
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
	for (int type = 0; type < MAP_TYPE_END; type++) {
		for (int i = 0; i < MAP_WIDTH_NUM_MAX * MAP_HEIGHT_NUM_MAX; i++) {
			if ((map_raw_data[type] + i)) {
				map_manager_delete_tile_instance_col(map_raw_data[type] + i);
			}
		}
		map_raw_data[type] = NULL;
	}

	for (int i = 0; i < COLLISION_STATIC_WALL_NUM; i++)
	{
		map_manager_delete_tile_instance_col(&map_wall[i]);
	}

	for (int i = 0; i < TILE_TEX_NUM; i++)
	{
		shape_data* tmp_tile = ((tile_base_data_t*)&tile_tex[i])->col_shape;
		if (tmp_tile) {
			collision_manager_delete_shape(tmp_tile);
			((tile_base_data_t*)&tile_tex[i])->col_shape = NULL;
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
	int layer_index = MAP_LAYER_TO_INDEX(layer);
	if ((layer_index < 0) || (MAP_TYPE_END <= layer_index)
		|| (map_raw_data[layer_index] == NULL)) {
		return;
	}

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
			// FIELD, BLOCK
			tile_instance_data_t* tile_data = (map_raw_data[layer_index] + map_index);
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
					src_rect = &frame_data->src_rect;
					dst_rect = VIEW_STAGE_RECT(w * tile_width, h * tile_height, src_rect->w, src_rect->h);
					GUI_RenderCopy(frame_data->res_img, src_rect, &dst_rect);
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

void map_manager_create_stage_map()
{
#if 0
	// set start position
	int x = STAGE_MAP_WIDTH_NUM / 2; int y = STAGE_MAP_HEIGHT_NUM / 2;
	g_stage_data->current_stage_map_index = y * STAGE_MAP_WIDTH_NUM + x;
	g_stage_data->stage_map[y * STAGE_MAP_WIDTH_NUM + x].section_id = 0;

	//
	// Test map for Debug
	//
	//      6
	//      2
	// 5 1 [P] 3  7
	//      4
	//      8
	if (g_stage_data->section_list[1] != NULL) g_stage_data->stage_map[(y + 0) * STAGE_MAP_WIDTH_NUM + (x - 1)].section_id = 1;
	if (g_stage_data->section_list[2] != NULL) g_stage_data->stage_map[(y - 1) * STAGE_MAP_WIDTH_NUM + (x + 0)].section_id = 2;
	if (g_stage_data->section_list[3] != NULL) g_stage_data->stage_map[(y + 0) * STAGE_MAP_WIDTH_NUM + (x + 1)].section_id = 3;
	if (g_stage_data->section_list[4] != NULL) g_stage_data->stage_map[(y + 1) * STAGE_MAP_WIDTH_NUM + (x + 0)].section_id = 4;

	if (g_stage_data->section_list[5] != NULL) g_stage_data->stage_map[(y + 0) * STAGE_MAP_WIDTH_NUM + (x - 2)].section_id = 5;
	if (g_stage_data->section_list[6] != NULL) g_stage_data->stage_map[(y - 2) * STAGE_MAP_WIDTH_NUM + (x + 0)].section_id = 6;
	if (g_stage_data->section_list[7] != NULL) g_stage_data->stage_map[(y + 0) * STAGE_MAP_WIDTH_NUM + (x + 2)].section_id = 7;
	if (g_stage_data->section_list[8] != NULL) g_stage_data->stage_map[(y + 2) * STAGE_MAP_WIDTH_NUM + (x + 0)].section_id = 8;
#else
	generate_stage_map();
#endif

	// load basic map settings from section1
	if (g_stage_data->section_list[1] != NULL) {
		char* path = g_stage_data->section_list[1]->map_path;
		map_manager_load(path);

		//full_path = g_base_path + "data/" + game_utils_upper_folder(path) + "/*.tile";
		char full_path[GAME_FULL_PATH_MAX];
		char tile_files[GAME_FULL_PATH_MAX];
		int tmp_path_size = game_utils_upper_folder(path, full_path);
		if (tmp_path_size <= 0) { LOG_ERROR("Error: map_manager_create_stage_map get upper_folder %s\n", path); return; }
		tmp_path_size = game_utils_string_cat(tile_files, full_path, (char*)"/*.tile");
		if (tmp_path_size <= 0) { LOG_ERROR("Error: map_manager_create_stage_map get tile_files %s\n", path); return; }
		tmp_path_size = game_utils_string_cat(full_path, g_base_path, (char*)"data/", (char*)tile_files);
		if (tmp_path_size <= 0) { LOG_ERROR("Error: map_manager_create_stage_map failed get %s\n", path); return; }

		// load *.tile
		WIN32_FIND_DATAA find_file_data;
		HANDLE h_find = FindFirstFileA(full_path, &find_file_data);
		if (h_find == INVALID_HANDLE_VALUE) {
			LOG_ERROR("Error: map_manager_create_stage_map FindFirstFileA() error\n", path);
			return;
		}

		do {
			if (!(find_file_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
				game_utils_upper_folder(path, full_path);
				//tile_files = full_path + "/" + find_file_data.cFileName;
				tmp_path_size = game_utils_string_cat(tile_files, full_path, (char*)"/", find_file_data.cFileName);
				if (tmp_path_size <= 0) { LOG_ERROR("Error: map_manager_create_stage_map get tile_files %s\n", find_file_data.cFileName); return; }

				//index_str = filename.substr(0, filename.size() - 5);
				char index_str[MEMORY_MANAGER_NAME_BUF_SIZE];
				int tile_filename_size = (int)strlen(find_file_data.cFileName);
				game_utils_string_copy_n(index_str, find_file_data.cFileName, tile_filename_size - 5 /* - strlen(".tile") */);
				int index = atoi(index_str);

				map_manager_load_tile(tile_files, &tile_tex[index]);
			}
		} while (FindNextFile(h_find, &find_file_data) != 0);
		FindClose(h_find);
	}
	else {
		LOG_ERROR("ERROR: map_manager_create_stage_map invalid g_stage_data->section_list.size \n");
		return;
	}

	// load g_stage_data->section_list[section_id]->map_path
	//  |
	//  +-> load map_raw_data[] -> g_stage_data->stage_map[g_stage_data->current_stage_map_index].section_map[][]
	for (int i = 0; i < LENGTH_OF(g_stage_data->stage_map); i++) {
		int section_id = g_stage_data->stage_map[i].section_id;
		if (section_id == STAGE_MAP_ID_IGNORE) {
			continue;
		}

		// set section type (copy)
		int section_type = g_stage_data->section_list[section_id]->section_type;
		g_stage_data->stage_map[i].section_type = section_type;

		// clear stat
		if (section_id == SECTION_INDEX_START) { // start section
			g_stage_data->stage_map[i].stat = STAGE_MAP_STAT_HINT;
		}
		else {
			g_stage_data->stage_map[i].stat = STAGE_MAP_STAT_NONE;
		}

		// clear mini_map_icon
		g_stage_data->stage_map[i].mini_map_icon = STAGE_MINI_MAP_ICON_NONE;

		// load section map
		write_section_map_index = i;
		map_manager_load_data_element(g_stage_data->section_list[section_id]->map_path);
	}
}

//
// create/clear I/F
//
static void map_manager_create_tile_instance(tile_instance_data_t* tile_inst, int w, int h, bool skip_create_col)
{
	SDL_Rect* src_rect;
	SDL_Rect dst_rect;

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
		if ((skip_create_col == true) || (base_shape == NULL)) {
			tile_inst->col_shape = NULL;
		}
		else {
			tile_inst->col_shape = collision_manager_create_static_shape(
				base_shape, tile_inst, g_tile_width, g_tile_height,
				&dst_rect.x, &dst_rect.y);
		}
	}
	else {
		tile_inst->obj = NULL;
		tile_inst->col_shape = NULL;
	}
}

static void map_manager_delete_tile_instance_col(tile_instance_data_t* tile_inst)
{
	// delete shape only (don't clear id)
	if (tile_inst->col_shape) {
		collision_manager_delete_shape(tile_inst->col_shape);
		tile_inst->col_shape = NULL;
	}
}

void map_manager_create_instance() {
	if ((map_raw_data[MAP_TYPE_FIELD] == NULL) || (map_raw_data[MAP_TYPE_BLOCK] == NULL)) return;

	int map_index = 0;
	for (int h = 0; h < layer_height; h++) {
		bool skip_create_col_h = false;
		if ((h == 0) || (h == (layer_height - 1))) {
			skip_create_col_h = true;
		}

		for (int w = 0; w < layer_width; w++) {
			bool skip_create_col_w = false;
			if ((w == 0) || (w == (layer_width - 1))) {
				skip_create_col_w = true;
			}

			// FIELD
			tile_instance_data_t* tile_inst = map_raw_data[MAP_TYPE_FIELD] + map_index;
			map_manager_create_tile_instance(tile_inst, w, h, (skip_create_col_h || skip_create_col_w));

			// BLOCK
			tile_inst = map_raw_data[MAP_TYPE_BLOCK] + map_index;
			map_manager_create_tile_instance(tile_inst, w, h, (skip_create_col_h || skip_create_col_w));

			map_index++;
		}
	}
}

void map_manager_clear_all_instance()
{
	for (int type = 0; type < MAP_TYPE_END; type++) {
		for (int i = 0; i < MAP_WIDTH_NUM_MAX * MAP_HEIGHT_NUM_MAX; i++) {
			if ((map_raw_data[type] + i)) {
				map_manager_delete_tile_instance_col(map_raw_data[type] + i);
			}
		}
	}

	for (int i = 0; i < COLLISION_STATIC_WALL_NUM; i++)	{
		map_manager_delete_tile_instance_col(&map_wall[i]);
	}
}

void map_manager_create_wall()
{
	// player margin
	//
	// #       player       #
	// <-  w/2  ->
	//  
	//   #       #      door     #       #
	//   <- w/4 ->
	//   |       |
	//   + wall  + door collision (16x16)
	//  
	// <-> player_x_space
	//
	int player_w_space = 0;
	int player_h_space = 0;
	if (g_player.col_shape->type == COLLISION_TYPE_BOX_D) {
		player_w_space = ((shape_box_data*)g_player.col_shape)->w / 2;
		player_h_space = ((shape_box_data*)g_player.col_shape)->h / 2;
	}
	else if (g_player.col_shape->type == COLLISION_TYPE_ROUND_D) {
		int r = ((shape_round_data*)g_player.col_shape)->r;
		player_w_space = r;
		player_h_space = r;
	}

	if (player_w_space <= (g_tile_width / 4))player_w_space = 0;
	else player_w_space = player_w_space - (g_tile_width / 4);

	if (player_h_space <= (g_tile_height / 4))player_h_space = 0;
	else player_h_space = player_h_space - (g_tile_height / 4);


	// TOP, BOTTOM
	int w = (MAP_WIDTH_NUM_MAX / 2) * g_tile_width - player_w_space;
	int h = g_tile_height;
	int x = 0;
	int y = 0;
	map_wall[COLLISION_STATIC_WALL_TOP_L].type = UNIT_TYPE_TILE;
	map_wall[COLLISION_STATIC_WALL_TOP_L].tile_type = TILE_TYPE_INSTANCE;
	map_wall[COLLISION_STATIC_WALL_TOP_L].col_shape = collision_manager_create_static_wall(COLLISION_STATIC_WALL_TOP_L, &map_wall[COLLISION_STATIC_WALL_TOP_L], x, y, w, h);

	x = (MAP_WIDTH_NUM_MAX / 2) * g_tile_width + g_tile_width + player_w_space;
	//y = 0;
	map_wall[COLLISION_STATIC_WALL_TOP_R].type = UNIT_TYPE_TILE;
	map_wall[COLLISION_STATIC_WALL_TOP_R].tile_type = TILE_TYPE_INSTANCE;
	map_wall[COLLISION_STATIC_WALL_TOP_R].col_shape = collision_manager_create_static_wall(COLLISION_STATIC_WALL_TOP_R, &map_wall[COLLISION_STATIC_WALL_TOP_R], x, y, w, h);

	x = 0;
	y = (MAP_HEIGHT_NUM_MAX - 1) * g_tile_height;
	map_wall[COLLISION_STATIC_WALL_BOTTOM_L].type = UNIT_TYPE_TILE;
	map_wall[COLLISION_STATIC_WALL_BOTTOM_L].tile_type = TILE_TYPE_INSTANCE;
	map_wall[COLLISION_STATIC_WALL_BOTTOM_L].col_shape = collision_manager_create_static_wall(COLLISION_STATIC_WALL_BOTTOM_L, &map_wall[COLLISION_STATIC_WALL_BOTTOM_L], x, y, w, h);

	x = (MAP_WIDTH_NUM_MAX / 2) * g_tile_width + g_tile_width + player_w_space;
	//y = (MAP_HEIGHT_NUM_MAX - 1) * g_tile_height;
	map_wall[COLLISION_STATIC_WALL_BOTTOM_R].type = UNIT_TYPE_TILE;
	map_wall[COLLISION_STATIC_WALL_BOTTOM_R].tile_type = TILE_TYPE_INSTANCE;
	map_wall[COLLISION_STATIC_WALL_BOTTOM_R].col_shape = collision_manager_create_static_wall(COLLISION_STATIC_WALL_BOTTOM_R, &map_wall[COLLISION_STATIC_WALL_BOTTOM_R], x, y, w, h);

	// LEFT, RIGHT
	w = g_tile_width;
	h = (MAP_HEIGHT_NUM_MAX / 2) * g_tile_height - player_h_space;

	x = 0;
	y = 0;
	map_wall[COLLISION_STATIC_WALL_LEFT_U].type = UNIT_TYPE_TILE;
	map_wall[COLLISION_STATIC_WALL_LEFT_U].tile_type = TILE_TYPE_INSTANCE;
	map_wall[COLLISION_STATIC_WALL_LEFT_U].col_shape = collision_manager_create_static_wall(COLLISION_STATIC_WALL_LEFT_U, &map_wall[COLLISION_STATIC_WALL_LEFT_U], x, y, w, h);

	//x = 0;
	y = (MAP_HEIGHT_NUM_MAX / 2) * g_tile_height + g_tile_height + player_h_space;
	map_wall[COLLISION_STATIC_WALL_LEFT_D].type = UNIT_TYPE_TILE;
	map_wall[COLLISION_STATIC_WALL_LEFT_D].tile_type = TILE_TYPE_INSTANCE;
	map_wall[COLLISION_STATIC_WALL_LEFT_D].col_shape = collision_manager_create_static_wall(COLLISION_STATIC_WALL_LEFT_D, &map_wall[COLLISION_STATIC_WALL_LEFT_D], x, y, w, h);

	x = (MAP_WIDTH_NUM_MAX - 1) * g_tile_width;
	y = 0;
	map_wall[COLLISION_STATIC_WALL_RIGHT_U].type = UNIT_TYPE_TILE;
	map_wall[COLLISION_STATIC_WALL_RIGHT_U].tile_type = TILE_TYPE_INSTANCE;
	map_wall[COLLISION_STATIC_WALL_RIGHT_U].col_shape = collision_manager_create_static_wall(COLLISION_STATIC_WALL_RIGHT_U, &map_wall[COLLISION_STATIC_WALL_RIGHT_U], x, y, w, h);

	//x = (MAP_WIDTH_NUM_MAX - 1) * g_tile_width;
	y = (MAP_HEIGHT_NUM_MAX / 2) * g_tile_height + g_tile_height + player_h_space;
	map_wall[COLLISION_STATIC_WALL_RIGHT_D].type = UNIT_TYPE_TILE;
	map_wall[COLLISION_STATIC_WALL_RIGHT_D].tile_type = TILE_TYPE_INSTANCE;
	map_wall[COLLISION_STATIC_WALL_RIGHT_D].col_shape = collision_manager_create_static_wall(COLLISION_STATIC_WALL_RIGHT_D, &map_wall[COLLISION_STATIC_WALL_RIGHT_D], x, y, w, h);
}

void map_manager_create_door()
{
	tile_instance_data_t* tile_inst = NULL;

	// stage info
	int stage_map_index = g_stage_data->current_stage_map_index;
	int stage_map_index_x = stage_map_index % STAGE_MAP_WIDTH_NUM;
	int stage_map_index_y = stage_map_index / STAGE_MAP_WIDTH_NUM;

	// section info
	int center_x = MAP_WIDTH_NUM_MAX / 2;
	int center_y = MAP_HEIGHT_NUM_MAX / 2;

	// wall info (TOP, BOTTOM)
	int w = g_tile_width * MAP_WIDTH_NUM_MAX;
	int h = g_tile_height;

	// STAGE_MAP_FACE_N
	int section_map_index_n = /* 0 * MAP_WIDTH_NUM_MAX + */ center_x;
	int next_stage_map_index = stage_map_index - STAGE_MAP_WIDTH_NUM;
	int x = 0;
	int y = 0;
	if ((0 <= stage_map_index_y - 1) && (g_stage_data->stage_map[next_stage_map_index].section_type == SECTION_TYPE_HIDE)) {
		// reset wall
		if ((map_raw_data[MAP_TYPE_BLOCK] + section_map_index_n)->id != TILE_ID_WALL) {
			(map_raw_data[MAP_TYPE_BLOCK] + section_map_index_n)->id = TILE_ID_WALL;
			tile_inst = map_raw_data[MAP_TYPE_BLOCK] + section_map_index_n;
			map_manager_create_tile_instance(tile_inst, center_x, 0, false);
		}

		// create bom_event item
		int item_id = unit_manager_create_items(center_x * g_tile_width, 0, unit_manager_search_items((char*)bom_event_item_path));
		unit_items_data_t* items_data = unit_manager_get_items(item_id);
		items_data->val1 = STAGE_MAP_FACE_N;
	}
	else if ((0 <= stage_map_index_y - 1) && (g_stage_data->stage_map[next_stage_map_index].section_id != STAGE_MAP_ID_IGNORE)) { // exist N side section
		// setup new door
		if ((map_raw_data[MAP_TYPE_BLOCK] + section_map_index_n)->id != TILE_ID_DOOR) {
			(map_raw_data[MAP_TYPE_BLOCK] + section_map_index_n)->id = TILE_ID_DOOR;

			tile_inst = map_raw_data[MAP_TYPE_BLOCK] + section_map_index_n;
			map_manager_create_tile_instance(tile_inst, center_x, 0, true);

			if (!(g_stage_data->stage_map[next_stage_map_index].stat & STAGE_MAP_STAT_HINT)) { // open mini map
				g_stage_data->stage_map[next_stage_map_index].stat |= STAGE_MAP_STAT_HINT;
			}
		}
		// already set door
		else {
			// do nothing
		}

		// door overlay effect
		const char** effect_path = NULL;
		if (g_stage_data->stage_map[next_stage_map_index].section_type == SECTION_TYPE_BOSS) effect_path = &door_boss_effect_path;
		else if (g_stage_data->stage_map[next_stage_map_index].section_type == SECTION_TYPE_NEST) effect_path = &door_nest_effect_path;
		if (effect_path != NULL) unit_manager_create_effect(center_x * g_tile_width, 0,
			unit_manager_search_effect((char*)*effect_path));
	}

	int map_maskBits = COLLISION_GROUP_MASK_PLAYER | COLLISION_GROUP_MASK_ENEMY | COLLISION_GROUP_MASK_ITEMS | COLLISION_GROUP_MASK_PLAYER_BULLET | COLLISION_GROUP_MASK_ENEMY_BULLET;
	if (map_wall[COLLISION_STATIC_WALL_TOP_DOOR].col_shape == NULL) {
		map_wall[COLLISION_STATIC_WALL_TOP_DOOR].type = UNIT_TYPE_TILE;
		map_wall[COLLISION_STATIC_WALL_TOP_DOOR].tile_type = TILE_TYPE_INSTANCE;
		map_wall[COLLISION_STATIC_WALL_TOP_DOOR].col_shape = collision_manager_create_static_wall(COLLISION_STATIC_WALL_TOP_DOOR, &map_wall[COLLISION_STATIC_WALL_TOP_DOOR], x, y, w, h);
	}
	else {
		// reset wall filter
		collision_manager_set_filter(map_wall[COLLISION_STATIC_WALL_TOP_DOOR].col_shape, map_maskBits);
	}
	collision_manager_set_filter(map_wall[COLLISION_STATIC_WALL_TOP_L].col_shape, (COLLISION_GROUP_U_NONE | COLLISION_GROUP_NONE));
	collision_manager_set_filter(map_wall[COLLISION_STATIC_WALL_TOP_R].col_shape, (COLLISION_GROUP_U_NONE | COLLISION_GROUP_NONE));

	// STAGE_MAP_FACE_S
	int section_map_index_s = (MAP_HEIGHT_NUM_MAX - 1) * MAP_WIDTH_NUM_MAX + center_x;
	next_stage_map_index = stage_map_index + STAGE_MAP_WIDTH_NUM;
	//x = 0;
	y = (MAP_HEIGHT_NUM_MAX - 1) * g_tile_height;
	if ((stage_map_index_y + 1 < STAGE_MAP_HEIGHT_NUM) && (g_stage_data->stage_map[next_stage_map_index].section_type == SECTION_TYPE_HIDE)) {
		// reset wall
		if ((map_raw_data[MAP_TYPE_BLOCK] + section_map_index_s)->id != TILE_ID_WALL) {
			(map_raw_data[MAP_TYPE_BLOCK] + section_map_index_s)->id = TILE_ID_WALL;
			tile_inst = map_raw_data[MAP_TYPE_BLOCK] + section_map_index_s;
			map_manager_create_tile_instance(tile_inst, center_x, (MAP_HEIGHT_NUM_MAX - 1), false);
		}

		// create bom_event item
		int item_id = unit_manager_create_items(center_x * g_tile_width, (MAP_HEIGHT_NUM_MAX - 1) * g_tile_height, unit_manager_search_items((char*)bom_event_item_path));
		unit_items_data_t* items_data = unit_manager_get_items(item_id);
		items_data->val1 = STAGE_MAP_FACE_S;
	}
	else if ((stage_map_index_y + 1 < STAGE_MAP_HEIGHT_NUM) && (g_stage_data->stage_map[next_stage_map_index].section_id != STAGE_MAP_ID_IGNORE)) { // exist S side section
		// setup new door
		if ((map_raw_data[MAP_TYPE_BLOCK] + section_map_index_s)->id != TILE_ID_DOOR) {
			(map_raw_data[MAP_TYPE_BLOCK] + section_map_index_s)->id = TILE_ID_DOOR;

			tile_inst = map_raw_data[MAP_TYPE_BLOCK] + section_map_index_s;
			map_manager_create_tile_instance(tile_inst, center_x, (MAP_HEIGHT_NUM_MAX - 1), true);

			if (!(g_stage_data->stage_map[next_stage_map_index].stat & STAGE_MAP_STAT_HINT)) { // open mini map
				g_stage_data->stage_map[next_stage_map_index].stat |= STAGE_MAP_STAT_HINT;
			}
		}
		// already set door
		else {
			// do nothing
		}

		// door overlay effect
		const char** effect_path = NULL;
		if (g_stage_data->stage_map[next_stage_map_index].section_type == SECTION_TYPE_BOSS) effect_path = &door_boss_effect_path;
		else if (g_stage_data->stage_map[next_stage_map_index].section_type == SECTION_TYPE_NEST) effect_path = &door_nest_effect_path;
		if (effect_path != NULL) unit_manager_create_effect(center_x * g_tile_width, (MAP_HEIGHT_NUM_MAX - 1) * g_tile_height,
			unit_manager_search_effect((char*)*effect_path));
	}

	if (map_wall[COLLISION_STATIC_WALL_BOTTOM_DOOR].col_shape == NULL) {
		map_wall[COLLISION_STATIC_WALL_BOTTOM_DOOR].type = UNIT_TYPE_TILE;
		map_wall[COLLISION_STATIC_WALL_BOTTOM_DOOR].tile_type = TILE_TYPE_INSTANCE;
		map_wall[COLLISION_STATIC_WALL_BOTTOM_DOOR].col_shape = collision_manager_create_static_wall(COLLISION_STATIC_WALL_BOTTOM_DOOR, &map_wall[COLLISION_STATIC_WALL_BOTTOM_DOOR], x, y, w, h);
	}
	else {
		// reset wall filter
		collision_manager_set_filter(map_wall[COLLISION_STATIC_WALL_BOTTOM_DOOR].col_shape, map_maskBits);
	}
	collision_manager_set_filter(map_wall[COLLISION_STATIC_WALL_BOTTOM_L].col_shape, (COLLISION_GROUP_U_NONE | COLLISION_GROUP_NONE));
	collision_manager_set_filter(map_wall[COLLISION_STATIC_WALL_BOTTOM_R].col_shape, (COLLISION_GROUP_U_NONE | COLLISION_GROUP_NONE));

	// wall info (LEFT, RIGHT)
	w = g_tile_width;
	h = g_tile_height * MAP_HEIGHT_NUM_MAX;

	// STAGE_MAP_FACE_W
	int section_map_index_w = center_y * MAP_WIDTH_NUM_MAX /* + 0 */;
	next_stage_map_index = stage_map_index - 1;
	x = 0;
	y = 0;
	if ((0 <= stage_map_index_x - 1) && (g_stage_data->stage_map[next_stage_map_index].section_type == SECTION_TYPE_HIDE)) {
		// reset wall
		if ((map_raw_data[MAP_TYPE_BLOCK] + section_map_index_w)->id != TILE_ID_WALL) {
			(map_raw_data[MAP_TYPE_BLOCK] + section_map_index_w)->id = TILE_ID_WALL;
			tile_inst = map_raw_data[MAP_TYPE_BLOCK] + section_map_index_w;
			map_manager_create_tile_instance(tile_inst, 0, center_y, false);
		}

		// create bom_event item
		int item_id = unit_manager_create_items(0, center_y * g_tile_height, unit_manager_search_items((char*)bom_event_item_path));
		unit_items_data_t* items_data = unit_manager_get_items(item_id);
		items_data->val1 = STAGE_MAP_FACE_W;
	}
	else if ((0 <= stage_map_index_x - 1) && (g_stage_data->stage_map[next_stage_map_index].section_id != STAGE_MAP_ID_IGNORE)) { // exist W side section
		// setup new door
		if ((map_raw_data[MAP_TYPE_BLOCK] + section_map_index_w)->id != TILE_ID_DOOR) {
			(map_raw_data[MAP_TYPE_BLOCK] + section_map_index_w)->id = TILE_ID_DOOR;

			tile_inst = map_raw_data[MAP_TYPE_BLOCK] + section_map_index_w;
			map_manager_create_tile_instance(tile_inst, 0, center_y, true);

			if (!(g_stage_data->stage_map[next_stage_map_index].stat & STAGE_MAP_STAT_HINT)) { // open mini map
				g_stage_data->stage_map[next_stage_map_index].stat |= STAGE_MAP_STAT_HINT;
			}
		}
		// already set door
		else {
			// do nothing
		}

		// door overlay effect
		const char** effect_path = NULL;
		if (g_stage_data->stage_map[next_stage_map_index].section_type == SECTION_TYPE_BOSS) effect_path = &door_boss_effect_path;
		else if (g_stage_data->stage_map[next_stage_map_index].section_type == SECTION_TYPE_NEST) effect_path = &door_nest_effect_path;
		if (effect_path != NULL) unit_manager_create_effect(0, center_y * g_tile_height,
			unit_manager_search_effect((char*)*effect_path));
	}

	if (map_wall[COLLISION_STATIC_WALL_LEFT_DOOR].col_shape == NULL) {
		map_wall[COLLISION_STATIC_WALL_LEFT_DOOR].type = UNIT_TYPE_TILE;
		map_wall[COLLISION_STATIC_WALL_LEFT_DOOR].tile_type = TILE_TYPE_INSTANCE;
		map_wall[COLLISION_STATIC_WALL_LEFT_DOOR].col_shape = collision_manager_create_static_wall(COLLISION_STATIC_WALL_LEFT_DOOR, &map_wall[COLLISION_STATIC_WALL_LEFT_DOOR], x, y, w, h);
	}
	else {
		// reset wall filter
		collision_manager_set_filter(map_wall[COLLISION_STATIC_WALL_LEFT_DOOR].col_shape, map_maskBits);
	}
	collision_manager_set_filter(map_wall[COLLISION_STATIC_WALL_LEFT_U].col_shape, (COLLISION_GROUP_U_NONE | COLLISION_GROUP_NONE));
	collision_manager_set_filter(map_wall[COLLISION_STATIC_WALL_LEFT_D].col_shape, (COLLISION_GROUP_U_NONE | COLLISION_GROUP_NONE));

	// STAGE_MAP_FACE_E
	int section_map_index_e = center_y * MAP_WIDTH_NUM_MAX + (MAP_WIDTH_NUM_MAX - 1);
	next_stage_map_index = stage_map_index + 1;
	x = (MAP_WIDTH_NUM_MAX - 1) * g_tile_width;
	//y = 0;
	if ((stage_map_index_x + 1 < STAGE_MAP_WIDTH_NUM) && (g_stage_data->stage_map[next_stage_map_index].section_type == SECTION_TYPE_HIDE)) {
		// reset wall
		if ((map_raw_data[MAP_TYPE_BLOCK] + section_map_index_e)->id != TILE_ID_WALL) {
			(map_raw_data[MAP_TYPE_BLOCK] + section_map_index_e)->id = TILE_ID_WALL;
			tile_inst = map_raw_data[MAP_TYPE_BLOCK] + section_map_index_e;
			map_manager_create_tile_instance(tile_inst, (MAP_WIDTH_NUM_MAX - 1), center_y, false);
		}

		// create bom_event item
		int item_id = unit_manager_create_items((MAP_WIDTH_NUM_MAX - 1) * g_tile_width, center_y * g_tile_height, unit_manager_search_items((char*)bom_event_item_path));
		unit_items_data_t* items_data = unit_manager_get_items(item_id);
		items_data->val1 = STAGE_MAP_FACE_E;
	}
	else if ((stage_map_index_x + 1 < STAGE_MAP_WIDTH_NUM) && (g_stage_data->stage_map[next_stage_map_index].section_id != STAGE_MAP_ID_IGNORE)) { // exist E side section
		// setup new door
		if ((map_raw_data[MAP_TYPE_BLOCK] + section_map_index_e)->id != TILE_ID_DOOR) {
			(map_raw_data[MAP_TYPE_BLOCK] + section_map_index_e)->id = TILE_ID_DOOR;

			tile_inst = map_raw_data[MAP_TYPE_BLOCK] + section_map_index_e;
			map_manager_create_tile_instance(tile_inst, (MAP_WIDTH_NUM_MAX - 1), center_y, true);

			if (!(g_stage_data->stage_map[next_stage_map_index].stat & STAGE_MAP_STAT_HINT)) { // open mini map
				g_stage_data->stage_map[next_stage_map_index].stat |= STAGE_MAP_STAT_HINT;
			}
		}
		// already set door
		else {
			// do nothing
		}

		// door overlay effect
		const char** effect_path = NULL;
		if (g_stage_data->stage_map[next_stage_map_index].section_type == SECTION_TYPE_BOSS) effect_path = &door_boss_effect_path;
		else if (g_stage_data->stage_map[next_stage_map_index].section_type == SECTION_TYPE_NEST) effect_path = &door_nest_effect_path;
		if (effect_path != NULL) unit_manager_create_effect((MAP_WIDTH_NUM_MAX - 1) * g_tile_width, center_y * g_tile_height,
			unit_manager_search_effect((char*)*effect_path));
	}

	if (map_wall[COLLISION_STATIC_WALL_RIGHT_DOOR].col_shape == NULL) {
		map_wall[COLLISION_STATIC_WALL_RIGHT_DOOR].type = UNIT_TYPE_TILE;
		map_wall[COLLISION_STATIC_WALL_RIGHT_DOOR].tile_type = TILE_TYPE_INSTANCE;
		map_wall[COLLISION_STATIC_WALL_RIGHT_DOOR].col_shape = collision_manager_create_static_wall(COLLISION_STATIC_WALL_RIGHT_DOOR, &map_wall[COLLISION_STATIC_WALL_RIGHT_DOOR], x, y, w, h);
	}
	else {
		// reset wall filter
		collision_manager_set_filter(map_wall[COLLISION_STATIC_WALL_RIGHT_DOOR].col_shape, map_maskBits);
	}
	collision_manager_set_filter(map_wall[COLLISION_STATIC_WALL_RIGHT_U].col_shape, (COLLISION_GROUP_U_NONE | COLLISION_GROUP_NONE));
	collision_manager_set_filter(map_wall[COLLISION_STATIC_WALL_RIGHT_D].col_shape, (COLLISION_GROUP_U_NONE | COLLISION_GROUP_NONE));
}

//
// door I/F
//
void map_manager_open_door()
{
	bool open_snd_flag = false;

	// section info
	int center_x = MAP_WIDTH_NUM_MAX / 2;
	int center_y = MAP_HEIGHT_NUM_MAX / 2;

	// clear door
	int section_map_index_n = /* 0 * MAP_WIDTH_NUM_MAX + */ center_x;
	int x = center_x * g_tile_width;
	int y = 0;
	if ((map_raw_data[MAP_TYPE_BLOCK] + section_map_index_n)->id == TILE_ID_DOOR) {
		(map_raw_data[MAP_TYPE_BLOCK] + section_map_index_n)->id = TILE_ID_NONE;
		map_manager_delete_tile_instance_col(map_raw_data[MAP_TYPE_BLOCK] + section_map_index_n);

		int trap_id = unit_manager_create_trap(x, y, unit_manager_search_trap((char*)go_next_path));
		unit_trap_data_t* trap_data = unit_manager_get_trap(trap_id);
		trap_data->sub_id = UNIT_TRAP_GATE_ID_GO_NEXT_N;
		open_snd_flag = true;
	}

	int section_map_index_s = (MAP_HEIGHT_NUM_MAX - 1) * MAP_WIDTH_NUM_MAX + center_x;
	//x = center_x * g_tile_width;
	y = (MAP_HEIGHT_NUM_MAX - 1) * g_tile_height;
	if ((map_raw_data[MAP_TYPE_BLOCK] + section_map_index_s)->id == TILE_ID_DOOR) {
		(map_raw_data[MAP_TYPE_BLOCK] + section_map_index_s)->id = TILE_ID_NONE;
		map_manager_delete_tile_instance_col(map_raw_data[MAP_TYPE_BLOCK] + section_map_index_s);

		int trap_id = unit_manager_create_trap(x, y, unit_manager_search_trap((char*)go_next_path));
		unit_trap_data_t* trap_data = unit_manager_get_trap(trap_id);
		trap_data->sub_id = UNIT_TRAP_GATE_ID_GO_NEXT_S;
		open_snd_flag = true;
	}

	int section_map_index_w = center_y * MAP_WIDTH_NUM_MAX /* + 0 */;
	x = 0;
	y = center_y * g_tile_height;
	if ((map_raw_data[MAP_TYPE_BLOCK] + section_map_index_w)->id == TILE_ID_DOOR) {
		(map_raw_data[MAP_TYPE_BLOCK] + section_map_index_w)->id = TILE_ID_NONE;
		map_manager_delete_tile_instance_col(map_raw_data[MAP_TYPE_BLOCK] + section_map_index_w);

		int trap_id = unit_manager_create_trap(x, y, unit_manager_search_trap((char*)go_next_path));
		unit_trap_data_t* trap_data = unit_manager_get_trap(trap_id);
		trap_data->sub_id = UNIT_TRAP_GATE_ID_GO_NEXT_W;
		open_snd_flag = true;
	}

	int section_map_index_e = center_y * MAP_WIDTH_NUM_MAX + (MAP_WIDTH_NUM_MAX - 1);
	x = (MAP_WIDTH_NUM_MAX - 1) * g_tile_width;
	//y = center_y * g_tile_height;
	if ((map_raw_data[MAP_TYPE_BLOCK] + section_map_index_e)->id == TILE_ID_DOOR) {
		(map_raw_data[MAP_TYPE_BLOCK] + section_map_index_e)->id = TILE_ID_NONE;
		map_manager_delete_tile_instance_col(map_raw_data[MAP_TYPE_BLOCK] + section_map_index_e);

		int trap_id = unit_manager_create_trap(x, y, unit_manager_search_trap((char*)go_next_path));
		unit_trap_data_t* trap_data = unit_manager_get_trap(trap_id);
		trap_data->sub_id = UNIT_TRAP_GATE_ID_GO_NEXT_E;
		open_snd_flag = true;
	}

	if (open_snd_flag) {
		sound_manager_set(resource_manager_getChunkFromPath("sounds/sfx_gate_open.ogg"), SOUND_MANAGER_CH_SFX2);
	}
}

void map_manager_open_hide_door(int stage_map_face)
{
	bool open_snd_flag = false;

	// section info
	int center_x = MAP_WIDTH_NUM_MAX / 2;
	int center_y = MAP_HEIGHT_NUM_MAX / 2;

	// clear door
	int section_map_index_n = /* 0 * MAP_WIDTH_NUM_MAX + */ center_x;
	int x = center_x * g_tile_width;
	int y = 0;
	if (stage_map_face == STAGE_MAP_FACE_N) {
		if ((map_raw_data[MAP_TYPE_BLOCK] + section_map_index_n)->id != TILE_ID_NONE) {
			(map_raw_data[MAP_TYPE_BLOCK] + section_map_index_n)->id = TILE_ID_NONE;
		}
		map_manager_delete_tile_instance_col(map_raw_data[MAP_TYPE_BLOCK] + section_map_index_n);

		int trap_id = unit_manager_create_trap(x, y, unit_manager_search_trap((char*)go_next_path));
		unit_trap_data_t* trap_data = unit_manager_get_trap(trap_id);
		trap_data->sub_id = UNIT_TRAP_GATE_ID_GO_NEXT_N;

		// create trush item
		unit_manager_create_effect(x, y, unit_manager_search_effect((char*)trush_effect_path));

		// open mini map
		int next_stage_map_index = g_stage_data->current_stage_map_index - STAGE_MAP_WIDTH_NUM;
		if (!(g_stage_data->stage_map[next_stage_map_index].stat & STAGE_MAP_STAT_HINT)) {
			g_stage_data->stage_map[next_stage_map_index].stat |= STAGE_MAP_STAT_HINT;
		}

		open_snd_flag = true;
	}

	int section_map_index_s = (MAP_HEIGHT_NUM_MAX - 1) * MAP_WIDTH_NUM_MAX + center_x;
	//x = center_x * g_tile_width;
	y = (MAP_HEIGHT_NUM_MAX - 1) * g_tile_height;
	if (stage_map_face == STAGE_MAP_FACE_S) {
		if ((map_raw_data[MAP_TYPE_BLOCK] + section_map_index_s)->id != TILE_ID_NONE) {
			(map_raw_data[MAP_TYPE_BLOCK] + section_map_index_s)->id = TILE_ID_NONE;
		}
		map_manager_delete_tile_instance_col(map_raw_data[MAP_TYPE_BLOCK] + section_map_index_s);

		int trap_id = unit_manager_create_trap(x, y, unit_manager_search_trap((char*)go_next_path));
		unit_trap_data_t* trap_data = unit_manager_get_trap(trap_id);
		trap_data->sub_id = UNIT_TRAP_GATE_ID_GO_NEXT_S;

		// create trush item
		unit_manager_create_effect(x, y, unit_manager_search_effect((char*)trush_effect_path));

		// open mini map
		int next_stage_map_index = g_stage_data->current_stage_map_index + STAGE_MAP_WIDTH_NUM;
		if (!(g_stage_data->stage_map[next_stage_map_index].stat & STAGE_MAP_STAT_HINT)) {
			g_stage_data->stage_map[next_stage_map_index].stat |= STAGE_MAP_STAT_HINT;
		}

		open_snd_flag = true;
	}

	int section_map_index_w = center_y * MAP_WIDTH_NUM_MAX /* + 0 */;
	x = 0;
	y = center_y * g_tile_height;
	if (stage_map_face == STAGE_MAP_FACE_W) {
		if ((map_raw_data[MAP_TYPE_BLOCK] + section_map_index_w)->id != TILE_ID_NONE) {
			(map_raw_data[MAP_TYPE_BLOCK] + section_map_index_w)->id = TILE_ID_NONE;
		}
		map_manager_delete_tile_instance_col(map_raw_data[MAP_TYPE_BLOCK] + section_map_index_w);

		int trap_id = unit_manager_create_trap(x, y, unit_manager_search_trap((char*)go_next_path));
		unit_trap_data_t* trap_data = unit_manager_get_trap(trap_id);
		trap_data->sub_id = UNIT_TRAP_GATE_ID_GO_NEXT_W;

		// create trush item
		unit_manager_create_effect(x, y, unit_manager_search_effect((char*)trush_effect_path));

		// open mini map
		int next_stage_map_index = g_stage_data->current_stage_map_index - 1;
		if (!(g_stage_data->stage_map[next_stage_map_index].stat & STAGE_MAP_STAT_HINT)) {
			g_stage_data->stage_map[next_stage_map_index].stat |= STAGE_MAP_STAT_HINT;
		}

		open_snd_flag = true;
	}

	int section_map_index_e = center_y * MAP_WIDTH_NUM_MAX + (MAP_WIDTH_NUM_MAX - 1);
	x = (MAP_WIDTH_NUM_MAX - 1) * g_tile_width;
	//y = center_y * g_tile_height;
	if (stage_map_face == STAGE_MAP_FACE_E) {
		if ((map_raw_data[MAP_TYPE_BLOCK] + section_map_index_e)->id != TILE_ID_NONE) {
			(map_raw_data[MAP_TYPE_BLOCK] + section_map_index_e)->id = TILE_ID_NONE;
		}
		map_manager_delete_tile_instance_col(map_raw_data[MAP_TYPE_BLOCK] + section_map_index_e);

		int trap_id = unit_manager_create_trap(x, y, unit_manager_search_trap((char*)go_next_path));
		unit_trap_data_t* trap_data = unit_manager_get_trap(trap_id);
		trap_data->sub_id = UNIT_TRAP_GATE_ID_GO_NEXT_E;

		// create trush item
		unit_manager_create_effect(x, y, unit_manager_search_effect((char*)trush_effect_path));

		// open mini map
		int next_stage_map_index = g_stage_data->current_stage_map_index + 1;
		if (!(g_stage_data->stage_map[next_stage_map_index].stat & STAGE_MAP_STAT_HINT)) {
			g_stage_data->stage_map[next_stage_map_index].stat |= STAGE_MAP_STAT_HINT;
		}

		open_snd_flag = true;
	}

	if (open_snd_flag) {
		sound_manager_set(resource_manager_getChunkFromPath("sounds/sfx_gate_open.ogg"), SOUND_MANAGER_CH_SFX2);
	}
}

void map_manager_stage_map_all_open()
{
	for (int i = 0; i < STAGE_MAP_WIDTH_NUM * STAGE_MAP_HEIGHT_NUM; i++) {
		// open mini map
		if ((g_stage_data->stage_map[i].section_id != STAGE_MAP_ID_IGNORE) && !(g_stage_data->stage_map[i].stat & STAGE_MAP_STAT_HINT)) {
			g_stage_data->stage_map[i].stat |= STAGE_MAP_STAT_HINT;
		}
	}
}

void map_manager_set_door_filter(int go_next_id)
{
	// enable wall
	int map_maskBits = COLLISION_GROUP_MASK_PLAYER | COLLISION_GROUP_MASK_ENEMY | COLLISION_GROUP_MASK_ITEMS | COLLISION_GROUP_MASK_PLAYER_BULLET | COLLISION_GROUP_MASK_ENEMY_BULLET;

	if (go_next_id == UNIT_TRAP_GATE_ID_GO_NEXT_N) {
		collision_manager_set_filter(map_wall[COLLISION_STATIC_WALL_TOP_L].col_shape, map_maskBits);
		collision_manager_set_filter(map_wall[COLLISION_STATIC_WALL_TOP_R].col_shape, map_maskBits);
		collision_manager_set_filter(map_wall[COLLISION_STATIC_WALL_TOP_DOOR].col_shape, (COLLISION_GROUP_U_NONE | COLLISION_GROUP_NONE));
	}
	else if (go_next_id == UNIT_TRAP_GATE_ID_GO_NEXT_S) {
		collision_manager_set_filter(map_wall[COLLISION_STATIC_WALL_BOTTOM_L].col_shape, map_maskBits);
		collision_manager_set_filter(map_wall[COLLISION_STATIC_WALL_BOTTOM_R].col_shape, map_maskBits);
		collision_manager_set_filter(map_wall[COLLISION_STATIC_WALL_BOTTOM_DOOR].col_shape, (COLLISION_GROUP_U_NONE | COLLISION_GROUP_NONE));
	}
	else if (go_next_id == UNIT_TRAP_GATE_ID_GO_NEXT_W) {
		collision_manager_set_filter(map_wall[COLLISION_STATIC_WALL_LEFT_U].col_shape, map_maskBits);
		collision_manager_set_filter(map_wall[COLLISION_STATIC_WALL_LEFT_D].col_shape, map_maskBits);
		collision_manager_set_filter(map_wall[COLLISION_STATIC_WALL_LEFT_DOOR].col_shape, (COLLISION_GROUP_U_NONE | COLLISION_GROUP_NONE));
	}
	else if (go_next_id == UNIT_TRAP_GATE_ID_GO_NEXT_E) {
		collision_manager_set_filter(map_wall[COLLISION_STATIC_WALL_RIGHT_U].col_shape, map_maskBits);
		collision_manager_set_filter(map_wall[COLLISION_STATIC_WALL_RIGHT_D].col_shape, map_maskBits);
		collision_manager_set_filter(map_wall[COLLISION_STATIC_WALL_RIGHT_DOOR].col_shape, (COLLISION_GROUP_U_NONE | COLLISION_GROUP_NONE));
	}
}

//
// load I/F
//
void map_manager_backup_to_section_map()
{
	int stage_map_index = g_stage_data->current_stage_map_index;
	for (int tile_type = 0; tile_type < MAP_TYPE_END; tile_type++) {
		for (int i = 0; i < (MAP_WIDTH_NUM_MAX * MAP_HEIGHT_NUM_MAX); i++) {
			g_stage_data->stage_map[stage_map_index].section_map[tile_type][i] = (map_raw_data[tile_type] + i)->id;
		}
	}
}

void map_manager_load_section_map()
{
	int stage_map_index = g_stage_data->current_stage_map_index;
	for (int tile_type = 0; tile_type < MAP_TYPE_END; tile_type++) {
		for (int i = 0; i < (MAP_WIDTH_NUM_MAX * MAP_HEIGHT_NUM_MAX); i++) {
			(map_raw_data[tile_type] + i)->id = g_stage_data->stage_map[stage_map_index].section_map[tile_type][i];
		}
	}

	// create map_raw_data[]->* from tile_tex[map_raw_data[]->id]
	map_manager_create_instance();
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
				unit_manager_create_effect(x_pos, y_pos, unit_manager_search_effect((char*)trush_effect_path));

				// create smoke effect
				unit_manager_create_effect(x_pos, y_pos, unit_manager_search_effect((char*)smoke_effect_path));
			}
			map_index++;
		}
	}
}

typedef struct _load_tile_callback_data_t load_tile_callback_data_t;
struct _load_tile_callback_data_t {
	bool read_flg[TILE_TAG_END];
	tile_data_t* tile;
};
static load_tile_callback_data_t load_tile_callback_data;
static void load_tile_callback(char* line, int line_size, int line_num, void* argv)
{
	load_tile_callback_data_t* data = (load_tile_callback_data_t*)argv;

	if (line[0] == '\0') return;
	if (line[0] == '#') return;

	if (line[0] == '[') {
		if (STRCMP_EQ(line, "[tile]")) {
			data->read_flg[TILE_TAG_TILE] = true;
			tile_base_data_t* tile_base = (tile_base_data_t*)data->tile;
			tile_base->type = UNIT_TYPE_TILE;
			tile_base->tile_type = TILE_TYPE_BASE;
			tile_base->breakable = TILE_BREAKABLE_FALSE;
			return;
		}
		if (STRCMP_EQ(line, "[/tile]")) { data->read_flg[TILE_TAG_TILE] = false; return; }
		if (STRCMP_EQ(line, "[frame]")) {
			data->read_flg[TILE_TAG_FRAME] = true;
			tile_base_data_t* tile_base = (tile_base_data_t*)data->tile;
			tile_base->anim = animation_manager_new_anim_data();
			animation_manager_new_anim_stat_base_data(tile_base->anim);
			return;
		}
		if (STRCMP_EQ(line, "[/frame]")) { data->read_flg[TILE_TAG_FRAME] = false; return; }
		if (STRCMP_EQ(line, "[img]")) { data->read_flg[TILE_TAG_IMG] = true;  return; }
		if (STRCMP_EQ(line, "[/img]")) { data->read_flg[TILE_TAG_IMG] = false; return; }
		if (STRCMP_EQ(line, "[collision]")) { data->read_flg[TILE_TAG_COLLISION] = true;  return; }
		if (STRCMP_EQ(line, "[/collision]")) { data->read_flg[TILE_TAG_COLLISION] = false; return; }
	}

	if (data->read_flg[TILE_TAG_TILE]) {
		load_tile_basic(line, data->tile);
	}
	if (data->read_flg[TILE_TAG_FRAME]) {
		load_tile_frame(line, data->tile);
	}
	if (data->read_flg[TILE_TAG_IMG]) {
		load_tile_img(line, data->tile);
	}
	if (data->read_flg[TILE_TAG_COLLISION]) {
		load_collision(line, &data->tile->col_shape);
	}
}

static int map_manager_load_tile(char* path, tile_data_t* tile)
{
	// full_path = g_base_path + "data/" + path;
	char full_path[GAME_FULL_PATH_MAX];
	int tmp_path_size = game_utils_string_cat(full_path, g_base_path, (char*)"data/", path);
	if (tmp_path_size == 0) { LOG_ERROR("map_manager_load_tile failed get %s\n", path); return 1; }

	// read file
	memset(load_tile_callback_data.read_flg, 0, sizeof(bool) * TILE_TAG_END);
	load_tile_callback_data.tile = tile;
	int ret = game_utils_files_read_line(full_path, load_tile_callback, (void*)&load_tile_callback_data);
	if (ret != 0) { LOG_ERROR("map_manager_load_tile %s error\n", path); return 1; }

	return 0;
}

static void load_tile_basic(char* line, tile_data_t* tile) {
	char key[MEMORY_MANAGER_NAME_BUF_SIZE];
	char value[MEMORY_MANAGER_NAME_BUF_SIZE];
	game_utils_split_key_value(line, key, value);

	if (STRCMP_EQ(key, "breakable")) {
		tile->breakable = STRCMP_EQ(value,"yes") ? TILE_BREAKABLE_TRUE : TILE_BREAKABLE_FALSE;
		return;
	}
}

static void load_tile_frame(char* line, tile_data_t* tile) {
	char key[MEMORY_MANAGER_NAME_BUF_SIZE];
	char value[MEMORY_MANAGER_STRING_BUF_SIZE];
	game_utils_split_key_value(line, key, value);

	tile_base_data_t* tile_base = (tile_base_data_t*)tile;
	if (STRCMP_EQ(key,"duration")) {
		if (STRCMP_EQ(value,"*")) {
			tile_base->anim->anim_stat_base_list[ANIM_STAT_IDLE]->type = ANIM_TYPE_STATIC;
			anim_frame_data_t* anim_frame_data = animation_manager_new_anim_frame();
			tile_base->anim->anim_stat_base_list[ANIM_STAT_IDLE]->frame_list[0] = anim_frame_data;
			anim_frame_data->frame_time = 0;
			tile_base->anim->anim_stat_base_list[ANIM_STAT_IDLE]->total_time = 0;
			tile_base->anim->anim_stat_base_list[ANIM_STAT_IDLE]->frame_size = 1;
		}
		else {
			int int_list[ANIM_FRAME_NUM_MAX];
			int int_list_size = game_utils_split_conmma_int(value, int_list, ANIM_FRAME_NUM_MAX);

			tile_base->anim->anim_stat_base_list[ANIM_STAT_IDLE]->type = ANIM_TYPE_DYNAMIC;
			tile_base->anim->anim_stat_base_list[ANIM_STAT_IDLE]->total_time = 0;
			for (int i = 0; i < int_list_size; i++) {
				anim_frame_data_t* anim_frame_data = animation_manager_new_anim_frame();
				tile_base->anim->anim_stat_base_list[ANIM_STAT_IDLE]->frame_list[i] = anim_frame_data;
				anim_frame_data->frame_time = int_list[i];
				tile_base->anim->anim_stat_base_list[ANIM_STAT_IDLE]->total_time += int_list[i];
			}
			tile_base->anim->anim_stat_base_list[ANIM_STAT_IDLE]->frame_size = int_list_size;
		}
		return;
	}
}

static void load_tile_img(char* line, tile_data_t* tile) {
	char key[MEMORY_MANAGER_NAME_BUF_SIZE];
	char value[MEMORY_MANAGER_STRING_BUF_SIZE];
	char image_filename[GAME_FULL_PATH_MAX];
	char str_list[MEMORY_MANAGER_STRING_BUF_SIZE * ANIM_FRAME_NUM_MAX];
	int str_list_size;
	game_utils_split_key_value(line, key, value);

	tile_base_data_t* tile_base = (tile_base_data_t*)tile;
	if (STRCMP_EQ(key,"layer")) {
		tile_base->anim->anim_stat_base_list[ANIM_STAT_IDLE]->tex_layer = atoi(value);
		return;
	}
	if (STRCMP_EQ(key,"dir_path")) {
		//dir_path = value;
		if (game_utils_string_copy(dir_path, value) != 0) {
			dir_path_size = 0;
			LOG_ERROR("Error: load_tile_img() failed copy dir_path %s\n", value);
		}
		dir_path_size = (int)strlen(dir_path);
		return;
	}
	if (STRCMP_EQ(key,"path")) {
		str_list_size = game_utils_split_conmma(value, str_list, ANIM_FRAME_NUM_MAX);

		//std::string image_filename = dir_path;
		for (int i = 0; i < str_list_size; i++) {
			anim_frame_data_t* anim_frame_data = tile_base->anim->anim_stat_base_list[ANIM_STAT_IDLE]->frame_list[i];
			game_utils_string_cat(image_filename, dir_path, &str_list[MEMORY_MANAGER_STRING_BUF_SIZE * i]);
			anim_frame_data->res_img = resource_manager_getTextureFromPath(image_filename);
		}
		return;
	}
	if (STRCMP_EQ(key,"effect")) {
		str_list_size = game_utils_split_conmma(value, str_list, ANIM_FRAME_NUM_MAX);

		char keyword_str[MEMORY_MANAGER_STRING_BUF_SIZE];
		for (int fi = 0; fi < str_list_size; fi++) {
			char* frame_effect_str = &str_list[MEMORY_MANAGER_STRING_BUF_SIZE * fi];
			int element_size = (int)strlen(&str_list[MEMORY_MANAGER_STRING_BUF_SIZE * fi]);

			int keyword_size = 10;
			game_utils_string_copy_n(keyword_str, frame_effect_str, keyword_size);
			if (STRCMP_EQ(keyword_str, "tile_clip(")) {
				int sep_index[4] = { 0 };
				int sep_i = 0;
				for (int i = keyword_size; i < element_size; i++) {
					if (frame_effect_str[i] == ':') {
						sep_index[sep_i] = i;
						sep_i++;
						continue;
					}
					if (frame_effect_str[i] == ')') {
						sep_index[sep_i] = i;
						sep_i++;
						break;
					}
				}
				if (sep_i != 4) {
					LOG_ERROR("load_tile_img effect title_clip error\n");
					return; // format error
				}

				//int x = atoi(str_list[fi].substr(10, sep_index[0] - 10).c_str());
				game_utils_string_copy_n(keyword_str, &frame_effect_str[keyword_size], sep_index[0] - keyword_size);
				int x = atoi(keyword_str);

				//int y = atoi(str_list[fi].substr(sep_index[0] + 1, sep_index[1] - sep_index[0]).c_str());
				game_utils_string_copy_n(keyword_str, &frame_effect_str[sep_index[0] + 1], sep_index[1] - (sep_index[0] + 1));
				int y = atoi(keyword_str);

				//int w = atoi(str_list[fi].substr(sep_index[1] + 1, sep_index[2] - sep_index[1]).c_str());
				game_utils_string_copy_n(keyword_str, &frame_effect_str[sep_index[1] + 1], sep_index[2] - (sep_index[1] + 1));
				int w = atoi(keyword_str);

				//int h = atoi(str_list[fi].substr(sep_index[2] + 1, sep_index[3] - sep_index[2]).c_str());
				game_utils_string_copy_n(keyword_str, &frame_effect_str[sep_index[2] + 1], sep_index[3] - (sep_index[2] + 1));
				int h = atoi(keyword_str);

				anim_frame_data_t* anim_frame_data = tile_base->anim->anim_stat_base_list[ANIM_STAT_IDLE]->frame_list[fi];
				anim_frame_data->src_rect = { x, y, w, h };
				continue;
			}
		}
		return;
	}
}

typedef struct _load_map_callback_data_t load_map_callback_data_t;
struct _load_map_callback_data_t {
	bool read_flg[MAP_TAG_END];
	char layer_name[MEMORY_MANAGER_NAME_BUF_SIZE];
};
static load_map_callback_data_t load_map_callback_data;
static void load_map_callback(char* line, int line_size, int line_num, void* argv)
{
	load_map_callback_data_t* data = (load_map_callback_data_t*)argv;

	// seek white space
	int seek_index = 0;
	for (seek_index = 0; seek_index < line_size; seek_index++) {
		if (line[seek_index] != ' ') {
			break;
		}
	}
	if (seek_index == line_size) return; // not found string

	if (line[seek_index] == '<') {
		char line_substr[MEMORY_MANAGER_LINE_BUF_SIZE];
		int ret = game_utils_string_copy_n(line_substr, &line[seek_index], 9);
		if (ret != 0) { LOG_ERROR("Error: load_map_callback get line_substr of %s\n", line); return; }

		// strlen(line_substr) == 9
		if (STRCMP_EQ(line_substr, "<tileset ")) {
			data->read_flg[MAP_TAG_TILESET] = true;
			//std::string tileset_element = line.substr(seek_index, line.size() - seek_index);
			ret = game_utils_string_copy_n(line_substr, &line[seek_index], line_size - seek_index);
			if (ret != 0) { LOG_ERROR("Error: load_map_callback get tileset_element from %s\n", line); return; }
			load_tileset(line_substr, &tile_width, &tile_height);
			g_tile_width = tile_width; g_tile_height = tile_height;
			data->read_flg[MAP_TAG_TILESET] = false;
			return;
		}

		// strlen(line_substr) == 7
		line_substr[7] = '\0';
		if (STRCMP_EQ(line_substr, "<image ")) {
			int img_width, img_height;
			char image_filename[GAME_FULL_PATH_MAX];

			data->read_flg[MAP_TAG_IMAGE] = true;
			//std::string image_element = line.substr(seek_index, line_size - seek_index);
			ret = game_utils_string_copy_n(line_substr, &line[seek_index], line_size - seek_index);
			if (ret != 0) { LOG_ERROR("Error: load_map_callback get image_element from %s\n", line); return; }
			load_image(line_substr, image_filename, &img_width, &img_height);
			data->read_flg[MAP_TAG_IMAGE] = false;
			return;
		}

		if (STRCMP_EQ(line_substr, "<layer ")) {
			data->read_flg[MAP_TAG_LAYER] = true;
			//std::string layer_element = line.substr(seek_index, line_size - seek_index);
			ret = game_utils_string_copy_n(line_substr, &line[seek_index], line_size - seek_index);
			if (ret != 0) { LOG_ERROR("Error: load_map_callback get layer_element from %s\n", line); return; }
			load_layer(line_substr, data->layer_name, &layer_width, &layer_height);
			if ((layer_width != MAP_WIDTH_NUM_MAX) || (layer_height != MAP_HEIGHT_NUM_MAX)) {
				LOG_ERROR("ERROR: load_map_callback %d x %d\n", layer_width, layer_height);
			}
			g_map_x_max = layer_width;
			g_map_y_max = layer_height;
			data->read_flg[MAP_TAG_LAYER] = false;
			return;
		}

		if (STRCMP_EQ(line_substr, "</data>")) {
			data->read_flg[MAP_TAG_DATA] = false;
			return;
		}

		// strlen(line_substr) == 6
		line_substr[6] = '\0';
		if (STRCMP_EQ(line_substr, "<data ")) {
			data->read_flg[MAP_TAG_DATA] = true;
			if (data->layer_name[0] == '1') {
				read_tile_type = MAP_TYPE_FIELD;

				map_field_data_t* tmp = (map_field_data_t*)&map_data_list[MAP_TYPE_FIELD];
				tmp->type = MAP_TYPE_FIELD;
				tmp->layer = MAP_TYPE_FIELD;
				tmp->x = layer_width;
				tmp->y = layer_height;
				tmp->map_raw_data = map_raw_data[MAP_TYPE_FIELD];
			}
			else if (data->layer_name[0] == '2') {
				read_tile_type = MAP_TYPE_BLOCK;

				map_block_data_t* tmp = (map_block_data_t*)&map_data_list[MAP_TYPE_BLOCK];
				tmp->type = MAP_TYPE_BLOCK;
				tmp->layer = MAP_TYPE_BLOCK;
				tmp->x = layer_width;
				tmp->y = layer_height;
				tmp->map_raw_data = map_raw_data[MAP_TYPE_BLOCK];
			}

			read_tile_index = 0;
			return;
		}

		// strlen(line_substr) == 5
		line_substr[5] = '\0';
		if (STRCMP_EQ(line_substr, "<map ")) {
			data->read_flg[MAP_TAG_MAP] = true;
			//std::string map_element = line.substr(seek_index, line_size - seek_index);
			ret = game_utils_string_copy_n(line_substr, &line[seek_index], line_size - seek_index);
			if (ret != 0) { LOG_ERROR("Error: load_map_callback get map_element from %s\n", line); return; }
			load_map_element(line_substr);
			data->read_flg[MAP_TAG_MAP] = false;
			return;
		}
	}

	if (data->read_flg[MAP_TAG_DATA]) {
		load_data(line);
	}
}

int map_manager_load(char* path)
{
	// full_path = g_base_path + "data/" + path;
	char full_path[GAME_FULL_PATH_MAX];
	int tmp_path_size = game_utils_string_cat(full_path, g_base_path, (char*)"data/", path);
	if (tmp_path_size == 0) { LOG_ERROR("map_manager_load failed get %s\n", path); return 1; }

	// init static variables
	layer_width = 0;
	layer_height = 0;
	tile_width = 0;
	tile_height = 0;

	// read file
	memset(load_map_callback_data.read_flg, 0, sizeof(bool)* MAP_TAG_END);
	int ret = game_utils_files_read_line(full_path, load_map_callback, (void*)&load_map_callback_data);
	if (ret != 0) { LOG_ERROR("map_manager_load %s error\n", path); return 1; }

	return 0;
}

typedef struct _load_data_element_callback_data_t load_data_element_callback_data_t;
struct _load_data_element_callback_data_t {
	bool read_flg[MAP_TAG_END];
	int read_tile_type_;
	char layer_name[MEMORY_MANAGER_NAME_BUF_SIZE];
};
static load_data_element_callback_data_t load_data_element_callback_data;
static void load_data_element_callback(char* line, int line_size, int line_num, void* argv)
{
	load_data_element_callback_data_t* data = (load_data_element_callback_data_t*)argv;

	int seek_index = 0;
	for (seek_index = 0; seek_index < line_size; seek_index++) {
		if (line[seek_index] != ' ') {
			break;
		}
	}
	if (seek_index == line_size) return; // not found string

	if (line[seek_index] == '<') {
		char line_substr[MEMORY_MANAGER_LINE_BUF_SIZE];
		int ret = game_utils_string_copy_n(line_substr, &line[seek_index], 7);
		if (ret != 0) { LOG_ERROR("Error: load_data_element_callback get line_substr of %s\n", line); return; }

		// strlen(line_substr) == 7
		if (STRCMP_EQ(line_substr, "<layer ")) {
			data->read_flg[MAP_TAG_LAYER] = true;
			//std::string layer_element = line.substr(seek_index, line_size - seek_index);
			ret = game_utils_string_copy_n(line_substr, &line[seek_index], line_size - seek_index);
			if (ret != 0) { LOG_ERROR("Error: load_data_element_callback get layer_element from %s\n", line); return; }

			int dummy_width, dummy_height;
			load_layer(line_substr, data->layer_name, &dummy_width, &dummy_height);
			if ((dummy_width != MAP_WIDTH_NUM_MAX) || (layer_height != MAP_HEIGHT_NUM_MAX)) {
				LOG_ERROR("ERROR: load_data_element_callback %d x %d\n", dummy_width, dummy_height);
			}
			data->read_flg[MAP_TAG_LAYER] = false;
			return;
		}

		if (STRCMP_EQ(line_substr, "</data>")) {
			data->read_flg[MAP_TAG_DATA] = false;
			return;
		}

		// strlen(line_substr) == 6
		line_substr[6] = '\0';
		if (STRCMP_EQ(line_substr, "<data ")) {
			data->read_flg[MAP_TAG_DATA] = true;
			if (data->layer_name[0] == '1') {
				data->read_tile_type_ = MAP_TYPE_FIELD;
			}
			else if (data->layer_name[0] == '2') {
				data->read_tile_type_ = MAP_TYPE_BLOCK;
			}
			read_tile_index = 0;
			return;
		}
	}

	if (data->read_flg[MAP_TAG_DATA]) {
		load_data_to_stage_data(line, data->read_tile_type_);
	}
}

static int map_manager_load_data_element(char* path)
{
	// full_path = g_base_path + "data/" + path;
	char full_path[GAME_FULL_PATH_MAX];
	int tmp_path_size = game_utils_string_cat(full_path, g_base_path, (char*)"data/", path);
	if (tmp_path_size == 0) { LOG_ERROR("map_manager_load_data_element failed get %s\n", path); return 1; }

	// read file
	memset(load_data_element_callback_data.read_flg, 0, sizeof(bool) * MAP_TAG_END);
	load_data_element_callback_data.read_tile_type_ = -1;
	int ret = game_utils_files_read_line(full_path, load_data_element_callback, (void*)&load_data_element_callback_data);
	if (ret != 0) { LOG_ERROR("map_manager_load_data_element %s error\n", path); return 1; }

	return 0;
}

static void load_map_element(char* line) {
	/* do nothing */
}

static void load_tileset(char* line, int* tile_width, int* tile_height)
{
	int seek_index = 0;
	int end_index = 0;

	int line_size = (int)strlen(line);
	for (seek_index = 0; seek_index < line_size; seek_index++) {
		if (line[seek_index] == ' ') continue;
		if (seek_index == line_size) break; // not found string

		char line_substr[MEMORY_MANAGER_NAME_BUF_SIZE];
		int ret = game_utils_string_copy_n(line_substr, &line[seek_index], 12);
		if (ret != 0) { LOG_ERROR("Error: load_tileset get line_substr from %s\n", line); continue; }

		// strlen(line_substr) == 12
		if (STRCMP_EQ(line_substr, "tileheight=\"")) {
			seek_index += 12;
			for (int i = seek_index; i < line_size; i++) {
				if (line[i] == '"') {
					end_index = i;
					break;
				}
			}

			//std::string val = line.substr(seek_index, (size_t)end_index - seek_index);
			ret = game_utils_string_copy_n(line_substr, &line[seek_index], end_index - seek_index);
			if (ret != 0) { LOG_ERROR("Error: load_tileset get tileheight from %s\n", line); break; }
			*tile_height = atoi(line_substr);

			seek_index = end_index;
			break; // tileset last attr
		}

		// strlen(line_substr) == 11
		line_substr[11] = '\0';
		if (STRCMP_EQ(line_substr, "tilewidth=\"")) {
			seek_index += 11;
			for (int i = seek_index; i < line_size; i++) {
				if (line[i] == '"') {
					end_index = i;
					break;
				}
			}
			//std::string val = line.substr(seek_index, (size_t)end_index - seek_index);
			ret = game_utils_string_copy_n(line_substr, &line[seek_index], end_index - seek_index);
			if (ret != 0) { LOG_ERROR("Error: load_tileset get tilewidth from %s\n", line); break; }
			*tile_width = atoi(line_substr);

			seek_index = end_index;
			continue;
		}

		// strlen(line_substr) == 8
		line_substr[8] = '\0';
		if (STRCMP_EQ(line_substr, "<tileset")) {
			seek_index += 8;
			continue;
		}
	}
}

static void load_image(char* line, char* filename, int* width, int* height) {
	int seek_index = 0;
	int end_index = 0;

	int line_size = (int)strlen(line);
	for (seek_index = 0; seek_index < line_size; seek_index++) {
		if (line[seek_index] == ' ') continue;
		if (seek_index == line_size) break; // not found string

		char line_substr[MEMORY_MANAGER_NAME_BUF_SIZE];
		int ret = game_utils_string_copy_n(line_substr, &line[seek_index], 8);
		if (ret != 0) { LOG_ERROR("Error: load_image get line_substr from %s\n", line); continue; }

		// strlen(line_substr) == 8
		if (STRCMP_EQ(line_substr, "source=\"")) {
			seek_index += 8;
			for (int i = seek_index; i < line_size; i++) {
				if (line[i] == '"') {
					end_index = i;
					break;
				}
			}
			//filename = line.substr(seek_index, (size_t)end_index - seek_index);
			ret = game_utils_string_copy_n(filename, &line[seek_index], end_index - seek_index);
			if (ret != 0) { LOG_ERROR("Error: load_image get filename from %s\n", line); break; }

			seek_index = end_index;
			continue;
		}

		if (STRCMP_EQ(line_substr, "height=\"")) {
			seek_index += 8;
			for (int i = seek_index; i < line_size; i++) {
				if (line[i] == '"') {
					end_index = i;
					break;
				}
			}
			//std::string val = line.substr(seek_index, (size_t)end_index - seek_index);
			ret = game_utils_string_copy_n(line_substr, &line[seek_index], end_index - seek_index);
			if (ret != 0) { LOG_ERROR("Error: load_image get height from %s\n", line); break; }
			*height = atoi(line_substr);

			seek_index = end_index;
			break; // image last attr
		}

		// strlen(line_substr) == 7
		line_substr[7] = '\0';
		if (STRCMP_EQ(line_substr, "width=\"")) {
			seek_index += 7;
			for (int i = seek_index; i < line_size; i++) {
				if (line[i] == '"') {
					end_index = i;
					break;
				}
			}

			//std::string val = line.substr(seek_index, (size_t)end_index - seek_index);
			ret = game_utils_string_copy_n(line_substr, &line[seek_index], end_index - seek_index);
			if (ret != 0) { LOG_ERROR("Error: load_image get width from %s\n", line); break; }
			*width = atoi(line_substr);

			seek_index = end_index;
			continue;
		}

		// strlen(line_substr) == 6
		line_substr[6] = '\0';
		if (STRCMP_EQ(line_substr, "<image")) {
			seek_index += 6;
			continue;
		}
	}
}

static void load_layer(char* line, char* name, int* width, int* height) {
	int seek_index = 0;
	int end_index = 0;

	int line_size = (int)strlen(line);
	for (seek_index = 0; seek_index < line_size; seek_index++) {
		if (line[seek_index] == ' ') continue;
		if (seek_index == line_size) break; // not found string

		char line_substr[MEMORY_MANAGER_NAME_BUF_SIZE];
		int ret = game_utils_string_copy_n(line_substr, &line[seek_index], 8);
		if (ret != 0) { LOG_ERROR("Error: load_layer get line_substr from %s\n", line); continue; }

		// strlen(line_substr) == 8
		if (STRCMP_EQ(line_substr, "height=\"")) {
			seek_index += 8;
			for (int i = seek_index; i < line_size; i++) {
				if (line[i] == '"') {
					end_index = i;
					break;
				}
			}
			//std::string val = line.substr(seek_index, (size_t)end_index - seek_index);
			ret = game_utils_string_copy_n(line_substr, &line[seek_index], end_index - seek_index);
			if (ret != 0) { LOG_ERROR("Error: load_layer get height from %s\n", line); break; }
			*height = atoi(line_substr);

			seek_index = end_index;
			break; // layer last attr
		}

		// strlen(line_substr) == 7
		line_substr[7] = '\0';
		if (STRCMP_EQ(line_substr, "width=\"")) {
			seek_index += 7;
			for (int i = seek_index; i < line_size; i++) {
				if (line[i] == '"') {
					end_index = i;
					break;
				}
			}
			//std::string val = line.substr(seek_index, (size_t)end_index - seek_index);
			ret = game_utils_string_copy_n(line_substr, &line[seek_index], end_index - seek_index);
			if (ret != 0) { LOG_ERROR("Error: load_layer get width from %s\n", line); break; }
			*width = atoi(line_substr);

			seek_index = end_index;
			continue;
		}

		// strlen(line_substr) == 6
		line_substr[6] = '\0';
		if (STRCMP_EQ(line_substr, "<layer")) {
			seek_index += 6;
			continue;
		}

		if (STRCMP_EQ(line_substr, "name=\"")) {
			seek_index += 6;
			for (int i = seek_index; i < line_size; i++) {
				if (line[i] == '"') {
					end_index = i;
					break;
				}
			}
			//name = line.substr(seek_index, (size_t)end_index - seek_index);
			ret = game_utils_string_copy_n(name, &line[seek_index], end_index - seek_index);
			if (ret != 0) { LOG_ERROR("Error: load_layer get name from %s\n", line); break; }

			seek_index = end_index;
			continue;
		}
	}
}

static void load_data(char* line) {
	int int_list[MAP_WIDTH_NUM_MAX * MAP_HEIGHT_NUM_MAX];
	int int_list_size = game_utils_split_conmma_int(line, int_list, MAP_WIDTH_NUM_MAX * MAP_HEIGHT_NUM_MAX);

	for (int i = 0; i < int_list_size; i++) {
		(map_raw_data[read_tile_type] + read_tile_index)->id = int_list[i];
		read_tile_index++;
	}
}

static void load_data_to_stage_data(char* line, int tile_type) {
	int int_list[MAP_WIDTH_NUM_MAX * MAP_HEIGHT_NUM_MAX];
	int int_list_size = game_utils_split_conmma_int(line, int_list, MAP_WIDTH_NUM_MAX * MAP_HEIGHT_NUM_MAX);

	for (int i = 0; i < int_list_size; i++) {
		g_stage_data->stage_map[write_section_map_index].section_map[tile_type][read_tile_index] = (char)int_list[i];
		read_tile_index++;
	}
}

//
// stage map
//
static bool get_stage_map_connected_type(int x, int y, int* dst_connection_count)
{
	bool not_normal_flag = false;
	int connection_count = 0;
	int connected_id, connected_type;
	if (x - 1 >= 0) { // W
		connected_id = g_stage_data->stage_map[y * STAGE_MAP_WIDTH_NUM + (x - 1)].section_id;
		if (connected_id != STAGE_MAP_ID_IGNORE) {
			connection_count += 1;
			connected_type = g_stage_data->section_list[connected_id]->section_type;
			if (connected_type != SECTION_TYPE_NORMAL) not_normal_flag = true;
		}
	}
	if (y - 1 >= 0) { // N
		connected_id = g_stage_data->stage_map[(y - 1) * STAGE_MAP_WIDTH_NUM + x].section_id;
		if (connected_id != STAGE_MAP_ID_IGNORE) {
			connection_count += 1;
			connected_type = g_stage_data->section_list[connected_id]->section_type;
			if (connected_type != SECTION_TYPE_NORMAL) not_normal_flag = true;
		}
	}
	if (x + 1 < STAGE_MAP_WIDTH_NUM) { // E
		connected_id = g_stage_data->stage_map[y * STAGE_MAP_WIDTH_NUM + (x + 1)].section_id;
		if (connected_id != STAGE_MAP_ID_IGNORE) {
			connection_count += 1;
			connected_type = g_stage_data->section_list[connected_id]->section_type;
			if (connected_type != SECTION_TYPE_NORMAL) not_normal_flag = true;
		}
	}
	if (y + 1 < STAGE_MAP_HEIGHT_NUM) { // S
		connected_id = g_stage_data->stage_map[(y + 1) * STAGE_MAP_WIDTH_NUM + x].section_id;
		if (connected_id != STAGE_MAP_ID_IGNORE) {
			connection_count += 1;
			connected_type = g_stage_data->section_list[connected_id]->section_type;
			if (connected_type != SECTION_TYPE_NORMAL) not_normal_flag = true;
		}
	}
	*dst_connection_count = connection_count;
	return not_normal_flag;
}

static int set_stage_map_index(int x, int y, stage_map_index_t* dst_stage_map_index, bool normal_flag, int direction_num)
{
	// register all empty section
	int empty_direction[STAGE_MAP_FACE_END];
	int empty_direction_count = 0;

	// register [x,y]->[empty]->[normal] section
	typedef struct _empty_normal_t empty_normal_t;
	struct _empty_normal_t
	{
		int face;
		int count;
	};
	empty_normal_t empty_normal_direction[STAGE_MAP_FACE_END];
	int empty_normal_direction_count = 0;
	int tmp_connection_count = 0;

	// W
	int index = y * STAGE_MAP_WIDTH_NUM + (x - 1);
	if ((x - 1 >= 0) && (g_stage_data->stage_map[index].section_id == STAGE_MAP_ID_IGNORE)) {
		empty_direction[empty_direction_count] = STAGE_MAP_FACE_W;
		empty_direction_count += 1;

		if (normal_flag && (get_stage_map_connected_type(x - 1, y, &tmp_connection_count) == false)) {
			empty_normal_direction[empty_normal_direction_count].face = STAGE_MAP_FACE_W;
			empty_normal_direction[empty_normal_direction_count].count = tmp_connection_count;
			empty_normal_direction_count += 1;
		}
	}

	// N
	index = (y - 1) * STAGE_MAP_WIDTH_NUM + x;
	if ((y - 1 >= 0) && (g_stage_data->stage_map[index].section_id == STAGE_MAP_ID_IGNORE)) {
		empty_direction[empty_direction_count] = STAGE_MAP_FACE_N;
		empty_direction_count += 1;

		if (normal_flag && (get_stage_map_connected_type(x, y - 1, &tmp_connection_count) == false)) {
			empty_normal_direction[empty_normal_direction_count].face = STAGE_MAP_FACE_N;
			empty_normal_direction[empty_normal_direction_count].count = tmp_connection_count;
			empty_normal_direction_count += 1;
		}
	}

	// E
	index = y * STAGE_MAP_WIDTH_NUM + (x + 1);
	if ((x + 1 < STAGE_MAP_WIDTH_NUM) && (g_stage_data->stage_map[index].section_id == STAGE_MAP_ID_IGNORE)) {
		empty_direction[empty_direction_count] = STAGE_MAP_FACE_E;
		empty_direction_count += 1;

		if (normal_flag && (get_stage_map_connected_type(x + 1, y, &tmp_connection_count) == false)) {
			empty_normal_direction[empty_normal_direction_count].face = STAGE_MAP_FACE_E;
			empty_normal_direction[empty_normal_direction_count].count = tmp_connection_count;
			empty_normal_direction_count += 1;
		}
	}

	// S
	index = (y + 1) * STAGE_MAP_WIDTH_NUM + x;
	if ((y + 1 < STAGE_MAP_HEIGHT_NUM) && (g_stage_data->stage_map[index].section_id == STAGE_MAP_ID_IGNORE)) {
		empty_direction[empty_direction_count] = STAGE_MAP_FACE_S;
		empty_direction_count += 1;

		if (normal_flag && (get_stage_map_connected_type(x, y + 1, &tmp_connection_count) == false)) {
			empty_normal_direction[empty_normal_direction_count].face = STAGE_MAP_FACE_S;
			empty_normal_direction[empty_normal_direction_count].count = tmp_connection_count;
			empty_normal_direction_count += 1;
		}
	}

	int new_stage_map_face = -1;
	// set normal section
	if (normal_flag) {
		if (empty_normal_direction_count == 0) {
			// not found empty section
			return 1;
		}

		// only 1 section
		if (empty_normal_direction_count == 1) {
			if ((direction_num >= 0) && (direction_num != empty_normal_direction[0].count)) {
				return 1;  // invalid direction_num
			}
			new_stage_map_face = empty_normal_direction[0].face;
		}
		// select by random
		else if (empty_normal_direction_count > 1) {
			int selected_index = game_utils_random_gen(empty_normal_direction_count - 1, 0);

			// check direction_num
			if (direction_num >= 0) {
				// search 4 direction
				for (int direction_index = 0; direction_index < empty_normal_direction_count; direction_index++) {
					if (direction_num != empty_normal_direction[selected_index].count) {
						selected_index += 1;
						if (selected_index >= empty_normal_direction_count) selected_index = 0;
						continue;
					}
					else {
						new_stage_map_face = empty_normal_direction[selected_index].face;
						break;
					}
				}

				if (new_stage_map_face == -1) {
					return 1;  // not found correct direction_num
				}
			}
			// get normal section
			else {
				new_stage_map_face = empty_normal_direction[selected_index].face;
			}
		}
	}
	// set section (un-supported direction_num checker)
	else {
		if (empty_direction_count == 0) {
			// not found empty section
			return 1;
		}

		// only 1 section
		if (empty_direction_count == 1) {
			new_stage_map_face = empty_direction[0];
		}
		// select by random
		else if (empty_direction_count > 1) {
			int selected_index = game_utils_random_gen(empty_direction_count - 1, 0);
			new_stage_map_face = empty_direction[selected_index];
		}
	}

	// set stage_map_index
	if (new_stage_map_face == STAGE_MAP_FACE_W) {
		dst_stage_map_index->x = x - 1;
		dst_stage_map_index->y = y;
	}
	else if (new_stage_map_face == STAGE_MAP_FACE_N) {
		dst_stage_map_index->x = x;
		dst_stage_map_index->y = y - 1;
	}
	else if (new_stage_map_face == STAGE_MAP_FACE_E) {
		dst_stage_map_index->x = x + 1;
		dst_stage_map_index->y = y;
	}
	else if (new_stage_map_face == STAGE_MAP_FACE_S) {
		dst_stage_map_index->x = x;
		dst_stage_map_index->y = y + 1;
	}

	return 0;
}

// stageN.dat [section]
//  (must)
//   section 1   ... N     => Normal
//   section N+1           => Boss
//
//  (optional)
//   section N+2           => Hide room
//   section N+3 ... N + i => Item room
//   section N+i+1         => Nest room
static void generate_stage_map()
{
	int ret = -1;

	// init random generator
	game_utils_random_init((unsigned int)time(NULL));

	// clear tmp region
	memset(tmp_stage_map_index, 0, sizeof(tmp_stage_map_index));
	tmp_stage_map_index_count = 0;

	// set start position
	int center_x = STAGE_MAP_WIDTH_NUM / 2;
	int center_y = STAGE_MAP_HEIGHT_NUM / 2;
	g_stage_data->current_stage_map_index = center_y * STAGE_MAP_WIDTH_NUM + center_x;
	g_stage_data->stage_map[g_stage_data->current_stage_map_index].section_id = 0;
	tmp_stage_map_index[0].x = center_x;
	tmp_stage_map_index[0].y = center_y;
	tmp_stage_map_index_count += 1;
	int normal_section_num = 1;

	int boss_section_id = -1;
	for (int i = 1; i < STAGE_MAP_WIDTH_NUM * STAGE_MAP_HEIGHT_NUM; i++) {
		bool setup_flg = false;
		if (g_stage_data->section_list[i] == NULL) break;

		if (g_stage_data->section_list[i]->section_type == SECTION_TYPE_BOSS) {
			boss_section_id = i;

			// [Boss]->[N]->[N-1]->[N-2]
			ret = set_stage_map_index(tmp_stage_map_index[normal_section_num - 1].x, tmp_stage_map_index[normal_section_num - 1].y, &tmp_stage_map_index[i], true, 1);
			if ((ret != 0) && (normal_section_num - 2 >= 0)) {
				ret = set_stage_map_index(tmp_stage_map_index[normal_section_num - 2].x, tmp_stage_map_index[normal_section_num - 2].y, &tmp_stage_map_index[i], true, 1);
			}
			if ((ret != 0) && (normal_section_num - 3 >= 0)) {
				ret = set_stage_map_index(tmp_stage_map_index[normal_section_num - 3].x, tmp_stage_map_index[normal_section_num - 3].y, &tmp_stage_map_index[i], true, 1);
			}

			if (ret == 0) {
				g_stage_data->stage_map[tmp_stage_map_index[i].y * STAGE_MAP_WIDTH_NUM + tmp_stage_map_index[i].x].section_id = i;
				tmp_stage_map_index_count += 1;
				setup_flg = true;
			}
		}
		else if (g_stage_data->section_list[i]->section_type == SECTION_TYPE_HIDE) {
			// default generator
		}
		else if (g_stage_data->section_list[i]->section_type == SECTION_TYPE_ITEM) {
			// default generator
		}
		else if (g_stage_data->section_list[i]->section_type == SECTION_TYPE_NEST) {
			// default generator
		}

		// set random point
		if (setup_flg == false) {
			int selected_index = 0;

			// set head randomly
			if (normal_section_num > 1) selected_index = game_utils_random_gen(normal_section_num - 1, 0);

			// try normal_section_num times (normal, 1 direction)
			for (int try_time = 0; try_time < normal_section_num; try_time++) {
				ret = set_stage_map_index(tmp_stage_map_index[selected_index].x, tmp_stage_map_index[selected_index].y, &tmp_stage_map_index[i], true, 1);
				if (ret == 0) break;

				selected_index += 1;
				if (selected_index >= normal_section_num) {
					selected_index = 0;
				}
			}

			// try normal_section_num times (normal, N direction)
			if (ret != 0) {
				for (int try_time = 0; try_time < normal_section_num; try_time++) {
					ret = set_stage_map_index(tmp_stage_map_index[selected_index].x, tmp_stage_map_index[selected_index].y, &tmp_stage_map_index[i], true, DIRECTION_NUM_IGNORE);
					if (ret == 0) break;

					selected_index += 1;
					if (selected_index >= normal_section_num) {
						selected_index = 0;
					}
				}
			}

			if (ret != 0) {
				LOG_ERROR("ERROR: generate_stage_map() not found empty section\n");
				return;
			}

			g_stage_data->stage_map[tmp_stage_map_index[i].y * STAGE_MAP_WIDTH_NUM + tmp_stage_map_index[i].x].section_id = i;
			tmp_stage_map_index_count += 1;
			if (g_stage_data->section_list[i]->section_type == SECTION_TYPE_NORMAL) {
				normal_section_num += 1;
			}
		}
	}
}
