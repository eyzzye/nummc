#include "game_common.h"
#include "ai_manager.h"

#include "game_window.h"
#include "game_timer.h"
#include "game_log.h"
#include "game_utils.h"
#include "map_manager.h"
#include "unit_manager.h"
#include "stage_manager.h"

#define AI_BASE_DATA_LIST_SIZE 64
static ai_data_t ai_base_data_list[AI_BASE_DATA_LIST_SIZE];
static ai_data_t* ai_base_data_list_start;
static ai_data_t* ai_base_data_list_end;
static int ai_base_data_index_end;

#define AI_DATA_LIST_SIZE 64
static ai_data_t ai_data_list[AI_DATA_LIST_SIZE];
static ai_data_t* ai_data_list_start;
static ai_data_t* ai_data_list_end;
static int ai_data_index_end;

// ai dummy
static unit_data_t dummy_unit_data;
static shape_data dummy_shape_data;

static bool within_trap(unit_data_t* unit_data, int step, float delta_x, float delta_y);

static void update_simple(ai_data_t* ai_data);
static void update_simple_fire(ai_data_t* ai_data);
static void update_left_right(ai_data_t* ai_data);
static void update_stay(ai_data_t* ai_data);
static void update_round(ai_data_t* ai_data);
static void update_go_to_bom(ai_data_t* ai_data);

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

static void reset_dummy_unit_data()
{
	memset(&dummy_unit_data, 0, sizeof(unit_data_t));
	memset(&dummy_shape_data, 0, sizeof(shape_data));
	dummy_unit_data.col_shape = &dummy_shape_data;
}

static void set_dummy_unit_data(unit_data_t* unit_data, int x, int y, float vec_x, float vec_y)
{
	reset_dummy_unit_data();

	dummy_unit_data.type = unit_data->type;
	dummy_unit_data.id = unit_data->id;
	memcpy(dummy_unit_data.col_shape, unit_data->col_shape, sizeof(shape_data));

	dummy_unit_data.col_shape->x = x;
	dummy_unit_data.col_shape->y = y;
	dummy_unit_data.col_shape->vec_x = vec_x;
	dummy_unit_data.col_shape->vec_y = vec_y;
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

int ai_manager_update(ai_data_t* ai_data)
{
	if (ai_data->type == AI_TYPE_SIMPLE) {
		update_simple(ai_data);
	}
	else if (ai_data->type == AI_TYPE_SIMPLE_FIRE) {
		update_simple_fire(ai_data);
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

	return 0;
}

static bool within_trap(unit_data_t* unit_data, int step, float delta_x, float delta_y)
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

static void ai_unit_prediction_point_single_step(unit_data_t* unit_data, int delta_time, float& x, float& y, float& vec_x, float& vec_y)
{
	// step
	x += vec_x * 1.0f / 60.0f;
	y += vec_y * 1.0f / 60.0f;

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

void ai_unit_prediction_point(unit_data_t* unit_data, int delta_time, int* p_x, int* p_y)
{
	float x = (float)PIX2MET(unit_data->col_shape->x);
	float y = (float)PIX2MET(unit_data->col_shape->y);
	float vec_x = (float)unit_data->col_shape->vec_x;
	float vec_y = (float)unit_data->col_shape->vec_y;

	int time_count = delta_time / DELTA_TIME_MIN;
	int last_time = delta_time % DELTA_TIME_MIN;

	for (int i = 0; i < time_count; i++) {
		ai_unit_prediction_point_single_step(unit_data, DELTA_TIME_MIN, x, y, vec_x, vec_y);
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

static void update_simple(ai_data_t* ai_data)
{
	ai_stat_data_t* ai_stat = (ai_stat_data_t*)ai_data;
	if (ai_stat->timer1 > 0) {
		ai_stat->timer1 -= g_delta_time;
		return;  // waitting
	}

	unit_data_t* unit_data = (unit_data_t*)ai_data->obj;
	if (unit_data->type != UNIT_TYPE_ENEMY) {
		LOG_ERROR("Error: update_simple param error.");
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
	}

	// move
	int new_step = -1;
	for (int rand_count = 0; rand_count < (AI_STAT_STEP_END - used_count); rand_count++) {
		new_step = game_utils_random_gen(AI_STAT_STEP_S, AI_STAT_STEP_N);
		if (ai_stat->step[new_step] == 0) {
			ai_stat->step[new_step] += 1;
			break;
		}
		else if (rand_count == (AI_STAT_STEP_END - used_count - 1)) {
			// last step
			for (int i = AI_STAT_STEP_N; i < AI_STAT_STEP_END; i++) {
				if (ai_stat->step[i] == 0) {
					new_step = i;
					ai_stat->step[i] += 1;
					break;
				}
			}
		}
	}

	bool move_dirt = false;
	float vec_x = 0.0f, vec_y = 0.0f;
	if (new_step == AI_STAT_STEP_N) {
		if (within_trap((unit_data_t*)ai_data->obj, new_step, 0.0f, (float)(-g_tile_height / 2)) == false) {
			vec_y = -((unit_data_t*)ai_data->obj)->col_shape->vec_y_delta;
			move_dirt = true;
		}
	}
	else if (new_step == AI_STAT_STEP_E) {
		if (within_trap((unit_data_t*)ai_data->obj, new_step, (float)(g_tile_width / 2), 0.0f) == false) {
			vec_x = ((unit_data_t*)ai_data->obj)->col_shape->vec_x_delta;
			move_dirt = true;
		}
	}
	else if (new_step == AI_STAT_STEP_W) {
		if (within_trap((unit_data_t*)ai_data->obj, new_step, (float)(-g_tile_width / 2), 0.0f) == false) {
			vec_x = -((unit_data_t*)ai_data->obj)->col_shape->vec_x_delta;
			move_dirt = true;
		}
	}
	else if (new_step == AI_STAT_STEP_S) {
		if (within_trap((unit_data_t*)ai_data->obj, new_step, 0.0f, (float)(g_tile_height / 2)) == false) {
			vec_y = ((unit_data_t*)ai_data->obj)->col_shape->vec_y_delta;
			move_dirt = true;
		}
	}

	if (move_dirt) unit_manager_enemy_move((unit_enemy_data_t*)ai_data->obj, vec_x, vec_y);

	// set wait timer
	ai_stat->timer1 = AI_SIMPLE_WAIT_TIMER;
}

static void update_simple_fire(ai_data_t* ai_data)
{
	ai_stat_data_t* ai_stat = (ai_stat_data_t*)ai_data;
	if (ai_stat->timer1 > 0) {
		ai_stat->timer1 -= g_delta_time;
		return;  // waitting
	}

	unit_data_t* unit_data = (unit_data_t*)ai_data->obj;
	if (unit_data->type != UNIT_TYPE_ENEMY) {
		LOG_ERROR("Error: update_simple_fire param error.");
	}

	// attack
	if (ai_stat->step[AI_STAT_STEP_W] == 1) {
		unit_manager_enemy_set_anim_stat(unit_data->id, ANIM_STAT_FLAG_ATTACK1);
		ai_stat->step[AI_STAT_STEP_W] += 1;
		ai_stat->timer1 = AI_SIMPLE_WAIT_TIMER;
		return;
	}
	else if (ai_stat->step[AI_STAT_STEP_E] == 1) {
		unit_manager_enemy_set_anim_stat(unit_data->id, ANIM_STAT_FLAG_ATTACK2);
		ai_stat->step[AI_STAT_STEP_E] += 1;
		ai_stat->timer1 = AI_SIMPLE_WAIT_TIMER;
		return;
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
	}

	// move
	int new_step = -1;
	for (int rand_count = 0; rand_count < (AI_STAT_STEP_END - used_count); rand_count++) {
		new_step = game_utils_random_gen(AI_STAT_STEP_S, AI_STAT_STEP_N);
		if (ai_stat->step[new_step] == 0) {
			ai_stat->step[new_step] += 1;
			break;
		}
		else if (rand_count == (AI_STAT_STEP_END - used_count - 1)) {
			// last step
			for (int i = AI_STAT_STEP_N; i < AI_STAT_STEP_END; i++) {
				if (ai_stat->step[i] == 0) {
					new_step = i;
					ai_stat->step[i] += 1;
					break;
				}
			}
		}
	}

	bool move_dirt = false;
	float vec_x = 0.0f, vec_y = 0.0f;
	if (new_step == AI_STAT_STEP_N) {
		if (within_trap((unit_data_t*)ai_data->obj, new_step, 0.0f, (float)(-g_tile_height / 2)) == false) {
			vec_y = -((unit_data_t*)ai_data->obj)->col_shape->vec_y_delta;
			move_dirt = true;
		}
	}
	else if (new_step == AI_STAT_STEP_E) {
		if (within_trap((unit_data_t*)ai_data->obj, new_step, (float)(g_tile_width / 2), 0.0f) == false) {
			vec_x = ((unit_data_t*)ai_data->obj)->col_shape->vec_x_delta;
			move_dirt = true;
		}
	}
	else if (new_step == AI_STAT_STEP_W) {
		if (within_trap((unit_data_t*)ai_data->obj, new_step, (float)(-g_tile_width / 2), 0.0f) == false) {
			vec_x = -((unit_data_t*)ai_data->obj)->col_shape->vec_x_delta;
			move_dirt = true;
		}
	}
	else if (new_step == AI_STAT_STEP_S) {
		if (within_trap((unit_data_t*)ai_data->obj, new_step, 0.0f, (float)(g_tile_height / 2)) == false) {
			vec_y = ((unit_data_t*)ai_data->obj)->col_shape->vec_y_delta;
			move_dirt = true;
		}
	}

	if (move_dirt) unit_manager_enemy_move((unit_enemy_data_t*)ai_data->obj, vec_x, vec_y);

	// set wait timer
	ai_stat->timer1 = AI_SIMPLE_WAIT_TIMER;
}

static void update_left_right(ai_data_t* ai_data)
{
	ai_stat_data_t* ai_stat = (ai_stat_data_t*)ai_data;
	if (ai_stat->timer1 > 0) {
		ai_stat->timer1 -= g_delta_time;
		return;  // waitting
	}

	unit_data_t* unit_data = (unit_data_t*)ai_data->obj;
	if (unit_data->type != UNIT_TYPE_ENEMY) {
		LOG_ERROR("Error: update_left_right param error.");
	}

	// step
	float vec_x = 0.0f, vec_y = 0.0f;
	bool move_dirt = false;
	if (ai_stat->step[AI_STAT_STEP_E] <= 1) {
		vec_x = -((unit_data_t*)ai_data->obj)->col_shape->vec_x_delta;
		move_dirt = true;
	}
	else if (ai_stat->step[AI_STAT_STEP_E] <= 2) {
		// attack left event
		unit_manager_enemy_set_anim_stat(unit_data->id, ANIM_STAT_FLAG_ATTACK1);
	}
	else if (ai_stat->step[AI_STAT_STEP_E] <= 6) {
		vec_x = ((unit_data_t*)ai_data->obj)->col_shape->vec_x_delta;
		move_dirt = true;
	}
	else if (ai_stat->step[AI_STAT_STEP_E] <= 7) {
		// attack right event
		unit_manager_enemy_set_anim_stat(unit_data->id, ANIM_STAT_FLAG_ATTACK2);
	}
	else if (ai_stat->step[AI_STAT_STEP_E] <= 9) {
		vec_x = -((unit_data_t*)ai_data->obj)->col_shape->vec_x_delta;
		move_dirt = true;
	}

	// set next step
	if (ai_stat->step[AI_STAT_STEP_E] == 9) {
		ai_stat->step[AI_STAT_STEP_E] = 0;
	}
	else {
		ai_stat->step[AI_STAT_STEP_E] += 1;
	}

	if (move_dirt) {
		unit_manager_enemy_move((unit_enemy_data_t*)ai_data->obj, vec_x, vec_y);
	}

	// set wait timer
	ai_stat->timer1 = AI_LEFT_RIGHT_WAIT_TIMER;
}

static void update_stay(ai_data_t* ai_data)
{
	ai_stat_data_t* ai_stat = (ai_stat_data_t*)ai_data;
	if (ai_stat->timer1 > 0) {
		ai_stat->timer1 -= g_delta_time;
		return;  // waitting
	}

	unit_data_t* unit_data = (unit_data_t*)ai_data->obj;
	if (unit_data->type != UNIT_TYPE_ENEMY) {
		LOG_ERROR("Error: update_stay param error.");
	}

	// step
	float vec_x = 0.0f, vec_y = 0.0f;
	if (ai_stat->step[AI_STAT_STEP_W] <= 1) {
		// do nothing
	}
	else if (ai_stat->step[AI_STAT_STEP_W] == 2) {
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

		int anim_stat_flag = ANIM_STAT_FLAG_ATTACK1;
		int bullet_index = ai_stat->bullet1;
		int enemy_face = UNIT_FACE_W;
		{
			// unit distance
			int dist_x, dist_y;
			float distance = unit_manager_get_distance(unit_data, (unit_data_t*)&g_player, &dist_x, &dist_y);

			// set attack type
			if (((face_pattern == UNIT_FACE_W) && (dist_x > 0))
				|| ((face_pattern == UNIT_FACE_E) && (dist_x <= 0))
				|| ((face_pattern == UNIT_FACE_N) && (dist_y > 0))
				|| ((face_pattern == UNIT_FACE_S) && (dist_y <= 0))) {
				anim_stat_flag = ANIM_STAT_FLAG_ATTACK2;
				bullet_index = ai_stat->bullet2;
			}

			// create dummy bullet
			std::string bullet_path = g_enemy_bullet_path[bullet_index];
			int bullet_base_id = unit_manager_search_enemy_bullet(bullet_path);
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
			float vec_x, vec_y, abs_vec = 1.0f;
			unit_manager_get_bullet_start_pos(unit_data, (unit_data_t*)bullet_data, UNIT_BULLET_NUM_SINGLE, enemy_face, &x, &y);
			unit_manager_enemy_get_face_velocity((unit_enemy_data_t*)unit_data, &vec_x, &vec_y, enemy_face, abs_vec, UNIT_BULLET_NUM_SINGLE);

			set_dummy_unit_data((unit_data_t*)bullet_data, x, y, vec_x, vec_y);
			ai_unit_prediction_point(&dummy_unit_data, 8000, &new_x, &new_y);

			int delta_x = new_x - x;
			int delta_y = new_y - y;
			float bullet_distance = sqrtf((float)(delta_x * delta_x + delta_y * delta_y));

			if (distance < bullet_distance) {
				// set attack action
				unit_manager_enemy_set_anim_stat(unit_data->id, anim_stat_flag);
				ai_stat->step[AI_STAT_STEP_W] = 0;
			}
		}
	}

	// set next step
	if (ai_stat->step[AI_STAT_STEP_W] < 2) {
		ai_stat->step[AI_STAT_STEP_W] += 1;
	}

	// set wait timer
	ai_stat->timer1 = AI_STAY_WAIT_TIMER;
}

static void update_round(ai_data_t* ai_data)
{
	ai_stat_data_t* ai_stat = (ai_stat_data_t*)ai_data;
	if (ai_stat->timer1 > 0) {
		ai_stat->timer1 -= g_delta_time;
		return;  // waitting
	}

	unit_data_t* unit_data = (unit_data_t*)ai_data->obj;
	if (unit_data->type != UNIT_TYPE_ENEMY) {
		LOG_ERROR("Error: update_round param error.");
	}

	// set wait timer
	ai_stat->timer1 = AI_STAY_WAIT_TIMER;
}

static void update_go_to_bom(ai_data_t* ai_data)
{
	ai_stat_data_t* ai_stat = (ai_stat_data_t*)ai_data;
	if (ai_stat->timer1 > 0) {
		ai_stat->timer1 -= g_delta_time;
		return;  // waitting
	}

	unit_data_t* unit_data = (unit_data_t*)ai_data->obj;
	if (unit_data->type != UNIT_TYPE_ENEMY) {
		LOG_ERROR("Error: update_go_to_bom param error.");
	}

	// step
	float vec_x = 0.0f, vec_y = 0.0f;
	int speed = -1;
	bool move_dirt = false;
	if (ai_stat->step[AI_STAT_STEP_E] == 0) {
		vec_x = -unit_data->col_shape->vec_x_delta;
		move_dirt = true;
	}
	else if (ai_stat->step[AI_STAT_STEP_E] == 1) {
		vec_x = unit_data->col_shape->vec_x_delta;
		move_dirt = true;
	}
	else if (ai_stat->step[AI_STAT_STEP_E] == 2) {
		vec_x = unit_data->col_shape->vec_x_delta;
		move_dirt = true;
	}
	else if (ai_stat->step[AI_STAT_STEP_E] == 3) {
		vec_x = -unit_data->col_shape->vec_x_delta;
		move_dirt = true;
	}
	else if (ai_stat->step[AI_STAT_STEP_E] == 4) {
		// go to player pos
		int tmp_x = g_player.col_shape->x - unit_data->col_shape->x;
		int tmp_y = g_player.col_shape->y - unit_data->col_shape->y;
		float vec_length = sqrtf((float)(tmp_x * tmp_x + tmp_y * tmp_y));

		float value = (unit_data->col_shape->vec_x_delta + unit_data->col_shape->vec_y_delta) * 0.5f;
		vec_x = value * tmp_x / vec_length;
		vec_y = value * tmp_y / vec_length;
		speed = ((unit_enemy_data_t*)unit_data)->speed + 2;

		move_dirt = true;
	}
	else if (ai_stat->step[AI_STAT_STEP_E] == 5) {
		// drop bom
		int x, y;
		unit_manager_get_spawn_items_pos_for_target(unit_data, (unit_data_t*)&g_player, 1, &x, &y);
		std::string bom_path = "units/items/bom/simple/bom.unit";
		int id = unit_manager_create_items(x, y, unit_manager_search_items(bom_path));
		unit_manager_items_set_anim_stat(id, ANIM_STAT_FLAG_ATTACK);

		// run away from player pos
		int tmp_x = g_player.col_shape->x - unit_data->col_shape->x;
		int tmp_y = g_player.col_shape->y - unit_data->col_shape->y;
		float vec_length = sqrtf((float)(tmp_x * tmp_x + tmp_y * tmp_y));

		float value = (unit_data->col_shape->vec_x_delta + unit_data->col_shape->vec_y_delta) * 0.5f;
		vec_x = - value * tmp_x / vec_length;
		vec_y = - value * tmp_y / vec_length;
		speed = ((unit_enemy_data_t*)unit_data)->speed + 3;

		move_dirt = true;
	}

	if (move_dirt) {
		unit_manager_enemy_move((unit_enemy_data_t*)ai_data->obj, vec_x, vec_y, speed);

		if (ai_stat->step[AI_STAT_STEP_E] == 5) {
			unit_manager_enemy_set_anim_stat(unit_data->id, ANIM_STAT_FLAG_ATTACK1);
		}
	}

	// set next step
	ai_stat->step[AI_STAT_STEP_E] += 1;
	if (ai_stat->step[AI_STAT_STEP_E] >= 8) {
		ai_stat->step[AI_STAT_STEP_E] = 0;
	}

	// set wait timer
	ai_stat->timer1 = AI_STAY_WAIT_TIMER;
}

void ai_manager_display() {
#ifdef AI_DEBUG
	int x, y;
	ai_unit_prediction_point((unit_data_t*)&g_player, 200, &x, &y);
	SDL_Rect rect = { VIEW_STAGE_X(x), VIEW_STAGE_Y(y), VIEW_STAGE(2), VIEW_STAGE(2) };

	SDL_SetRenderDrawColor(g_ren, 255, 0, 0, 255);
	SDL_RenderFillRect(g_ren, &rect);
#endif
}
