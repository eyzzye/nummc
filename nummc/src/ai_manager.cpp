#include "game_common.h"
#include "ai_manager.h"

#include "game_window.h"
#include "game_timer.h"
#include "game_log.h"
#include "game_utils.h"
#include "map_manager.h"
#include "unit_manager.h"
#include "stage_manager.h"

#define AI_BULLET_TAG_BASIC      0
#define AI_BULLET_TAG_END        1

static const char* high_light_line_path = "units/effect/high_light_line/high_light_line.unit";
static const char* bom_path             = "units/items/bom/simple/bom.unit";

#define AI_BASE_DATA_LIST_SIZE (UNIT_ENEMY_LIST_SIZE * 3 + UNIT_ENEMY_BULLET_BASE_LIST_SIZE)
static ai_data_t ai_base_data_list[AI_BASE_DATA_LIST_SIZE];
static ai_data_t* ai_base_data_list_start;
static ai_data_t* ai_base_data_list_end;
static int ai_base_data_index_end;

#define AI_DATA_LIST_SIZE (UNIT_ENEMY_LIST_SIZE * 3 + UNIT_ENEMY_BULLET_LIST_SIZE)
static ai_data_t ai_data_list[AI_DATA_LIST_SIZE];
static ai_data_t* ai_data_list_start;
static ai_data_t* ai_data_list_end;
static int ai_data_index_end;

// ai dummy
unit_data_t ai_dummy_unit_data;
static shape_data dummy_shape_data;

static void load_basic(char* line, ai_bullet_t* bullet_data);
static void update_simple(ai_data_t* ai_data);
static void update_left_right(ai_data_t* ai_data);
static void update_stay(ai_data_t* ai_data);
static void update_round(ai_data_t* ai_data);
static void update_go_to_bom(ai_data_t* ai_data);
static void update_bullet_wave(ai_stat_bullet_t* ai_bullet);
static void update_bullet_random(ai_stat_bullet_t* ai_bullet);

// extern
bool ai_manager_within_trap(unit_data_t* unit_data, int step, float delta_x, float delta_y);
void ai_manager_get_attack_region(ai_stat_data_t* ai_stat, unit_data_t* unit_data, SDL_Rect* attack_region, float* vec_x, float* vec_y);
int ai_manager_get_player_direction(unit_data_t* unit_data);
void ai_manager_unit_prediction_point(unit_data_t* unit_data, int delta_time, int* p_x, int* p_y);
bool ai_manager_decide_attack_in_region(unit_data_t* unit_data, float abs_vec, int delta_time, int* anim_stat_flag);
int ai_manager_get_rand_direction(ai_stat_data_t* ai_stat, int used_count);
void ai_manager_move_to(unit_data_t* unit_data, int x, int y, float abs_vec);

int ai_manager_init()
{
	memset(ai_base_data_list, 0, sizeof(ai_base_data_list));
	ai_base_data_list_start = NULL;
	ai_base_data_list_end = NULL;
	ai_base_data_index_end = 0;

	memset(ai_data_list, 0, sizeof(ai_data_list));
	ai_data_list_start = NULL;
	ai_data_list_end = NULL;
	ai_data_index_end = 0;

	return 0;
}

void ai_manager_unload()
{

}

//
// ai data control functions
//
void ai_manager_copy(ai_data_t* dst, ai_data_t* src)
{
	dst->type = src->type;
	dst->val1 = src->val1;
	dst->val2 = src->val2;
	dst->val3 = src->val3;
	dst->val4 = src->val4;
}

void ai_manager_bullet_copy(ai_bullet_t* dst, ai_bullet_t* src)
{
	dst->type              = src->type;
	dst->bullet_path_index = src->bullet_path_index;
	dst->bullet_track_type = src->bullet_track_type;
	dst->bullet_num        = src->bullet_num;
	dst->bullet_face       = src->bullet_face;
	dst->val1              = src->val1;
	dst->val2              = src->val2;
	dst->val3              = src->val3;
	dst->val4              = src->val4;
}

int ai_manager_get_ai_type(char* value)
{
	int ret = AI_TYPE_NONE;
	if (STRCMP_EQ(value,"SIMPLE")) ret = AI_TYPE_SIMPLE;
	else if (STRCMP_EQ(value,"LEFT_RIGHT"))  ret = AI_TYPE_LEFT_RIGHT;
	else if (STRCMP_EQ(value,"UP_DOWN"))     ret = AI_TYPE_UP_DOWN;
	else if (STRCMP_EQ(value,"STAY"))        ret = AI_TYPE_STAY;
	else if (STRCMP_EQ(value,"FACE_ROUND"))  ret = AI_TYPE_FACE_ROUND;
	else if (STRCMP_EQ(value,"ROUND"))       ret = AI_TYPE_ROUND;
	else if (STRCMP_EQ(value,"ROUND_LR"))    ret = AI_TYPE_ROUND_LR;
	else if (STRCMP_EQ(value,"ROUND_MOVE"))  ret = AI_TYPE_ROUND_MOVE;
	else if (STRCMP_EQ(value,"RANDOM"))      ret = AI_TYPE_RANDOM;
	else if (STRCMP_EQ(value,"RANDOM_GRID")) ret = AI_TYPE_RANDOM_GRID;
	else if (STRCMP_EQ(value,"GO_TO_BOM"))   ret = AI_TYPE_GO_TO_BOM;
	// boss
	else if (STRCMP_EQ(value,"BOSS_ONE"))   ret = AI_TYPE_BOSS_ONE;
	else if (STRCMP_EQ(value,"BOSS_TWO"))   ret = AI_TYPE_BOSS_TWO;
	else if (STRCMP_EQ(value,"BOSS_THREE")) ret = AI_TYPE_BOSS_THREE;
	else if (STRCMP_EQ(value,"BOSS_FOUR"))  ret = AI_TYPE_BOSS_FOUR;
	else if (STRCMP_EQ(value,"BOSS_FIVE"))  ret = AI_TYPE_BOSS_FIVE;
	else if (STRCMP_EQ(value,"BOSS_SIX"))   ret = AI_TYPE_BOSS_SIX;
	else if (STRCMP_EQ(value,"BOSS_SEVEN")) ret = AI_TYPE_BOSS_SEVEN;
	else if (STRCMP_EQ(value,"BOSS_EIGHT")) ret = AI_TYPE_BOSS_EIGHT;
	else if (STRCMP_EQ(value,"BOSS_NINE"))  ret = AI_TYPE_BOSS_NINE;
	else if (STRCMP_EQ(value,"BOSS_X"))     ret = AI_TYPE_BOSS_X;
	else if (STRCMP_EQ(value,"BOSS_Y"))     ret = AI_TYPE_BOSS_Y;
	else if (STRCMP_EQ(value,"BOSS_Z"))     ret = AI_TYPE_BOSS_Z;

	return ret;
}


typedef struct _load_bullet_file_callback_data_t load_bullet_file_callback_data_t;
struct _load_bullet_file_callback_data_t {
	bool read_flg[AI_BULLET_TAG_END];
	ai_bullet_t* bullet_base;
};
static load_bullet_file_callback_data_t load_bullet_file_callback_data;
static void load_bullet_file_callback(char* line, int line_size, int line_num, void* argv)
{
	load_bullet_file_callback_data_t* data = (load_bullet_file_callback_data_t*)argv;

	if (line[0] == '\0') return;
	if (line[0] == '#') return;

	if (line[0] == '[') {
		if (STRCMP_EQ(line, "[basic]")) { data->read_flg[AI_BULLET_TAG_BASIC] = true; return; }
		if (STRCMP_EQ(line, "[/basic]")) { data->read_flg[AI_BULLET_TAG_BASIC] = false; return; }
	}

	if (data->read_flg[AI_BULLET_TAG_BASIC]) {
		load_basic(line, data->bullet_base);
	}
}

int ai_manager_load_bullet_file(char* path, ai_bullet_t* bullet_base)
{
	// full_path = g_base_path + "data/" + path;
	char full_path[GAME_FULL_PATH_MAX];
	int tmp_path_size = game_utils_string_cat(full_path, g_base_path, (char*)"data/", path);
	if (tmp_path_size == 0) { LOG_ERROR("ai_manager_load_bullet_file failed get %s\n", path); return 1;	}

	// read file
	memset(load_bullet_file_callback_data.read_flg, 0, sizeof(bool) * AI_BULLET_TAG_END);
	load_bullet_file_callback_data.bullet_base = bullet_base;
	int ret = game_utils_files_read_line(full_path, load_bullet_file_callback, (void*)&load_bullet_file_callback_data);
	if (ret != 0) { LOG_ERROR("ai_manager_load_bullet_file %s error\n", path); return 1; }

	return 0;
}

static void load_basic(char* line, ai_bullet_t* bullet_data)
{
	char key[GAME_UTILS_STRING_NAME_BUF_SIZE];
	char value[GAME_UTILS_STRING_CHAR_BUF_SIZE];
	game_utils_split_key_value(line, key, value);

	if (STRCMP_EQ(key, "bullet_unit")) {
		char* bullet_path = value;
		for (int i = 0; i < UNIT_BULLET_ID_END; i++) {
			if (STRCMP_EQ(bullet_path, (char*)g_enemy_bullet_path[i])) {
				((ai_bullet_t*)bullet_data)->bullet_path_index = i;
				break;
			}
		}
		return;
	}

	int bullet_track_type = UNIT_BULLET_TRACK_NONE;
	if (STRCMP_EQ(key,"bullet_track_type")) {
		char* bullet_track_str = value;
		if (STRCMP_EQ(bullet_track_str, "LINE")) {
			bullet_track_type = UNIT_BULLET_TRACK_LINE;
		}
		else if (STRCMP_EQ(bullet_track_str, "RADIAL")) {
			bullet_track_type = UNIT_BULLET_TRACK_RADIAL;
		}
		else if (STRCMP_EQ(bullet_track_str, "WAVE")) {
			bullet_track_type = UNIT_BULLET_TRACK_WAVE;
		}
		else if (STRCMP_EQ(bullet_track_str, "CROSS")) {
			bullet_track_type = UNIT_BULLET_TRACK_CROSS;
		}
		else if (STRCMP_EQ(bullet_track_str, "XCROSS")) {
			bullet_track_type = UNIT_BULLET_TRACK_XCROSS;
		}
		else if (STRCMP_EQ(bullet_track_str, "RANDOM")) {
			bullet_track_type = UNIT_BULLET_TRACK_RANDOM;
		}
		bullet_data->bullet_track_type = bullet_track_type;
		return;
	}

	int bullet_num = UNIT_BULLET_NUM_NONE;
	if (STRCMP_EQ(key,"bullet_num")) {
		char* bullet_path = value;
		if (STRCMP_EQ(bullet_path, "SINGLE")) {
			bullet_num = UNIT_BULLET_NUM_SINGLE;
		}
		else if (STRCMP_EQ(bullet_path, "DOUBLE")) {
			bullet_num = UNIT_BULLET_NUM_DOUBLE;
		}
		else if (STRCMP_EQ(bullet_path, "TRIPLE")) {
			bullet_num = UNIT_BULLET_NUM_TRIPLE;
		}
		else if (STRCMP_EQ(bullet_path, "QUADRUPLE")) {
			bullet_num = UNIT_BULLET_NUM_QUADRUPLE;
		}
		else {
			bullet_num = atoi(bullet_path);
		}
		bullet_data->bullet_num = bullet_num;
		return;
	}

	int bullet_face = UNIT_FACE_NONE;
	if (STRCMP_EQ(key,"bullet_face")) {
		char* bullet_face_str = value;
		if (STRCMP_EQ(bullet_face_str, "N")) {
			bullet_face = UNIT_FACE_N;
		}
		else if (STRCMP_EQ(bullet_face_str, "E")) {
			bullet_face = UNIT_FACE_E;
		}
		else if (STRCMP_EQ(bullet_face_str, "W")) {
			bullet_face = UNIT_FACE_W;
		}
		else if (STRCMP_EQ(bullet_face_str, "S")) {
			bullet_face = UNIT_FACE_S;
		}
		bullet_data->bullet_face = bullet_face;
		return;
	}

	if (STRCMP_EQ(key,"val1")) {
		bullet_data->val1 = atoi(value);
		return;
	}
	else if (STRCMP_EQ(key,"val2")) {
		bullet_data->val2 = atoi(value);
		return;
	}
	else if (STRCMP_EQ(key,"val3")) {
		bullet_data->val3 = atoi(value);
		return;
	}
	else if (STRCMP_EQ(key,"val4")) {
		bullet_data->val4 = atoi(value);
		return;
	}
}

void ai_manager_delete_ai_data(ai_data_t* delete_data)
{
	ai_data_t* tmp1 = delete_data->prev;
	ai_data_t* tmp2 = delete_data->next;
	if (tmp1) tmp1->next = tmp2;
	if (tmp2) tmp2->prev = tmp1;

	if (delete_data == ai_data_list_start) ai_data_list_start = tmp2;
	if (delete_data == ai_data_list_end) ai_data_list_end = tmp1;

	memset(delete_data, 0, sizeof(ai_data_t));
}

int ai_manager_delete_ghost(ai_data_t* ai_data)
{
	// delete ai ghost
	int ghost_id = ((ai_stat_data_t*)ai_data)->ghost_id;
	if (ghost_id != UNIT_TRAP_ID_IGNORE) {
		unit_manager_clear_trap(unit_manager_get_trap(ghost_id));
		((ai_stat_data_t*)ai_data)->ghost_id = UNIT_TRAP_ID_IGNORE;
	}
	return 0;
}

static void reset_dummy_unit_data()
{
	memset(&ai_dummy_unit_data, 0, sizeof(unit_data_t));
	memset(&dummy_shape_data, 0, sizeof(shape_data));
	ai_dummy_unit_data.col_shape = &dummy_shape_data;
}

void ai_manager_set_dummy_unit_data(unit_data_t* unit_data, int x, int y, float vec_x, float vec_y)
{
	reset_dummy_unit_data();

	ai_dummy_unit_data.type = unit_data->type;
	ai_dummy_unit_data.id = unit_data->id;
	memcpy(ai_dummy_unit_data.col_shape, unit_data->col_shape, sizeof(shape_data));

	ai_dummy_unit_data.col_shape->x = x;
	ai_dummy_unit_data.col_shape->y = y;
	ai_dummy_unit_data.col_shape->vec_x = vec_x;
	ai_dummy_unit_data.col_shape->vec_y = vec_y;
}

ai_data_t* ai_manager_new_ai_base_data()
{
	ai_data_t* ai_base_data = NULL;

	int start_index = ai_base_data_index_end % AI_BASE_DATA_LIST_SIZE;
	int new_index = -1;
	for (int i = 0; i < AI_BASE_DATA_LIST_SIZE; i++) {
		int index = start_index + i;
		if (index >= AI_BASE_DATA_LIST_SIZE) index -= AI_BASE_DATA_LIST_SIZE;
		if (ai_base_data_list[index].type == AI_TYPE_NONE) {
			new_index = index;
			ai_base_data_index_end += i;
			break;
		}
	}
	if (new_index == -1) {
		LOG_ERROR("ai_manager_new_ai_base_data error\n");
		return NULL;
	}

	ai_base_data = &ai_base_data_list[new_index];
	ai_base_data->id = ai_base_data_index_end;

	// set prev, next
	ai_base_data->prev = ai_base_data_list_end;
	ai_base_data->next = NULL;
	if (ai_base_data_list_end) ai_base_data_list_end->next = ai_base_data;
	ai_base_data_list_end = ai_base_data;

	// only first node
	if (ai_base_data_list_start == NULL) {
		ai_base_data->prev = NULL;
		ai_base_data_list_start = ai_base_data;
	}

	ai_base_data_index_end += 1;
	return ai_base_data;
}

ai_data_t* ai_manager_new_ai_data()
{
	ai_data_t* ai_data = NULL;

	int start_index = ai_data_index_end % AI_DATA_LIST_SIZE;
	int new_index = -1;
	for (int i = 0; i < AI_DATA_LIST_SIZE; i++) {
		int index = start_index + i;
		if (index >= AI_DATA_LIST_SIZE) index -= AI_DATA_LIST_SIZE;
		if (ai_data_list[index].type == AI_TYPE_NONE) {
			new_index = index;
			ai_data_index_end += i;
			break;
		}
	}
	if (new_index == -1) {
		LOG_ERROR("ai_manager_new_ai_data error\n");
		return NULL;
	}

	ai_data = &ai_data_list[new_index];
	ai_data->id = ai_data_index_end;

	// set prev, next
	ai_data->prev = ai_data_list_end;
	ai_data->next = NULL;
	if (ai_data_list_end) ai_data_list_end->next = ai_data;
	ai_data_list_end = ai_data;

	// only first node
	if (ai_data_list_start == NULL) {
		ai_data->prev = NULL;
		ai_data_list_start = ai_data;
	}

	ai_data_index_end += 1;
	return ai_data;
}

//
// ai I/F functions
//
int ai_manager_spawn(ai_data_t* ai_data)
{
	int ret = 0;
	if (ai_data->type == AI_TYPE_BOSS_SIX) {
		ret = ai_manager_boss_spawn_six(ai_data);
	}
	else if (ai_data->type == AI_TYPE_BOSS_EIGHT) {
		ret = ai_manager_boss_spawn_eight(ai_data);
	}
	return ret;
}

int ai_manager_stop(ai_data_t* ai_data)
{
	int ret = 0;
	if (ai_data->type == AI_TYPE_BOSS_THREE) {
		ret = ai_manager_boss_stop_three(ai_data);
	}
	else if (ai_data->type == AI_TYPE_BOSS_FOUR) {
		ret = ai_manager_boss_stop_four(ai_data);
	}
	return ret;
}

int ai_manager_update(ai_data_t* ai_data)
{
	if (ai_data->type == AI_TYPE_SIMPLE) {
		update_simple(ai_data);
	}
	else if (ai_data->type == AI_TYPE_LEFT_RIGHT) {
		update_left_right(ai_data);
	}
	else if (ai_data->type == AI_TYPE_UP_DOWN) {

	}
	else if (ai_data->type == AI_TYPE_STAY) {
		update_stay(ai_data);
	}
	else if (ai_data->type == AI_TYPE_ROUND) {
		update_round(ai_data);
	}
	else if (ai_data->type == AI_TYPE_GO_TO_BOM) {
		update_go_to_bom(ai_data);
	}
	// boss
	else if (ai_data->type == AI_TYPE_BOSS_ONE) {
		ai_manager_boss_update_one(ai_data);
	}
	else if (ai_data->type == AI_TYPE_BOSS_TWO) {
		ai_manager_boss_update_two(ai_data);
	}
	else if (ai_data->type == AI_TYPE_BOSS_THREE) {
		ai_manager_boss_update_three(ai_data);
	}
	else if (ai_data->type == AI_TYPE_BOSS_FOUR) {
		ai_manager_boss_update_four(ai_data);
	}
	else if (ai_data->type == AI_TYPE_BOSS_FIVE) {
		ai_manager_boss_update_five(ai_data);
	}
	else if (ai_data->type == AI_TYPE_BOSS_SIX) {
		ai_manager_boss_update_six(ai_data);
	}
	else if (ai_data->type == AI_TYPE_BOSS_SEVEN) {
		ai_manager_boss_update_seven(ai_data);
	}
	else if (ai_data->type == AI_TYPE_BOSS_EIGHT) {
		ai_manager_boss_update_eight(ai_data);
	}
	else if (ai_data->type == AI_TYPE_BOSS_NINE) {
		ai_manager_boss_update_nine(ai_data);
	}
	else if (ai_data->type == AI_TYPE_BOSS_X) {
		ai_manager_boss_update_x(ai_data);
	}

	return 0;
}

int ai_manager_bullet_update(ai_data_t* ai_data)
{
	if (ai_data == NULL) return 0;

	if (ai_data->type == AI_TYPE_BULLET) {
		ai_stat_bullet_t* ai_bullet = (ai_stat_bullet_t*)ai_data;
		if (ai_bullet->bullet_track_type == UNIT_BULLET_TRACK_WAVE) {
			update_bullet_wave(ai_bullet);
		}
		if (ai_bullet->bullet_track_type == UNIT_BULLET_TRACK_RANDOM) {
			update_bullet_random(ai_bullet);
		}
	}
	return 0;
}

//
// ai common functions
//
bool ai_manager_within_trap(unit_data_t* unit_data, int step, float delta_x, float delta_y)
{
	float float_x = 0.0f, float_y = 0.0f;
	if (unit_data->col_shape->type == COLLISION_TYPE_BOX_D) {
		int w =((shape_box_data*)unit_data->col_shape)->w;
		int h = ((shape_box_data*)unit_data->col_shape)->h;
		unit_data->col_shape->y;

		if (step == AI_STAT_STEP_N) {
			float_x = unit_data->col_shape->x + unit_data->col_shape->offset_x + w / 2 + delta_x;
			float_y = unit_data->col_shape->y + delta_y;
		}
		else if (step == AI_STAT_STEP_S) {
			float_x = unit_data->col_shape->x + unit_data->col_shape->offset_x + w / 2 + delta_x;
			float_y = unit_data->col_shape->y + unit_data->col_shape->offset_y + h     + delta_y;
		}
		else if (step == AI_STAT_STEP_W) {
			float_x = unit_data->col_shape->x + delta_x;
			float_y = unit_data->col_shape->y + unit_data->col_shape->offset_y + h / 2 + delta_y;
		}
		else if (step == AI_STAT_STEP_E) {
			float_x = unit_data->col_shape->x + unit_data->col_shape->offset_x + w     + delta_x;
			float_y = unit_data->col_shape->y + unit_data->col_shape->offset_y + h / 2 + delta_y;
		}
	}
	else if (unit_data->col_shape->type == COLLISION_TYPE_ROUND_D) {
		int r = ((shape_round_data*)unit_data->col_shape)->r;
		if (step == AI_STAT_STEP_N) {
			float_x = unit_data->col_shape->x     + delta_x;
			float_y = unit_data->col_shape->y - r + delta_y;
		}
		else if (step == AI_STAT_STEP_S) {
			float_x = unit_data->col_shape->x     + delta_x;
			float_y = unit_data->col_shape->y + r + delta_y;
		}
		else if (step == AI_STAT_STEP_W) {
			float_x = unit_data->col_shape->x - r + delta_x;
			float_y = unit_data->col_shape->y     + delta_y;
		}
		else if (step == AI_STAT_STEP_E) {
			float_x = unit_data->col_shape->x + r + delta_x;
			float_y = unit_data->col_shape->y     + delta_y;
		}
	}
	else {
		LOG_ERROR("Error: ai in_trap() get irregular type.");
	}

	return unit_manager_trap_within((int)float_x, (int)float_y);
}

// ai_stat->val2: attack_region_width
// ai_stat->timer2: decision_count
void ai_manager_get_attack_region(ai_stat_data_t* ai_stat, unit_data_t* unit_data, SDL_Rect* attack_region, float* vec_x, float* vec_y)
{
	int attack_region_width = ai_stat->val2;
	int attack_speed = 3;
	int player_cx, player_cy;
	unit_manager_get_center_position((unit_data_t*)&g_player, &player_cx, &player_cy);

	int player_w, player_h;
	if (g_player.col_shape->type == COLLISION_TYPE_BOX_D) {
		player_w = ((shape_box_data*)g_player.col_shape)->w / 2;
		player_h = ((shape_box_data*)g_player.col_shape)->h / 2;
	}
	else if (g_player.col_shape->type == COLLISION_TYPE_ROUND_D) {
		player_w = ((shape_round_data*)g_player.col_shape)->r;
		player_h = ((shape_round_data*)g_player.col_shape)->r;
	}

	int pos_x, pos_y;
	unit_manager_get_position(unit_data, &pos_x, &pos_y);

	if (unit_data->col_shape->type == COLLISION_TYPE_BOX_D) {
		int w, h;
		w = ((shape_box_data*)unit_data->col_shape)->w;
		h = ((shape_box_data*)unit_data->col_shape)->h;
		pos_x += unit_data->col_shape->offset_x;
		pos_y += unit_data->col_shape->offset_y;

		if (unit_data->col_shape->face == UNIT_FACE_N) {
			attack_region->x = pos_x - attack_region_width / 2 - player_w;
			attack_region->y = 0;
			attack_region->w = attack_region_width + w + 2 * player_w;
			attack_region->h = pos_y;
			*vec_y = -((unit_data_t*)ai_stat->obj)->col_shape->vec_y_delta * attack_speed;
		}
		else if (unit_data->col_shape->face == UNIT_FACE_S) {
			attack_region->x = pos_x - attack_region_width / 2 - player_w;
			attack_region->y = pos_y + w;
			attack_region->w = attack_region_width + w + 2 * player_w;
			attack_region->h = g_map_y_max * g_tile_height - attack_region->y;
			*vec_y = ((unit_data_t*)ai_stat->obj)->col_shape->vec_y_delta * attack_speed;
		}
		else if (unit_data->col_shape->face == UNIT_FACE_E) {
			attack_region->x = pos_x + w;
			attack_region->y = pos_y - attack_region_width / 2 - player_h;
			attack_region->w = g_map_x_max * g_tile_width - attack_region->x;
			attack_region->h = attack_region_width + h + 2 * player_h;
			*vec_x = ((unit_data_t*)ai_stat->obj)->col_shape->vec_x_delta * attack_speed;
		}
		else { // (unit_data->col_shape->face == UNIT_FACE_W)
			attack_region->x = 0;
			attack_region->y = pos_y - attack_region_width / 2 - player_h;
			attack_region->w = pos_x;
			attack_region->h = attack_region_width + h + 2 * player_h;
			*vec_x = -((unit_data_t*)ai_stat->obj)->col_shape->vec_x_delta * attack_speed;
		}
	}
	else if (unit_data->col_shape->type == COLLISION_TYPE_ROUND_D) {
		int r = ((shape_round_data*)unit_data->col_shape)->r;
		pos_x += unit_data->col_shape->offset_x;
		pos_y += unit_data->col_shape->offset_y;

		if (unit_data->col_shape->face == UNIT_FACE_N) {
			attack_region->x = pos_x - attack_region_width / 2 - r - player_w;
			attack_region->y = 0;
			attack_region->w = attack_region_width + 2 * r + 2 * player_w;
			attack_region->h = pos_y - r;
			*vec_y = -((unit_data_t*)ai_stat->obj)->col_shape->vec_y_delta * attack_speed;
		}
		else if (unit_data->col_shape->face == UNIT_FACE_S) {
			attack_region->x = pos_x - attack_region_width / 2 - r - player_w;
			attack_region->y = pos_y + r;
			attack_region->w = attack_region_width + 2 * r + 2 * player_w;
			attack_region->h = g_map_y_max * g_tile_height - attack_region->y;
			*vec_y = ((unit_data_t*)ai_stat->obj)->col_shape->vec_y_delta * attack_speed;
		}
		else if (unit_data->col_shape->face == UNIT_FACE_E) {
			attack_region->x = pos_x + r;
			attack_region->y = pos_y - attack_region_width / 2 - r - player_h;
			attack_region->w = g_map_x_max * g_tile_width - attack_region->x;
			attack_region->h = attack_region_width + 2 * r + 2 * player_h;
			*vec_x = ((unit_data_t*)ai_stat->obj)->col_shape->vec_x_delta * attack_speed;
		}
		else { // (unit_data->col_shape->face == UNIT_FACE_W)
			attack_region->x = 0;
			attack_region->y = pos_y - attack_region_width / 2 - r - player_h;
			attack_region->w = pos_x - r;
			attack_region->h = attack_region_width + 2 * r + 2 * player_h;
			*vec_x = -((unit_data_t*)ai_stat->obj)->col_shape->vec_x_delta * attack_speed;
		}
	}

	// AABB decision
	if (((attack_region->x < player_cx) && (player_cx < attack_region->x + attack_region->w)) &&
		((attack_region->y < player_cy) && (player_cy < attack_region->y + attack_region->h)))
	{
		ai_stat->timer2 += 1;
	}
	else {
		ai_stat->timer2 = 0;
	}
}

int ai_manager_get_player_direction(unit_data_t* unit_data)
{
	int direction_face = AI_STAT_STEP_W;
	int player_cx, player_cy, enemy_cx, enemy_cy;
	unit_manager_get_center_position((unit_data_t*)&g_player, &player_cx, &player_cy);
	unit_manager_get_center_position(unit_data, &enemy_cx, &enemy_cy);

	int tmp_x = player_cx - enemy_cx;
	int tmp_y = player_cy - enemy_cy;
	if (ABS(tmp_x) >= ABS(tmp_y)) {
		if (tmp_x <= 0) direction_face = AI_STAT_STEP_W;
		else direction_face = AI_STAT_STEP_E;
	}
	else {
		if (tmp_y <= 0) direction_face = AI_STAT_STEP_N;
		else direction_face = AI_STAT_STEP_S;
	}
	return direction_face;
}

static void ai_unit_prediction_point_single_step(unit_data_t* unit_data, int delta_time, float& x, float& y, float& vec_x, float& vec_y)
{
	// step
	x += vec_x * (g_delta_time / ONE_FRAME_TIME) * 1.0f / 60.0f;
	y += vec_y * (g_delta_time / ONE_FRAME_TIME) * 1.0f / 60.0f;

	// friction
	if ((unit_data->type == UNIT_TYPE_PLAYER) || (unit_data->type == UNIT_TYPE_ENEMY) || (unit_data->type == UNIT_TYPE_ITEMS)) {
		float abs_vec_x = ABS(vec_x);
		if (abs_vec_x > 0) {
			float delta_vec_x = vec_x > 0 ? -unit_data->col_shape->vec_x_delta : unit_data->col_shape->vec_x_delta;
			vec_x += (g_stage_data->friction_coef * delta_vec_x * g_delta_time);
			if (ABS(vec_x) < FLOAT_NEAR_ZERO) vec_x = 0.0f;
		}

		float abs_vec_y = ABS(vec_y);
		if (abs_vec_y > 0) {
			float delta_vec_y = vec_y > 0 ? -unit_data->col_shape->vec_y_delta : unit_data->col_shape->vec_y_delta;
			vec_y += (g_stage_data->friction_coef * delta_vec_y * g_delta_time);
			if (ABS(vec_y) < FLOAT_NEAR_ZERO) vec_y = 0.0f;
		}
	}
}

void ai_manager_unit_prediction_point(unit_data_t* unit_data, int delta_time, int* p_x, int* p_y)
{
	float x = (float)PIX2MET(unit_data->col_shape->x);
	float y = (float)PIX2MET(unit_data->col_shape->y);
	float vec_x = (float)unit_data->col_shape->vec_x;
	float vec_y = (float)unit_data->col_shape->vec_y;

	int time_count = delta_time / ONE_FRAME_TIME;
	int last_time = delta_time % ONE_FRAME_TIME;

	for (int i = 0; i < time_count; i++) {
		ai_unit_prediction_point_single_step(unit_data, ONE_FRAME_TIME, x, y, vec_x, vec_y);
	}

	if (last_time > 0) {
		ai_unit_prediction_point_single_step(unit_data, last_time, x, y, vec_x, vec_y);
	}

	if (unit_data->col_shape->type & COLLISION_TYPE_BOX) {
		int w = ((shape_box_data*)(unit_data->col_shape))->w;
		int h = ((shape_box_data*)(unit_data->col_shape))->h;
		*p_x = (int)MET2PIX(x) + unit_data->col_shape->offset_x + w / 2;
		*p_y = (int)MET2PIX(y) + unit_data->col_shape->offset_y + h / 2;
	}
	else if (unit_data->col_shape->type & COLLISION_TYPE_ROUND) {
		*p_x = (int)MET2PIX(x);
		*p_y = (int)MET2PIX(y);
	}
}

// anim_stat_flag:
//  [P]          = player
//  [E][3][w][m] = enemy(W,E,N,S)
//------------------------------------------
// face = W
//  [P]<[E]     = ANIM_STAT_FLAG_ATTACK1
//      [E]>[P] = ANIM_STAT_FLAG_ATTACK2
//------------------------------------------
// face = E
//  [P]<[3]     = ANIM_STAT_FLAG_ATTACK2
//      [3]>[P] = ANIM_STAT_FLAG_ATTACK1
//------------------------------------------
// face = N
//      [P]     = ANIM_STAT_FLAG_ATTACK1
//       ^
//      [w]
//      [w]     = ANIM_STAT_FLAG_ATTACK2
//       v
//      [P]
//------------------------------------------
// face = S
//      [P]     = ANIM_STAT_FLAG_ATTACK2
//       ^
//      [m]
//      [m]     = ANIM_STAT_FLAG_ATTACK1
//       v
//      [P]
//------------------------------------------
bool ai_manager_decide_attack_in_region(unit_data_t* unit_data, float abs_vec, int delta_time, int* anim_stat_flag)
{
	bool attack_dirt = false;
	float distance, bullet_distance;

	// set pattern
	int face_pattern = UNIT_FACE_W;
	if (unit_data->col_shape->face_type == UNIT_FACE_TYPE_ALL) {
		face_pattern = unit_data->col_shape->face;
	}
	else if (unit_data->col_shape->face_type == UNIT_FACE_TYPE_LR) {
		if (unit_data->col_shape->face == UNIT_FACE_E) {
			face_pattern = UNIT_FACE_E;
		}
	}
	else if (unit_data->col_shape->face_type == UNIT_FACE_TYPE_UD) {
		if (unit_data->col_shape->face == UNIT_FACE_S) {
			face_pattern = UNIT_FACE_S;
		}
	}

	ai_stat_bullet_t* ai_stat_bullet = (ai_stat_bullet_t*)((unit_enemy_data_t*)unit_data)->bullet[0];
	int bullet_index = ai_stat_bullet->bullet_path_index;
	int enemy_face = UNIT_FACE_W;

	// unit distance
	int dist_x, dist_y;
	distance = unit_manager_get_distance(unit_data, (unit_data_t*)&g_player, &dist_x, &dist_y);

	// set attack type
	if (((face_pattern == UNIT_FACE_W) && (dist_x > 0))
		|| ((face_pattern == UNIT_FACE_E) && (dist_x <= 0))
		|| ((face_pattern == UNIT_FACE_N) && (dist_y > 0))
		|| ((face_pattern == UNIT_FACE_S) && (dist_y <= 0))) {
		*anim_stat_flag = ANIM_STAT_FLAG_ATTACK2;
		ai_stat_bullet = (ai_stat_bullet_t*)((unit_enemy_data_t*)unit_data)->bullet[1];
		bullet_index = ai_stat_bullet->bullet_path_index;
	}

	// create dummy bullet
	int bullet_base_id = unit_manager_search_enemy_bullet((char*)g_enemy_bullet_path[bullet_index]);
	unit_enemy_bullet_data_t* bullet_data = unit_manager_get_enemy_bullet_base(bullet_base_id);

	// set bullet direction
	if ((face_pattern == UNIT_FACE_W) || (face_pattern == UNIT_FACE_E)) {
		if (dist_x > 0) enemy_face = UNIT_FACE_E;
	}
	else if ((face_pattern == UNIT_FACE_N) || (face_pattern == UNIT_FACE_S)) {
		if (dist_y > 0) enemy_face = UNIT_FACE_S;
		else enemy_face = UNIT_FACE_N;
	}

	int x, y, new_x, new_y;
	float vec_x, vec_y;
	int bullet_track_type = UNIT_BULLET_TRACK_LINE; // dummy
	int bullet_num = UNIT_BULLET_NUM_SINGLE;		// dummy
	unit_manager_get_bullet_start_pos(unit_data, (unit_data_t*)bullet_data, bullet_track_type, bullet_num, enemy_face, &x, &y);
	unit_manager_enemy_get_face_velocity((unit_enemy_data_t*)unit_data, &vec_x, &vec_y, enemy_face, abs_vec, bullet_track_type, bullet_num);

	ai_manager_set_dummy_unit_data((unit_data_t*)bullet_data, x, y, vec_x, vec_y);
	ai_manager_unit_prediction_point(&ai_dummy_unit_data, delta_time, &new_x, &new_y);

	int delta_x = new_x - x;
	int delta_y = new_y - y;
	bullet_distance = sqrtf((float)(delta_x * delta_x + delta_y * delta_y));

	if (distance < bullet_distance) {
		attack_dirt = true;
	}

	return attack_dirt;
}

// ai_stat->step[rand_direction] = 0 -> 1
int ai_manager_get_rand_direction(ai_stat_data_t* ai_stat, int used_count)
{
	int rand_direction = AI_STAT_STEP_W;
	for (int rand_count = 0; rand_count < (AI_STAT_STEP_END - used_count); rand_count++) {
		rand_direction = game_utils_random_gen(AI_STAT_STEP_S, AI_STAT_STEP_N);
		if (ai_stat->step[rand_direction] == 0) {
			ai_stat->step[rand_direction] += 1;
			break;
		}
		else if (rand_count == (AI_STAT_STEP_END - used_count - 1)) {
			// last step
			for (int i = AI_STAT_STEP_N; i < AI_STAT_STEP_END; i++) {
				if (ai_stat->step[i] == 0) {
					rand_direction = i;
					ai_stat->step[i] += 1;
					break;
				}
			}
		}
	}
	return rand_direction;
}

void ai_manager_move_to(unit_data_t* unit_data, int x, int y, float abs_vec)
{
	float delta_x = (float)x - unit_data->col_shape->float_x;
	float delta_y = (float)y - unit_data->col_shape->float_y;
	float length = sqrtf(delta_x * delta_x + delta_y * delta_y);
	if (length <= 0.0f) return; // arrived already

	unit_data->col_shape->float_x = unit_data->col_shape->float_x + abs_vec * delta_x / length;
	unit_data->col_shape->float_y = unit_data->col_shape->float_y + abs_vec * delta_y / length;
	unit_data->col_shape->x = (int)unit_data->col_shape->float_x;
	unit_data->col_shape->y = (int)unit_data->col_shape->float_y;
	b2Vec2 new_pos = { PIX2MET(unit_data->col_shape->x), PIX2MET(unit_data->col_shape->y) };
	unit_data->col_shape->b2body->SetTransform(new_pos, 0.0f);
}

//
// ai update sub functions
//
static void update_simple(ai_data_t* ai_data)
{
	ai_stat_data_t* ai_stat = (ai_stat_data_t*)ai_data;
	unit_data_t* unit_data = (unit_data_t*)ai_data->obj;
	if (unit_data->type != UNIT_TYPE_ENEMY) {
		LOG_ERROR("Error: update_simple_fire param error.");
	}

	// slope_attack
	if (ai_stat->val1 & AI_PARAM_SLOPE_ATTACK) {
		int decision_count = ai_stat->val3;
		float vec_x = 0.0f, vec_y = 0.0f;
		SDL_Rect attack_region = { 0 };

		if (unit_data->anim->stat == ANIM_STAT_FLAG_ATTACK1) {
			// keep moving
			return;
		}
		else {
			ai_manager_get_attack_region(ai_stat, unit_data, &attack_region, &vec_x, &vec_y);
		}

		// slope_attack
		if (ai_stat->timer2 > decision_count) {
			((unit_enemy_data_t*)unit_data)->resistance_stat |= UNIT_EFFECT_FLAG_E_NO_FRICTION;
			unit_data->col_shape->vec_x_max = ABS(vec_x);
			unit_data->col_shape->vec_y_max = ABS(vec_y);
			unit_manager_enemy_move((unit_enemy_data_t*)ai_data->obj, vec_x, vec_y);
			unit_manager_enemy_set_anim_stat(unit_data->id, ANIM_STAT_FLAG_ATTACK1);

			// create high_light_line
			if ((unit_data->col_shape->face == UNIT_FACE_N) || (unit_data->col_shape->face == UNIT_FACE_S)) {
				int unit_id = unit_manager_create_effect(attack_region.x, 0, unit_manager_search_effect((char*)high_light_line_path));
				unit_effect_data_t* effect_data = unit_manager_get_effect(unit_id);
				((shape_box_data*)effect_data->col_shape)->w = attack_region.w;
				((shape_box_data*)effect_data->col_shape)->h = g_map_y_max * g_tile_height;
			}
			else { // (unit_data->col_shape->face == UNIT_FACE_E) || (unit_data->col_shape->face == UNIT_FACE_W)
				int unit_id = unit_manager_create_effect(0, attack_region.y, unit_manager_search_effect((char*)high_light_line_path));
				unit_effect_data_t* effect_data = unit_manager_get_effect(unit_id);
				((shape_box_data*)effect_data->col_shape)->w = g_map_x_max * g_tile_width;
				((shape_box_data*)effect_data->col_shape)->h = attack_region.h;
			}

			// reset timer
			ai_stat->timer1 = AI_WAIT_TIMER_SIMPLE;
			ai_stat->timer2 = 0;
			return;
		}
		else if (((unit_enemy_data_t*)unit_data)->resistance_stat & UNIT_EFFECT_FLAG_E_NO_FRICTION) {
			((unit_enemy_data_t*)unit_data)->resistance_stat &= ~UNIT_EFFECT_FLAG_E_NO_FRICTION;
			unit_data->col_shape->vec_x_max = unit_data->base->col_shape->vec_x_max;
			unit_data->col_shape->vec_y_max = unit_data->base->col_shape->vec_y_max;
		}
	}

	if (ai_stat->timer1 > 0) {
		ai_stat->timer1 -= unit_manager_enemy_get_delta_time((unit_enemy_data_t*)unit_data);
		return;  // waitting
	}

	// attack
	if ((unit_data->col_shape->face_type == UNIT_FACE_TYPE_LR) || (unit_data->col_shape->face_type == UNIT_FACE_TYPE_ALL)) {
		if (ai_stat->step[AI_STAT_STEP_W] == 1) {
			if (ai_stat->val1 & AI_PARAM_ATTACK) {
				unit_manager_enemy_set_anim_stat(unit_data->id, ANIM_STAT_FLAG_ATTACK1);
				ai_stat->step[AI_STAT_STEP_W] += 1;
				ai_stat->timer1 = AI_WAIT_TIMER_SIMPLE;
				return;
			}
			else {
				// do next step
				ai_stat->step[AI_STAT_STEP_W] += 1;
			}
		}
		else if (ai_stat->step[AI_STAT_STEP_E] == 1) {
			if (ai_stat->val1 & AI_PARAM_ATTACK) {
				unit_manager_enemy_set_anim_stat(unit_data->id, ANIM_STAT_FLAG_ATTACK1);
				ai_stat->step[AI_STAT_STEP_E] += 1;
				ai_stat->timer1 = AI_WAIT_TIMER_SIMPLE;
				return;
			}
			else {
				// do next step
				ai_stat->step[AI_STAT_STEP_E] += 1;
			}
		}
	}

	if ((unit_data->col_shape->face_type == UNIT_FACE_TYPE_UD) || (unit_data->col_shape->face_type == UNIT_FACE_TYPE_ALL)) {
		if (ai_stat->step[AI_STAT_STEP_N] == 1) {
			if (ai_stat->val1 & AI_PARAM_ATTACK) {
				unit_manager_enemy_set_anim_stat(unit_data->id, ANIM_STAT_FLAG_ATTACK1);
				ai_stat->step[AI_STAT_STEP_N] += 1;
				ai_stat->timer1 = AI_WAIT_TIMER_SIMPLE;
				return;
			}
			else {
				// do next step
				ai_stat->step[AI_STAT_STEP_N] += 1;
			}
		}
		else if (ai_stat->step[AI_STAT_STEP_S] == 1) {
			if (ai_stat->val1 & AI_PARAM_ATTACK) {
				unit_manager_enemy_set_anim_stat(unit_data->id, ANIM_STAT_FLAG_ATTACK1);
				ai_stat->step[AI_STAT_STEP_S] += 1;
				ai_stat->timer1 = AI_WAIT_TIMER_SIMPLE;
				return;
			}
			else {
				// do next step
				ai_stat->step[AI_STAT_STEP_S] += 1;
			}
		}
	}

	int used_count = 0;
	for (int i = AI_STAT_STEP_N; i < AI_STAT_STEP_END; i++) {
		if (ai_stat->step[i]) used_count += 1;
	}

	if (used_count == AI_STAT_STEP_END) {
		ai_stat->step[AI_STAT_STEP_N] = 0;
		ai_stat->step[AI_STAT_STEP_E] = 0;
		ai_stat->step[AI_STAT_STEP_W] = 0;
		ai_stat->step[AI_STAT_STEP_S] = 0;
		used_count = 0;
		//ai_stat->timer1 = AI_WAIT_TIMER_SIMPLE;
		return;
	}

	// move
	int new_step = ai_manager_get_rand_direction(ai_stat, used_count);
	bool move_dirt = false;
	float vec_x = 0.0f, vec_y = 0.0f;
	if (new_step == AI_STAT_STEP_N) {
		if (ai_manager_within_trap((unit_data_t*)ai_data->obj, new_step, 0.0f, (float)(-g_tile_height / 2)) == false) {
			vec_y = -((unit_data_t*)ai_data->obj)->col_shape->vec_y_delta;
			move_dirt = true;
		}
	}
	else if (new_step == AI_STAT_STEP_E) {
		if (ai_manager_within_trap((unit_data_t*)ai_data->obj, new_step, (float)(g_tile_width / 2), 0.0f) == false) {
			vec_x = ((unit_data_t*)ai_data->obj)->col_shape->vec_x_delta;
			move_dirt = true;
		}
	}
	else if (new_step == AI_STAT_STEP_W) {
		if (ai_manager_within_trap((unit_data_t*)ai_data->obj, new_step, (float)(-g_tile_width / 2), 0.0f) == false) {
			vec_x = -((unit_data_t*)ai_data->obj)->col_shape->vec_x_delta;
			move_dirt = true;
		}
	}
	else if (new_step == AI_STAT_STEP_S) {
		if (ai_manager_within_trap((unit_data_t*)ai_data->obj, new_step, 0.0f, (float)(g_tile_height / 2)) == false) {
			vec_y = ((unit_data_t*)ai_data->obj)->col_shape->vec_y_delta;
			move_dirt = true;
		}
	}

	if (move_dirt) unit_manager_enemy_move((unit_enemy_data_t*)ai_data->obj, vec_x, vec_y);

	// set wait timer
	ai_stat->timer1 = AI_WAIT_TIMER_SIMPLE;
}

static void update_left_right(ai_data_t* ai_data)
{
	unit_data_t* unit_data = (unit_data_t*)ai_data->obj;
	if (unit_data->type != UNIT_TYPE_ENEMY) {
		LOG_ERROR("Error: update_left_right param error.");
	}

	ai_stat_data_t* ai_stat = (ai_stat_data_t*)ai_data;
	if (ai_stat->timer1 > 0) {
		ai_stat->timer1 -= unit_manager_enemy_get_delta_time((unit_enemy_data_t*)unit_data);
		return;  // waitting
	}

	// attack
	if ((ai_stat->step[AI_STAT_STEP_W] == 2) || (ai_stat->step[AI_STAT_STEP_W] == 7)) {
		if (ai_stat->val1 & AI_PARAM_ATTACK) {
			unit_manager_enemy_set_anim_stat(unit_data->id, ANIM_STAT_FLAG_ATTACK1);
			ai_stat->step[AI_STAT_STEP_W] += 1;
			ai_stat->timer1 = AI_WAIT_TIMER_LEFT_RIGHT;
			return;
		}
		else {
			// do next step
			ai_stat->step[AI_STAT_STEP_W] += 1;
		}
	}

	// step
	float vec_x = 0.0f, vec_y = 0.0f;
	bool move_dirt = false;
	if (ai_stat->step[AI_STAT_STEP_W] <= 1) {
		vec_x = -((unit_data_t*)ai_data->obj)->col_shape->vec_x_delta;
		move_dirt = true;
	}
	else if (ai_stat->step[AI_STAT_STEP_W] <= 6) {
		vec_x = ((unit_data_t*)ai_data->obj)->col_shape->vec_x_delta;
		move_dirt = true;
	}
	else if (ai_stat->step[AI_STAT_STEP_W] <= 9) {
		vec_x = -((unit_data_t*)ai_data->obj)->col_shape->vec_x_delta;
		move_dirt = true;
	}

	// set next step
	if (ai_stat->step[AI_STAT_STEP_W] == 9) {
		ai_stat->step[AI_STAT_STEP_W] = 0;
	}
	else {
		ai_stat->step[AI_STAT_STEP_W] += 1;
	}

	if (move_dirt) {
		unit_manager_enemy_move((unit_enemy_data_t*)ai_data->obj, vec_x, vec_y);
	}

	// set wait timer
	ai_stat->timer1 = AI_WAIT_TIMER_LEFT_RIGHT;
}

static void update_stay(ai_data_t* ai_data)
{
	unit_data_t* unit_data = (unit_data_t*)ai_data->obj;
	if (unit_data->type != UNIT_TYPE_ENEMY) {
		LOG_ERROR("Error: update_stay param error.");
	}

	ai_stat_data_t* ai_stat = (ai_stat_data_t*)ai_data;
	if (ai_stat->timer1 > 0) {
		ai_stat->timer1 -= unit_manager_enemy_get_delta_time((unit_enemy_data_t*)unit_data);
		return;  // waitting
	}

	// attack
	if ((ai_stat->step[AI_STAT_STEP_W] == 1) || (ai_stat->step[AI_STAT_STEP_W] == 3)) {
		if (ai_stat->val1 & AI_PARAM_ATTACK) {

			bool attack_dirt = false;
			ai_stat_bullet_t* ai_stat_bullet = (ai_stat_bullet_t*)((unit_enemy_data_t*)unit_data)->bullet[0];
			int anim_stat_flag = ANIM_STAT_FLAG_ATTACK1;

			if (ai_stat->val1 & AI_PARAM_ALWAYS) {
				if (ai_stat->step[AI_STAT_STEP_W] == 1) {
					//ai_stat_bullet = (ai_stat_bullet_t*)((unit_enemy_data_t*)unit_data)->bullet[0];
					//anim_stat_flag = ANIM_STAT_FLAG_ATTACK1;
					attack_dirt = true;
				}
				else if (ai_stat->step[AI_STAT_STEP_W] == 3) {
					ai_stat_bullet = (ai_stat_bullet_t*)((unit_enemy_data_t*)unit_data)->bullet[1];
					anim_stat_flag = ANIM_STAT_FLAG_ATTACK2;
					attack_dirt = true;
				}

			} else if (ai_stat->val1 & AI_PARAM_IN_REGION) {
				attack_dirt = ai_manager_decide_attack_in_region(unit_data, 1.0f, 5120, &anim_stat_flag);
			}

			if (attack_dirt) {
				// set attack action
				unit_manager_enemy_set_anim_stat(unit_data->id, anim_stat_flag);
				ai_stat->step[AI_STAT_STEP_W] += 1;
				if (ai_stat->step[AI_STAT_STEP_W] > 3) ai_stat->step[AI_STAT_STEP_W] = 0;
				ai_stat->timer1 = AI_WAIT_TIMER_STAY;
				return;
			}
		}
	}

	// step
	float vec_x = 0.0f, vec_y = 0.0f;
	if (ai_stat->step[AI_STAT_STEP_W] == 0) {
		ai_stat->step[AI_STAT_STEP_W] += 1;
	}
	else if (ai_stat->step[AI_STAT_STEP_W] == 2) {
		ai_stat->step[AI_STAT_STEP_W] += 1;
	}

	// set wait timer
	ai_stat->timer1 = AI_WAIT_TIMER_STAY;
}

static void update_round(ai_data_t* ai_data)
{
	unit_data_t* unit_data = (unit_data_t*)ai_data->obj;
	if (unit_data->type != UNIT_TYPE_ENEMY) {
		LOG_ERROR("Error: update_round param error.");
	}

	ai_stat_data_t* ai_stat = (ai_stat_data_t*)ai_data;
	if (ai_stat->timer1 > 0) {
		ai_stat->timer1 -= unit_manager_enemy_get_delta_time((unit_enemy_data_t*)unit_data);
		return;  // waitting
	}

	// step
	if (ai_stat->step[AI_STAT_STEP_W] == 0) {
		if ((ai_stat->val1 & AI_PARAM_ATTACK) && (ai_stat->val1 & AI_PARAM_ALWAYS)) {
			if (unit_data->col_shape->b2body) {
				ai_bullet_t* bullet = (ai_bullet_t*)((unit_enemy_data_t*)unit_data)->bullet[0];
				float angle = unit_data->col_shape->b2body->GetAngle();
				int round_count = (int)(angle / (2.0f * b2_pi));
				angle -= round_count * (2.0f * b2_pi);

				bool attack_dirt = false;
				int attack_count = ai_stat->val2;
				if ((g_stage_data->daytime_stat == STAGE_DAYTIME_STAT_LATE_NIGHT) && (ai_stat->val1 & AI_PARAM_NIGHT_MODE)) {
					attack_count = 4; // nightmare
				}

				if (((attack_count == 1) || (attack_count == 2) || (attack_count == 4))
					&& (angle >= 0) && (angle < b2_pi * 5.0f / 180.0f)) {
					attack_dirt = true;
				}
				else if ((attack_count == 4)
					&& (angle >= b2_pi * 90.0f / 180.0f) && (angle < b2_pi * 95.0f / 180.0f)) {
					attack_dirt = true;
				}
				else if (((attack_count == 2) || (attack_count == 4))
					&& (angle >= b2_pi * 180.0f / 180.0f) && (angle < b2_pi * 185.0f / 180.0f)) {
					attack_dirt = true;
				}
				else if ((attack_count == 4)
					&& (angle >= b2_pi * 270.0f / 180.0f) && (angle < b2_pi * 275.0f / 180.0f)) {
					attack_dirt = true;
				}

				if (attack_dirt) {
					unit_manager_enemy_set_anim_stat(unit_data->id, ANIM_STAT_FLAG_ATTACK1);
					ai_stat->step[AI_STAT_STEP_W] += 1;
				}
				else {
					// don't reset timer
					return;
				}
			}
		}
	}
	else if (ai_stat->step[AI_STAT_STEP_W] == 1) {
		ai_stat->step[AI_STAT_STEP_W] = 0;
	}

	// set wait timer
	ai_stat->timer1 = AI_WAIT_TIMER_ROUND;
}

static void update_go_to_bom(ai_data_t* ai_data)
{
	unit_data_t* unit_data = (unit_data_t*)ai_data->obj;
	if (unit_data->type != UNIT_TYPE_ENEMY) {
		LOG_ERROR("Error: update_go_to_bom param error.");
	}

	ai_stat_data_t* ai_stat = (ai_stat_data_t*)ai_data;
	if (ai_stat->timer1 > 0) {
		ai_stat->timer1 -= unit_manager_enemy_get_delta_time((unit_enemy_data_t*)unit_data);
		return;  // waitting
	}

	// step
	float vec_x = 0.0f, vec_y = 0.0f;
	int speed = -1;
	bool move_dirt = false;
	if (ai_stat->step[AI_STAT_STEP_W] == 0) {
		vec_x = -unit_data->col_shape->vec_x_delta;
		move_dirt = true;
	}
	else if (ai_stat->step[AI_STAT_STEP_W] == 1) {
		vec_x = unit_data->col_shape->vec_x_delta;
		move_dirt = true;
	}
	else if (ai_stat->step[AI_STAT_STEP_W] == 2) {
		vec_x = unit_data->col_shape->vec_x_delta;
		move_dirt = true;
	}
	else if (ai_stat->step[AI_STAT_STEP_W] == 3) {
		vec_x = -unit_data->col_shape->vec_x_delta;
		move_dirt = true;
	}
	else if (ai_stat->step[AI_STAT_STEP_W] == 4) {
		// go to player pos
		int player_cx, player_cy, enemy_cx, enemy_cy;
		unit_manager_get_center_position((unit_data_t*)&g_player, &player_cx, &player_cy);
		unit_manager_get_center_position(unit_data, &enemy_cx, &enemy_cy);
		int tmp_x = player_cx - enemy_cx;
		int tmp_y = player_cy - enemy_cy;
		float vec_length = sqrtf((float)(tmp_x * tmp_x + tmp_y * tmp_y));

		float value = (unit_data->col_shape->vec_x_delta + unit_data->col_shape->vec_y_delta) * 0.5f;
		vec_x = value * tmp_x / vec_length;
		vec_y = value * tmp_y / vec_length;
		speed = ((unit_enemy_data_t*)unit_data)->speed + 2;

		move_dirt = true;
	}
	else if (ai_stat->step[AI_STAT_STEP_W] == 5) {
		// drop bom
		int x, y;
		unit_manager_get_spawn_items_pos_for_target(unit_data, (unit_data_t*)&g_player, 1, &x, &y);
		int id = unit_manager_create_items(x, y, unit_manager_search_items((char*)bom_path));
		unit_manager_items_set_anim_stat(id, ANIM_STAT_FLAG_ATTACK);

		// run away from player pos
		int player_cx, player_cy, enemy_cx, enemy_cy;
		unit_manager_get_center_position((unit_data_t*)&g_player, &player_cx, &player_cy);
		unit_manager_get_center_position(unit_data, &enemy_cx, &enemy_cy);
		int tmp_x = player_cx - enemy_cx;
		int tmp_y = player_cy - enemy_cy;
		float vec_length = sqrtf((float)(tmp_x * tmp_x + tmp_y * tmp_y));

		float value = (unit_data->col_shape->vec_x_delta + unit_data->col_shape->vec_y_delta) * 0.5f;
		vec_x = - value * tmp_x / vec_length;
		vec_y = - value * tmp_y / vec_length;
		speed = ((unit_enemy_data_t*)unit_data)->speed + 3;

		move_dirt = true;
	}

	if (move_dirt) {
		unit_manager_enemy_move((unit_enemy_data_t*)ai_data->obj, vec_x, vec_y, speed);

		if (ai_stat->step[AI_STAT_STEP_W] == 5) {
			unit_manager_enemy_set_anim_stat(unit_data->id, ANIM_STAT_FLAG_ATTACK1);
		}
	}

	// set next step
	ai_stat->step[AI_STAT_STEP_W] += 1;
	if (ai_stat->step[AI_STAT_STEP_W] >= 8) {
		ai_stat->step[AI_STAT_STEP_W] = 0;
	}

	// set wait timer
	ai_stat->timer1 = AI_WAIT_TIMER_GO_TO_BOM;
}

//
// ai bullet update sub functions
//
static void update_bullet_wave(ai_stat_bullet_t* ai_bullet) {
	unit_enemy_bullet_data_t* unit_enemy_bullet = (unit_enemy_bullet_data_t*)ai_bullet->obj;
	int face = unit_enemy_bullet->col_shape->face;

	int BULLET_WAVE_TIMER_TERM1 = ai_bullet->val2;
	int BULLET_WAVE_TIMER_TERM2 = ai_bullet->val3;
	int BULLET_WAVE_TIMER_MAX = BULLET_WAVE_TIMER_TERM1 + BULLET_WAVE_TIMER_TERM2;

	int delta_val = ai_bullet->val1; // wave width
	if (ai_bullet->timer1 < BULLET_WAVE_TIMER_TERM1) {
		delta_val = delta_val / (BULLET_WAVE_TIMER_TERM1 / ONE_FRAME_TIME);
	}
	else {
		delta_val = -delta_val / (BULLET_WAVE_TIMER_TERM2 / ONE_FRAME_TIME);
	}

	if ((g_stage_data->section_circumstance & SECTION_CIRCUMSTANCE_FLAG_SLOWED_ENEMY) || (unit_enemy_bullet->effect_stat & UNIT_EFFECT_FLAG_E_FREEZE_UP)) {
		delta_val >>= 1; // div 2
	}

	bool move_dirt = false;
	b2Vec2 new_point = unit_enemy_bullet->col_shape->b2body->GetPosition();
	if (face == UNIT_FACE_N) {
		unit_enemy_bullet->col_shape->x += delta_val;
		new_point.x += PIX2MET(delta_val);
		move_dirt = true;
	}
	else if (face == UNIT_FACE_S) {
		unit_enemy_bullet->col_shape->x += delta_val;
		new_point.x += PIX2MET(delta_val);
		move_dirt = true;
	}
	else if (face == UNIT_FACE_W) {
		unit_enemy_bullet->col_shape->y += delta_val;
		new_point.y += PIX2MET(delta_val);
		move_dirt = true;
	}
	else if (face == UNIT_FACE_E) {
		unit_enemy_bullet->col_shape->y += delta_val;
		new_point.y += PIX2MET(delta_val);
		move_dirt = true;
	}

	if (move_dirt) {
		unit_enemy_bullet->col_shape->b2body->SetTransform(new_point, 0);
		//unit_enemy_bullet->col_shape->x = (int)MET2PIX(unit_enemy_bullet->col_shape->b2body->GetPosition().x);
		//unit_enemy_bullet->col_shape->y = (int)MET2PIX(unit_enemy_bullet->col_shape->b2body->GetPosition().y);
	}

	if (ai_bullet->timer1 < BULLET_WAVE_TIMER_MAX) {
		ai_bullet->timer1 += unit_manager_enemy_bullet_get_delta_time(unit_enemy_bullet);
	}
	else {
		ai_bullet->timer1 = 0;
	}
}

static void update_bullet_random(ai_stat_bullet_t* ai_bullet) {
	unit_enemy_bullet_data_t* unit_enemy_bullet = (unit_enemy_bullet_data_t*)ai_bullet->obj;
	b2Vec2 new_vec = unit_enemy_bullet->col_shape->b2body->GetLinearVelocity();
	float fall_vec = (float)ai_bullet->val1 / 1000.0f;

	if ((g_stage_data->section_circumstance & SECTION_CIRCUMSTANCE_FLAG_SLOWED_ENEMY) || (unit_enemy_bullet->effect_stat & UNIT_EFFECT_FLAG_E_FREEZE_UP)) {
		fall_vec *= 0.5f;
	}
	new_vec.y += fall_vec;

	unit_enemy_bullet->col_shape->b2body->SetLinearVelocity(new_vec);
	unit_enemy_bullet->col_shape->vec_x = unit_enemy_bullet->col_shape->b2body->GetLinearVelocity().x;
	unit_enemy_bullet->col_shape->vec_y = unit_enemy_bullet->col_shape->b2body->GetLinearVelocity().y;
}

//
// ai debug functions
//
void ai_manager_display() {
#ifdef AI_DEBUG
	int x, y;
	ai_unit_prediction_point((unit_data_t*)&g_player, 200, &x, &y);
	SDL_Rect rect = { VIEW_STAGE_X(x), VIEW_STAGE_Y(y), VIEW_STAGE(2), VIEW_STAGE(2) };

	SDL_SetRenderDrawColor(g_ren, 255, 0, 0, 255);
	SDL_RenderFillRect(g_ren, &rect);
#endif
}
