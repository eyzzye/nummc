#include "game_common.h"
#include "ai_manager.h"

#include "game_window.h"
#include "game_timer.h"
#include "game_log.h"
#include "game_utils.h"
#include "map_manager.h"
#include "unit_manager.h"
#include "stage_manager.h"

extern bool ai_manager_within_trap(unit_data_t* unit_data, int step, float delta_x, float delta_y);

void ai_manager_boss_update_one(ai_data_t* ai_data)
{
	ai_stat_data_t* ai_stat = (ai_stat_data_t*)ai_data;
	unit_data_t* unit_data = (unit_data_t*)ai_data->obj;
	if (unit_data->type != UNIT_TYPE_ENEMY) {
		LOG_ERROR("Error: update_simple_fire param error.");
	}

	if (ai_stat->timer1 > 0) {
		ai_stat->timer1 -= unit_manager_enemy_get_delta_time((unit_enemy_data_t*)unit_data);
		return;  // waitting
	}

	// attack (QUAD CONTINUE)
	bool attack_dirt = false;
	if ((unit_data->col_shape->face_type == UNIT_FACE_TYPE_LR) || (unit_data->col_shape->face_type == UNIT_FACE_TYPE_ALL)) {
		if (ai_stat->step[AI_STAT_STEP_W] == 4) {
			if (ai_stat->val1 & AI_PARAM_ATTACK) {
				ai_stat_bullet_t* ai_bullet = (ai_stat_bullet_t*)((unit_enemy_data_t*)unit_data)->bullet[0];
				if (ai_bullet->val1 & AI_BULLET_PARAM_CONTINUE) ai_bullet->timer2 = 0;
				attack_dirt = true;
			}
		}
		else if (ai_stat->step[AI_STAT_STEP_W] == 5) {
			if (ai_stat->val1 & AI_PARAM_ATTACK) {
				ai_stat_bullet_t* ai_bullet = (ai_stat_bullet_t*)((unit_enemy_data_t*)unit_data)->bullet[0];
				if (ai_bullet->val1 & AI_BULLET_PARAM_CONTINUE) ai_bullet->timer2 = 1;
				attack_dirt = true;
			}
		}
		else if (ai_stat->step[AI_STAT_STEP_W] == 6) {
			if (ai_stat->val1 & AI_PARAM_ATTACK) {
				ai_stat_bullet_t* ai_bullet = (ai_stat_bullet_t*)((unit_enemy_data_t*)unit_data)->bullet[0];
				if (ai_bullet->val1 & AI_BULLET_PARAM_CONTINUE) ai_bullet->timer2 = 2;
				attack_dirt = true;
			}
		}
		else if (ai_stat->step[AI_STAT_STEP_W] == 7) {
			if (ai_stat->val1 & AI_PARAM_ATTACK) {
				ai_stat_bullet_t* ai_bullet = (ai_stat_bullet_t*)((unit_enemy_data_t*)unit_data)->bullet[0];
				if (ai_bullet->val1 & AI_BULLET_PARAM_CONTINUE) ai_bullet->timer2 = 3;
				attack_dirt = true;
			}
		}

		if (attack_dirt) {
			unit_manager_enemy_set_anim_stat(unit_data->id, ANIM_STAT_FLAG_ATTACK1);
			ai_stat->step[AI_STAT_STEP_W] += 1;
			ai_stat->timer1 = AI_WAIT_TIMER_BOSS_ONE / 4;
			return;
		}
		else if (ai_stat->step[AI_STAT_STEP_W] >= 4) {
			// do next step
			ai_stat->step[AI_STAT_STEP_W] = 0;
			ai_stat->timer1 = AI_WAIT_TIMER_BOSS_ONE;
			return;
		}
	}

	if (ai_stat->step[AI_STAT_STEP_W] > 7) {
		ai_stat->step[AI_STAT_STEP_W] = 0;
		ai_stat->timer1 = AI_WAIT_TIMER_BOSS_ONE;
		return;
	}

	// detect direction
	int new_step = AI_STAT_STEP_W;
	int player_cx, player_cy, enemy_cx, enemy_cy;
	unit_manager_get_center_position((unit_data_t*)&g_player, &player_cx, &player_cy);
	unit_manager_get_center_position(unit_data, &enemy_cx, &enemy_cy);

	int tmp_x = player_cx - enemy_cx;
	int tmp_y = player_cy - enemy_cy;
	if (ABS(tmp_x) >= ABS(tmp_y)) {
		if (tmp_x <= 0) new_step = AI_STAT_STEP_W;
		else new_step = AI_STAT_STEP_E;
	}
	else {
		if (tmp_y <= 0) new_step = AI_STAT_STEP_N;
		else new_step = AI_STAT_STEP_S;
	}

	// move
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

	if (move_dirt) {
		unit_manager_enemy_move((unit_enemy_data_t*)ai_data->obj, vec_x, vec_y);
		ai_stat->step[AI_STAT_STEP_W] += 1;
	}

	// set wait timer
	ai_stat->timer1 = AI_WAIT_TIMER_BOSS_ONE;
}

void ai_manager_boss_update_two(ai_data_t* ai_data)
{

}

void ai_manager_boss_update_three(ai_data_t* ai_data)
{

}

void ai_manager_boss_update_four(ai_data_t* ai_data)
{

}
