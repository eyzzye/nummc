#include "game_common.h"
#include "ai_manager.h"

#include "game_window.h"
#include "game_timer.h"
#include "game_log.h"
#include "game_utils.h"
#include "map_manager.h"
#include "unit_manager.h"
#include "stage_manager.h"
#include "resource_manager.h"
#include "sound_manager.h"

// ai dummy
extern unit_data_t ai_dummy_unit_data;

extern void ai_manager_set_dummy_unit_data(unit_data_t* unit_data, int x, int y, float vec_x, float vec_y);
extern bool ai_manager_within_trap(unit_data_t* unit_data, int step, float delta_x, float delta_y);
extern void ai_manager_get_attack_region(ai_stat_data_t* ai_stat, unit_data_t* unit_data, SDL_Rect* attack_region, float* vec_x, float* vec_y);
extern int ai_manager_get_player_direction(unit_data_t* unit_data);
extern void ai_manager_unit_prediction_point(unit_data_t* unit_data, int delta_time, int* p_x, int* p_y);
extern bool ai_manager_decide_attack_in_region(unit_data_t* unit_data, float abs_vec, int delta_time, int* anim_stat_flag);
extern int ai_manager_get_rand_direction(ai_stat_data_t* ai_stat, int used_count);
extern void ai_manager_move_to(unit_data_t* unit_data, int x, int y, float abs_vec);

static std::string enemy_shadow_path[ANIM_BASE_SIZE_END] = {
	"units/effect/shadow/shadow.unit",
	"units/effect/shadow/48x48/shadow.unit",
	"units/effect/shadow/64x64/shadow.unit",
};
static std::string enemy_shadow_drop_path[ANIM_BASE_SIZE_END] = {
	"units/effect/shadow/shadow_drop.unit",
	"units/effect/shadow/48x48/shadow_drop.unit",
	"units/effect/shadow/64x64/shadow_drop.unit",
};

static std::string high_light_line_path  = "units/effect/high_light_line/high_light_line.unit";
static std::string ghost_trap_three_path = "units/trap/ghost/3/boss/ghost.unit";
static std::string ghost_trap_four_path  = "units/trap/ghost/4/boss/ghost.unit";
static std::string fall_bom_path         = "units/items/bom/fall/fall_bom.unit";
static std::string mini_boss_six_path    = "units/enemy/6/mini_boss/six.unit";
static std::string mini_enemy_eight_path = "units/enemy/8/lv2/eight.unit";

void ai_manager_boss_update_one(ai_data_t* ai_data)
{
	ai_stat_data_t* ai_stat = (ai_stat_data_t*)ai_data;
	unit_data_t* unit_data = (unit_data_t*)ai_data->obj;
	if (unit_data->type != UNIT_TYPE_ENEMY) {
		LOG_ERROR("Error: update_one param error.");
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

	// move
	int new_step = ai_manager_get_player_direction(unit_data);
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
	unit_data_t* unit_data = (unit_data_t*)ai_data->obj;
	if (unit_data->type != UNIT_TYPE_ENEMY) {
		LOG_ERROR("Error: update_two param error.");
	}

	ai_stat_data_t* ai_stat = (ai_stat_data_t*)ai_data;
	if (ai_stat->timer1 > 0) {
		ai_stat->timer1 -= unit_manager_enemy_get_delta_time((unit_enemy_data_t*)unit_data);
		return;  // waitting
	}

	// attack
	if ((ai_stat->step[AI_STAT_STEP_W] == 2) || (ai_stat->step[AI_STAT_STEP_W] == 8)) {
		if (ai_stat->val1 & AI_PARAM_ATTACK) {
			ai_stat_bullet_t* ai_bullet = (ai_stat_bullet_t*)((unit_enemy_data_t*)unit_data)->bullet[0];
			if (ai_bullet->val1 & AI_BULLET_PARAM_TARGET) {
				unit_manager_get_center_position((unit_data_t*)&g_player, &ai_bullet->val2, &ai_bullet->val3);
				ai_bullet->val4 = ai_manager_get_player_direction(unit_data);
			}

			unit_manager_enemy_set_anim_stat(unit_data->id, ANIM_STAT_FLAG_ATTACK1);
			ai_stat->step[AI_STAT_STEP_W] += 1;
			ai_stat->timer1 = AI_WAIT_TIMER_BOSS_TWO;
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
	else if (ai_stat->step[AI_STAT_STEP_W] <= 7) {
		vec_x = ((unit_data_t*)ai_data->obj)->col_shape->vec_x_delta;
		move_dirt = true;
	}
	else if (ai_stat->step[AI_STAT_STEP_W] <= 11) {
		vec_x = -((unit_data_t*)ai_data->obj)->col_shape->vec_x_delta;
		move_dirt = true;
	}

	// set next step
	if (ai_stat->step[AI_STAT_STEP_W] == 11) {
		ai_stat->step[AI_STAT_STEP_W] = 0;
	}
	else {
		ai_stat->step[AI_STAT_STEP_W] += 1;
	}

	if (move_dirt) {
		unit_manager_enemy_move((unit_enemy_data_t*)ai_data->obj, vec_x, vec_y);
	}

	// set wait timer
	ai_stat->timer1 = AI_WAIT_TIMER_BOSS_TWO;
}

int ai_manager_boss_stop_three(ai_data_t* ai_data)
{
	return ai_manager_delete_ghost(ai_data);
}

// ai_stat->val3: move_to_step
static int boss_three_target[3][2];
void ai_manager_boss_update_three(ai_data_t* ai_data)
{
	unit_data_t* unit_data = (unit_data_t*)ai_data->obj;
	if (unit_data->type != UNIT_TYPE_ENEMY) {
		LOG_ERROR("Error: update_three param error.");
	}

	ai_stat_data_t* ai_stat = (ai_stat_data_t*)ai_data;
	if ((5 <= ai_stat->step[AI_STAT_STEP_W]) && (ai_stat->step[AI_STAT_STEP_W] <= 8)) {
		if (ai_stat->step[AI_STAT_STEP_W] == 5) {
			ai_manager_move_to(unit_data, boss_three_target[0][0], boss_three_target[0][1], 2.0f);
		}
		else if (ai_stat->step[AI_STAT_STEP_W] == 6) {
			ai_manager_move_to(unit_data, boss_three_target[1][0], boss_three_target[1][1], 2.0f);
		}
		else if (ai_stat->step[AI_STAT_STEP_W] == 7) {
			ai_manager_move_to(unit_data, boss_three_target[2][0], boss_three_target[2][1], 2.0f);
		}

		b2Vec2 trap_new_pos = { PIX2MET(unit_data->col_shape->x), PIX2MET(unit_data->col_shape->y) };
		unit_trap_data_t* trap_data = unit_manager_get_trap(ai_stat->ghost_id);
		trap_data->col_shape->b2body->SetTransform(trap_new_pos, 0.0f);
		trap_data->col_shape->x = unit_data->col_shape->x;
		trap_data->col_shape->y = unit_data->col_shape->y;
	}

	if (ai_stat->timer1 > 0) {
		ai_stat->timer1 -= unit_manager_enemy_get_delta_time((unit_enemy_data_t*)unit_data);
		return;  // waitting
	}

	// attack
	if ((ai_stat->step[AI_STAT_STEP_W] == 1) || (ai_stat->step[AI_STAT_STEP_W] == 2)) {
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
				else if (ai_stat->step[AI_STAT_STEP_W] == 2) {
					ai_stat_bullet = (ai_stat_bullet_t*)((unit_enemy_data_t*)unit_data)->bullet[1];
					anim_stat_flag = ANIM_STAT_FLAG_ATTACK2;
					attack_dirt = true;
				}

			}
			else if (ai_stat->val1 & AI_PARAM_IN_REGION) {
				attack_dirt = ai_manager_decide_attack_in_region(unit_data, 1.0f, 5120, &anim_stat_flag);
			}

			if (attack_dirt) {
				// set attack action
				unit_manager_enemy_set_anim_stat(unit_data->id, anim_stat_flag);
				ai_stat->step[AI_STAT_STEP_W] += 1;
				ai_stat->timer1 = AI_WAIT_TIMER_BOSS_THREE;
				return;
			}
		}
	}

	// step
	float vec_x = 0.0f, vec_y = 0.0f;
	if (ai_stat->step[AI_STAT_STEP_W] == 0) {
		ai_stat->step[AI_STAT_STEP_W] += 1;
		ai_stat->timer1 = AI_WAIT_TIMER_BOSS_THREE;
		return;
	}
	else if (ai_stat->step[AI_STAT_STEP_W] == 3) {
		// stay
		ai_stat->step[AI_STAT_STEP_W] += 1;
		ai_stat->timer1 = AI_WAIT_TIMER_BOSS_THREE;
		return;
	}
	else if (ai_stat->step[AI_STAT_STEP_W] == 4) {
		// move (open)
		collision_manager_delete_joint(unit_data->col_shape);
		unit_manager_enemy_set_anim_stat(unit_data->id, ANIM_STAT_FLAG_MOVE);
		collision_manager_set_filter(unit_data->col_shape, COLLISION_GROUP_MASK_PLAYER_BULLET);

		// create trap (enemy ghost)
		((unit_enemy_data_t*)unit_data)->resistance_stat |= UNIT_EFFECT_FLAG_E_NO_TRAP_DAMAGE;
		ai_stat->ghost_id = unit_manager_create_trap(unit_data->col_shape->x, unit_data->col_shape->y, unit_manager_search_trap(ghost_trap_three_path));
		unit_trap_data_t* trap_data = unit_manager_get_trap(ai_stat->ghost_id);
		collision_manager_set_filter(trap_data->col_shape, COLLISION_GROUP_MASK_PLAYER_BULLET);

		// set move_to point
		if (ai_stat->val3 == 0) { // W
			boss_three_target[0][0] = unit_data->col_shape->x - 128;
			boss_three_target[0][1] = unit_data->col_shape->y;
			boss_three_target[1][0] = unit_data->col_shape->x - 64;
			boss_three_target[1][1] = unit_data->col_shape->y + (int)(128.0f * sinf(b2_pi * 60.0f / 180.0f));
			boss_three_target[2][0] = unit_data->col_shape->x;
			boss_three_target[2][1] = unit_data->col_shape->y;
			ai_stat->val3 += 1;
		}
		else if (ai_stat->val3 == 1) { // S
			boss_three_target[0][0] = unit_data->col_shape->x;
			boss_three_target[0][1] = unit_data->col_shape->y + 128;
			boss_three_target[1][0] = unit_data->col_shape->x + (int)(128.0f * sinf(b2_pi * 60.0f / 180.0f));
			boss_three_target[1][1] = unit_data->col_shape->y + 64;
			boss_three_target[2][0] = unit_data->col_shape->x;
			boss_three_target[2][1] = unit_data->col_shape->y;
			ai_stat->val3 += 1;
		}
		else if (ai_stat->val3 == 2) { // E
			boss_three_target[0][0] = unit_data->col_shape->x + 128;
			boss_three_target[0][1] = unit_data->col_shape->y;
			boss_three_target[1][0] = unit_data->col_shape->x + 64;
			boss_three_target[1][1] = unit_data->col_shape->y - (int)(128.0f * sinf(b2_pi * 60.0f / 180.0f));
			boss_three_target[2][0] = unit_data->col_shape->x;
			boss_three_target[2][1] = unit_data->col_shape->y;
			ai_stat->val3 += 1;
		}
		else if (ai_stat->val3 == 3) { // N
			boss_three_target[0][0] = unit_data->col_shape->x;
			boss_three_target[0][1] = unit_data->col_shape->y - 128;
			boss_three_target[1][0] = unit_data->col_shape->x - (int)(128.0f * sinf(b2_pi * 60.0f / 180.0f));
			boss_three_target[1][1] = unit_data->col_shape->y - 64;
			boss_three_target[2][0] = unit_data->col_shape->x;
			boss_three_target[2][1] = unit_data->col_shape->y;
			ai_stat->val3 = 0;
		}

		unit_data->col_shape->float_x = (float)unit_data->col_shape->x;
		unit_data->col_shape->float_y = (float)unit_data->col_shape->y;

		ai_stat->step[AI_STAT_STEP_W] += 1;
		ai_stat->timer1 = AI_WAIT_TIMER_BOSS_THREE;
		return;
	}
	else if (ai_stat->step[AI_STAT_STEP_W] == 5) {
		ai_stat->step[AI_STAT_STEP_W] += 1;
		ai_stat->timer1 = AI_WAIT_TIMER_BOSS_THREE;
		return;
	}
	else if (ai_stat->step[AI_STAT_STEP_W] == 6) {
		ai_stat->step[AI_STAT_STEP_W] += 1;
		ai_stat->timer1 = AI_WAIT_TIMER_BOSS_THREE;
		return;
	}
	else if (ai_stat->step[AI_STAT_STEP_W] == 7) {
		ai_stat->step[AI_STAT_STEP_W] += 1;
		ai_stat->timer1 = AI_WAIT_TIMER_BOSS_THREE;
		return;
	}
	else if (ai_stat->step[AI_STAT_STEP_W] > 7) {
		// stay (fix)
		collision_manager_set_joint((void*)unit_data);
		collision_manager_set_filter(unit_data->col_shape, (COLLISION_GROUP_MASK_PLAYER | COLLISION_GROUP_MASK_ITEMS | COLLISION_GROUP_MASK_PLAYER_BULLET | COLLISION_GROUP_MASK_MAP));

		// delete trap (enemy ghost)
		ai_manager_delete_ghost(ai_data);
		((unit_enemy_data_t*)unit_data)->resistance_stat &= ~UNIT_EFFECT_FLAG_E_NO_TRAP_DAMAGE;

		unit_data->col_shape->vec_x = 0.0f;
		unit_data->col_shape->vec_y = 0.0f;
		b2Vec2 new_vec((float32)unit_data->col_shape->vec_x, (float32)unit_data->col_shape->vec_y);
		unit_data->col_shape->b2body->SetLinearVelocity(new_vec);
		unit_manager_enemy_set_anim_stat(unit_data->id, ANIM_STAT_FLAG_IDLE);

		ai_stat->step[AI_STAT_STEP_W] = 0;
	}

	// set wait timer
	ai_stat->timer1 = AI_WAIT_TIMER_BOSS_THREE;
}

int ai_manager_boss_stop_four(ai_data_t* ai_data)
{
	return ai_manager_delete_ghost(ai_data);
}

static int boss_four_target[2][2];
void ai_manager_boss_update_four(ai_data_t* ai_data)
{
	unit_data_t* unit_data = (unit_data_t*)ai_data->obj;
	if (unit_data->type != UNIT_TYPE_ENEMY) {
		LOG_ERROR("Error: update_four param error.");
	}

	ai_stat_data_t* ai_stat = (ai_stat_data_t*)ai_data;
	if ((5 <= ai_stat->step[AI_STAT_STEP_W]) && (ai_stat->step[AI_STAT_STEP_W] <= 6)) {
		if (ai_stat->step[AI_STAT_STEP_W] == 5) {
			ai_manager_move_to(unit_data, boss_four_target[0][0], boss_four_target[0][1], 3.0f);
		}
		if (ai_stat->step[AI_STAT_STEP_W] == 6) {
			ai_manager_move_to(unit_data, boss_four_target[1][0], boss_four_target[1][1], 3.0f);
		}

		b2Vec2 trap_new_pos = { PIX2MET(unit_data->col_shape->x), PIX2MET(unit_data->col_shape->y) };
		unit_trap_data_t* trap_data = unit_manager_get_trap(ai_stat->ghost_id);
		trap_data->col_shape->b2body->SetTransform(trap_new_pos, 0.0f);
		trap_data->col_shape->x = unit_data->col_shape->x;
		trap_data->col_shape->y = unit_data->col_shape->y;
	}

	if (ai_stat->timer1 > 0) {
		ai_stat->timer1 -= unit_manager_enemy_get_delta_time((unit_enemy_data_t*)unit_data);
		return;  // waitting
	}

	// attack
	if ((ai_stat->step[AI_STAT_STEP_W] == 1) || (ai_stat->step[AI_STAT_STEP_W] == 2)) {
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
				else if (ai_stat->step[AI_STAT_STEP_W] == 2) {
					ai_stat_bullet = (ai_stat_bullet_t*)((unit_enemy_data_t*)unit_data)->bullet[1];
					anim_stat_flag = ANIM_STAT_FLAG_ATTACK2;
					attack_dirt = true;
				}
			}

			if (attack_dirt) {
				// set attack action
				unit_manager_enemy_set_anim_stat(unit_data->id, anim_stat_flag);
				ai_stat->step[AI_STAT_STEP_W] += 1;
				ai_stat->timer1 = AI_WAIT_TIMER_BOSS_FOUR;
				return;
			}
		}
	}

	// step
	float vec_x = 0.0f, vec_y = 0.0f;
	if (ai_stat->step[AI_STAT_STEP_W] == 0) {
		ai_stat->step[AI_STAT_STEP_W] += 1;
		ai_stat->timer1 = AI_WAIT_TIMER_BOSS_FOUR;
		return;
	}
	else if (ai_stat->step[AI_STAT_STEP_W] == 3) {
		// stay
		ai_stat->step[AI_STAT_STEP_W] += 1;
		ai_stat->timer1 = AI_WAIT_TIMER_BOSS_FOUR;
		return;
	}
	else if (ai_stat->step[AI_STAT_STEP_W] == 4) {
		// move (open)
		unit_manager_enemy_set_anim_stat(unit_data->id, ANIM_STAT_FLAG_MOVE);
		collision_manager_set_filter(unit_data->col_shape, COLLISION_GROUP_MASK_PLAYER_BULLET);

		// create trap (enemy ghost)
		((unit_enemy_data_t*)unit_data)->resistance_stat |= UNIT_EFFECT_FLAG_E_NO_TRAP_DAMAGE;
		ai_stat->ghost_id = unit_manager_create_trap(unit_data->col_shape->x, unit_data->col_shape->y, unit_manager_search_trap(ghost_trap_four_path));
		unit_trap_data_t* trap_data = unit_manager_get_trap(ai_stat->ghost_id);
		collision_manager_set_filter(trap_data->col_shape, COLLISION_GROUP_MASK_PLAYER_BULLET);

		// set move_to point
		int new_step = ai_manager_get_player_direction(unit_data);
		if (new_step == AI_STAT_STEP_W) { // W
			boss_four_target[0][0] = unit_data->col_shape->x - 128;
			boss_four_target[0][1] = unit_data->col_shape->y;
			boss_four_target[1][0] = unit_data->col_shape->x;
			boss_four_target[1][1] = unit_data->col_shape->y;
		}
		else if (new_step == AI_STAT_STEP_S) { // S
			boss_four_target[0][0] = unit_data->col_shape->x;
			boss_four_target[0][1] = unit_data->col_shape->y + 128;
			boss_four_target[1][0] = unit_data->col_shape->x;
			boss_four_target[1][1] = unit_data->col_shape->y;
		}
		else if (new_step == AI_STAT_STEP_E) { // E
			boss_four_target[0][0] = unit_data->col_shape->x + 128;
			boss_four_target[0][1] = unit_data->col_shape->y;
			boss_four_target[1][0] = unit_data->col_shape->x;
			boss_four_target[1][1] = unit_data->col_shape->y;
		}
		else if (new_step == AI_STAT_STEP_N) { // N
			boss_four_target[0][0] = unit_data->col_shape->x;
			boss_four_target[0][1] = unit_data->col_shape->y - 128;
			boss_four_target[1][0] = unit_data->col_shape->x;
			boss_four_target[1][1] = unit_data->col_shape->y;
		}

		unit_data->col_shape->float_x = (float)unit_data->col_shape->x;
		unit_data->col_shape->float_y = (float)unit_data->col_shape->y;

		ai_stat->step[AI_STAT_STEP_W] += 1;
		ai_stat->timer1 = AI_WAIT_TIMER_BOSS_FOUR;
		return;
	}
	else if (ai_stat->step[AI_STAT_STEP_W] == 5) {
		ai_stat->step[AI_STAT_STEP_W] += 1;
		ai_stat->timer1 = AI_WAIT_TIMER_BOSS_FOUR;
		return;
	}
	else if (ai_stat->step[AI_STAT_STEP_W] >= 6) {
		collision_manager_set_filter(unit_data->col_shape, (COLLISION_GROUP_MASK_PLAYER | COLLISION_GROUP_MASK_ITEMS | COLLISION_GROUP_MASK_PLAYER_BULLET | COLLISION_GROUP_MASK_MAP));

		// delete trap (enemy ghost)
		ai_manager_delete_ghost(ai_data);
		((unit_enemy_data_t*)unit_data)->resistance_stat &= ~UNIT_EFFECT_FLAG_E_NO_TRAP_DAMAGE;

		unit_data->col_shape->vec_x = 0.0f;
		unit_data->col_shape->vec_y = 0.0f;
		b2Vec2 new_vec((float32)unit_data->col_shape->vec_x, (float32)unit_data->col_shape->vec_y);
		unit_data->col_shape->b2body->SetLinearVelocity(new_vec);
		unit_manager_enemy_set_anim_stat(unit_data->id, ANIM_STAT_FLAG_IDLE);

		ai_stat->step[AI_STAT_STEP_W] = 0;
	}

	// set wait timer
	ai_stat->timer1 = AI_WAIT_TIMER_BOSS_FOUR;
}

// ai_stat->val2: attack_count
// ai_stat->val3: attack_step
void ai_manager_boss_update_five(ai_data_t* ai_data)
{
	unit_data_t* unit_data = (unit_data_t*)ai_data->obj;
	if (unit_data->type != UNIT_TYPE_ENEMY) {
		LOG_ERROR("Error: update_five param error.");
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
				int attack_step = ai_stat->val3;
				if (((attack_count == 1) || (attack_count == 2) || (attack_count == 4))
					&& (angle >= 0) && (angle < b2_pi * 5.0f / 180.0f)) {
					if ((attack_step == 0) || (attack_step == 1)) {
						attack_dirt = true;
					}
				}
				else if ((attack_count == 4)
					&& (angle >= b2_pi * 90.0f / 180.0f) && (angle < b2_pi * 95.0f / 180.0f)) {
					if (attack_step == 0) {
						attack_dirt = true;
					}
					else if (attack_step == 1) {
						ai_stat->val3 += 1; // next step
						ai_stat->step[AI_STAT_STEP_W] += 1;
					}
				}
				else if (((attack_count == 2) || (attack_count == 4))
					&& (angle >= b2_pi * 180.0f / 180.0f) && (angle < b2_pi * 185.0f / 180.0f)) {
					if ((attack_step == 0) || (attack_step == 2)) {
						attack_dirt = true;
					}
				}
				else if ((attack_count == 4)
					&& (angle >= b2_pi * 270.0f / 180.0f) && (angle < b2_pi * 275.0f / 180.0f)) {
					if (attack_step == 0) {
						attack_dirt = true;
						ai_stat->val3 += 1; // next step
					}
					else if (attack_step == 2) {
						ai_stat->val3 = 0; // next step
						ai_stat->step[AI_STAT_STEP_W] += 1;
					}
				}

				if (attack_dirt) {
					if (attack_step == 0) {
						unit_manager_enemy_set_anim_stat(unit_data->id, ANIM_STAT_FLAG_ATTACK1);
					}
					else if (attack_step == 1) {
						ai_stat_bullet_t* bullet = (ai_stat_bullet_t*)((unit_enemy_data_t*)unit_data)->bullet[1];
						bullet->val2 = AI_BULLET_PARAM_XCROSS_OFF; // off
						unit_manager_enemy_set_anim_stat(unit_data->id, ANIM_STAT_FLAG_ATTACK2);
					}
					else if (attack_step == 2) {
						ai_stat_bullet_t* bullet = (ai_stat_bullet_t*)((unit_enemy_data_t*)unit_data)->bullet[1];
						bullet->val2 = AI_BULLET_PARAM_XCROSS_ON; // on
						unit_manager_enemy_set_anim_stat(unit_data->id, ANIM_STAT_FLAG_ATTACK2);
					}

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
	ai_stat->timer1 = AI_WAIT_TIMER_BOSS_FIVE;
}

int ai_manager_boss_spawn_six(ai_data_t* ai_data)
{
	unit_data_t* unit_data = (unit_data_t*)ai_data->obj;
	if (unit_data->type != UNIT_TYPE_ENEMY) {
		LOG_ERROR("Error: spawn_six param error.");
	}

	int enemy_id;
	unit_manager_load_enemy(mini_boss_six_path);

	// left
	enemy_id = unit_manager_create_enemy(120, 152, UNIT_FACE_W, unit_manager_search_enemy(mini_boss_six_path));
	// right
	enemy_id = unit_manager_create_enemy(312, 88, UNIT_FACE_W, unit_manager_search_enemy(mini_boss_six_path));

	// spawn sound
	sound_manager_play(resource_manager_getChunkFromPath("sounds/sfx_drop.ogg"));

	return 0;
}

// ai_stat->val2: mini_boss_on (no use)
// ai_stat->val3: attack_step
void ai_manager_boss_update_six(ai_data_t* ai_data)
{
	unit_data_t* unit_data = (unit_data_t*)ai_data->obj;
	if (unit_data->type != UNIT_TYPE_ENEMY) {
		LOG_ERROR("Error: update_six param error.");
	}

	ai_stat_data_t* ai_stat = (ai_stat_data_t*)ai_data;
	if (ai_stat->timer1 > 0) {
		ai_stat->timer1 -= unit_manager_enemy_get_delta_time((unit_enemy_data_t*)unit_data);
		return;  // waitting
	}

	int used_count = 0;
	for (int i = AI_STAT_STEP_N; i < AI_STAT_STEP_END; i++) {
		if (ai_stat->step[i]) used_count += 1;
	}

	int mini_boss_on = ai_stat->val2;
	int attack_step = ai_stat->val3;
	if (attack_step == 0) {
		if (used_count == AI_STAT_STEP_END) {
			ai_stat->val3 += 1;
		}
	}

	if (ai_stat->val3 == 1) {
		// attack
		if (ai_stat->val1 & AI_PARAM_ATTACK) {
			unit_manager_enemy_set_anim_stat(unit_data->id, ANIM_STAT_FLAG_ATTACK1);
			ai_stat->step[AI_STAT_STEP_W] += 1;
			ai_stat->timer1 = AI_WAIT_TIMER_BOSS_SIX;
		}

		ai_stat->val3 += 1;
		return;
	}
	else if (ai_stat->val3 == 2) {
		// attack
		if (ai_stat->val1 & AI_PARAM_ATTACK) {
			unit_manager_enemy_set_anim_stat(unit_data->id, ANIM_STAT_FLAG_ATTACK2);
			ai_stat->step[AI_STAT_STEP_W] += 1;
			ai_stat->timer1 = AI_WAIT_TIMER_BOSS_SIX;
		}

		// init
		ai_stat->step[AI_STAT_STEP_N] = 0;
		ai_stat->step[AI_STAT_STEP_E] = 0;
		ai_stat->step[AI_STAT_STEP_W] = 0;
		ai_stat->step[AI_STAT_STEP_S] = 0;
		used_count = 0;
		ai_stat->timer1 = AI_WAIT_TIMER_BOSS_SIX;
		ai_stat->val3 = 0; // attack_step reset

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
	ai_stat->timer1 = AI_WAIT_TIMER_BOSS_SIX;
}

static int boss_seven_target[2][2];
void ai_manager_boss_update_seven(ai_data_t* ai_data)
{
	unit_data_t* unit_data = (unit_data_t*)ai_data->obj;
	if (unit_data->type != UNIT_TYPE_ENEMY) {
		LOG_ERROR("Error: update_seven param error.");
	}

	ai_stat_data_t* ai_stat = (ai_stat_data_t*)ai_data;
	if ((2 <= ai_stat->step[AI_STAT_STEP_W]) && (ai_stat->step[AI_STAT_STEP_W] <= 8)) {
		if (ai_stat->step[AI_STAT_STEP_W] == 2) {
			ai_manager_move_to(unit_data, boss_seven_target[0][0], boss_seven_target[0][1], 6.0f);
		}
		if (ai_stat->step[AI_STAT_STEP_W] == 8) {
			ai_manager_move_to(unit_data, boss_seven_target[1][0], boss_seven_target[1][1], 6.0f);
		}
	}

	if (ai_stat->timer1 > 0) {
		ai_stat->timer1 -= unit_manager_enemy_get_delta_time((unit_enemy_data_t*)unit_data);
		return;  // waitting
	}

	// attack
	bool attack_dirt = false;
	if (ai_stat->step[AI_STAT_STEP_W] == 3) {
		if (ai_stat->val1 & AI_PARAM_ATTACK) {
			attack_dirt = true;
		}
	}
	else if (ai_stat->step[AI_STAT_STEP_W] == 4) {
		if (ai_stat->val1 & AI_PARAM_ATTACK) {
			attack_dirt = true;
		}
	}
	else if (ai_stat->step[AI_STAT_STEP_W] == 5) {
		if (ai_stat->val1 & AI_PARAM_ATTACK) {
			attack_dirt = true;
		}
	}
	else if (ai_stat->step[AI_STAT_STEP_W] == 6) {
		if (ai_stat->val1 & AI_PARAM_ATTACK) {
			attack_dirt = true;
		}
	}

	if (attack_dirt) {
		// spawn boms
		int bom_x = game_utils_random_gen((g_map_x_max - 2) * g_tile_width + g_tile_width / 2, (g_tile_width + g_tile_width / 2));
		int bom_y = game_utils_random_gen((g_map_y_max - 2) * g_tile_height + g_tile_height / 2, (g_tile_height + g_tile_height / 2));
		int id = unit_manager_create_items(bom_x, bom_y, unit_manager_search_items(fall_bom_path));
		unit_manager_create_effect(bom_x, bom_y, unit_manager_search_effect(enemy_shadow_drop_path[ANIM_BASE_SIZE_32x32]));

		ai_stat->step[AI_STAT_STEP_W] += 1;
		if (ai_stat->step[AI_STAT_STEP_W] == 7) ai_stat->timer1 = AI_WAIT_TIMER_BOSS_SEVEN * 3;
		else  ai_stat->timer1 = AI_WAIT_TIMER_BOSS_SEVEN;
		return;
	}

	// wait
	float vec_x = 0.0f, vec_y = 0.0f;
	if (ai_stat->step[AI_STAT_STEP_W] == 0) {
		ai_stat->step[AI_STAT_STEP_W] += 1;
		ai_stat->timer1 = AI_WAIT_TIMER_BOSS_SEVEN;
		return;
	}
	else if (ai_stat->step[AI_STAT_STEP_W] == 1) {
		// move (open)
		unit_manager_enemy_set_anim_stat(unit_data->id, ANIM_STAT_FLAG_MOVE);

		// detect direction
		boss_seven_target[0][0] = unit_data->col_shape->x;
		boss_seven_target[0][1] = -256;
		boss_seven_target[1][0] = unit_data->col_shape->x;
		boss_seven_target[1][1] = unit_data->col_shape->y;
		unit_manager_create_effect(unit_data->col_shape->x, unit_data->col_shape->y, unit_manager_search_effect(enemy_shadow_path[ANIM_BASE_SIZE_64x64]));

		unit_data->col_shape->float_x = (float)unit_data->col_shape->x;
		unit_data->col_shape->float_y = (float)unit_data->col_shape->y;

		ai_stat->step[AI_STAT_STEP_W] += 1;
		ai_stat->timer1 = AI_WAIT_TIMER_BOSS_SEVEN;
		return;
	}
	else if (ai_stat->step[AI_STAT_STEP_W] == 2) {
		ai_stat->step[AI_STAT_STEP_W] += 1;
		ai_stat->timer1 = AI_WAIT_TIMER_BOSS_SEVEN / 4;
		return;
	}
	else if (ai_stat->step[AI_STAT_STEP_W] == 7) {
		unit_manager_create_effect(boss_seven_target[1][0], boss_seven_target[1][1], unit_manager_search_effect(enemy_shadow_drop_path[ANIM_BASE_SIZE_64x64]));

		ai_stat->step[AI_STAT_STEP_W] += 1;
		ai_stat->timer1 = AI_WAIT_TIMER_BOSS_SEVEN;
		return;
	}
	else if (ai_stat->step[AI_STAT_STEP_W] > 7) {
		unit_data->col_shape->vec_x = 0.0f;
		unit_data->col_shape->vec_y = 0.0f;
		b2Vec2 new_vec((float32)unit_data->col_shape->vec_x, (float32)unit_data->col_shape->vec_y);
		unit_data->col_shape->b2body->SetLinearVelocity(new_vec);
		unit_manager_enemy_set_anim_stat(unit_data->id, ANIM_STAT_FLAG_IDLE);

		ai_stat->step[AI_STAT_STEP_W] = 0;
	}

	// set wait timer
	ai_stat->timer1 = AI_WAIT_TIMER_BOSS_SEVEN;
}

int ai_manager_boss_spawn_eight(ai_data_t* ai_data)
{
	unit_data_t* unit_data = (unit_data_t*)ai_data->obj;
	if (unit_data->type != UNIT_TYPE_ENEMY) {
		LOG_ERROR("Error: spawn_eight param error.");
	}

	if (unit_manager_enemy_get_enemy_count() < 3) {
		int enemy_id;
		unit_manager_load_enemy(mini_enemy_eight_path);

		int enemy_cx, enemy_cy;
		unit_manager_get_center_position(unit_data, &enemy_cx, &enemy_cy);

		enemy_cx = MAX(g_tile_width, MIN((g_map_x_max - 1) * g_tile_width - g_tile_width / 2, enemy_cx));
		enemy_cy = MAX(g_tile_height, MIN((g_map_y_max - 1) * g_tile_height - g_tile_height / 2, enemy_cy));

		// left
		enemy_id = unit_manager_create_enemy(enemy_cx, enemy_cy, UNIT_FACE_W, unit_manager_search_enemy(mini_enemy_eight_path));
		// right
		enemy_id = unit_manager_create_enemy(enemy_cx + g_tile_width / 2, enemy_cy, UNIT_FACE_E, unit_manager_search_enemy(mini_enemy_eight_path));

		// spawn sound
		sound_manager_play(resource_manager_getChunkFromPath("sounds/sfx_drop.ogg"));
	}

	return 0;
}

void ai_manager_boss_update_eight(ai_data_t* ai_data)
{
	ai_stat_data_t* ai_stat = (ai_stat_data_t*)ai_data;
	unit_data_t* unit_data = (unit_data_t*)ai_data->obj;
	if (unit_data->type != UNIT_TYPE_ENEMY) {
		LOG_ERROR("Error: update_eight param error.");
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
				int unit_id = unit_manager_create_effect(attack_region.x, 0, unit_manager_search_effect(high_light_line_path));
				unit_effect_data_t* effect_data = unit_manager_get_effect(unit_id);
				((shape_box_data*)effect_data->col_shape)->w = attack_region.w;
				((shape_box_data*)effect_data->col_shape)->h = g_map_y_max * g_tile_height;
			}
			else { // (unit_data->col_shape->face == UNIT_FACE_E) || (unit_data->col_shape->face == UNIT_FACE_W)
				int unit_id = unit_manager_create_effect(0, attack_region.y, unit_manager_search_effect(high_light_line_path));
				unit_effect_data_t* effect_data = unit_manager_get_effect(unit_id);
				((shape_box_data*)effect_data->col_shape)->w = g_map_x_max * g_tile_width;
				((shape_box_data*)effect_data->col_shape)->h = attack_region.h;
			}

			// reset timer
			ai_stat->timer1 = AI_WAIT_TIMER_BOSS_EIGHT;
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
				ai_stat->timer1 = AI_WAIT_TIMER_BOSS_EIGHT;
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
				ai_stat->timer1 = AI_WAIT_TIMER_BOSS_EIGHT;
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
				ai_stat->timer1 = AI_WAIT_TIMER_BOSS_EIGHT;
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
				ai_stat->timer1 = AI_WAIT_TIMER_BOSS_EIGHT;
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

	int spawn_on = ai_stat->val4;
	if ((spawn_on == AI_PARAM_SPAWN_OFF) && (used_count == AI_STAT_STEP_END)) {
		ai_manager_boss_spawn_eight(ai_data);
		ai_stat->timer1 = AI_WAIT_TIMER_BOSS_EIGHT;
		ai_stat->val4 = AI_PARAM_SPAWN_ON;
		return;
	}
	else if ((spawn_on == AI_PARAM_SPAWN_ON) && (used_count == AI_STAT_STEP_END)) {
		ai_stat->step[AI_STAT_STEP_N] = 0;
		ai_stat->step[AI_STAT_STEP_E] = 0;
		ai_stat->step[AI_STAT_STEP_W] = 0;
		ai_stat->step[AI_STAT_STEP_S] = 0;
		used_count = 0;
		ai_stat->val4 = AI_PARAM_SPAWN_OFF;

		ai_stat->timer1 = AI_WAIT_TIMER_BOSS_EIGHT;
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
	ai_stat->timer1 = AI_WAIT_TIMER_BOSS_EIGHT;
}

static int boss_nine_target[2][2];
void ai_manager_boss_update_nine(ai_data_t* ai_data)
{
	unit_data_t* unit_data = (unit_data_t*)ai_data->obj;
	if (unit_data->type != UNIT_TYPE_ENEMY) {
		LOG_ERROR("Error: update_nine param error.");
	}

	ai_stat_data_t* ai_stat = (ai_stat_data_t*)ai_data;
	if ((4 <= ai_stat->step[AI_STAT_STEP_W]) && (ai_stat->step[AI_STAT_STEP_W] <= 6)) {
		if (ai_stat->step[AI_STAT_STEP_W] == 4) {
			ai_manager_move_to(unit_data, boss_nine_target[0][0], boss_nine_target[0][1], 6.0f);
		}
		if (ai_stat->step[AI_STAT_STEP_W] == 6) {
			ai_manager_move_to(unit_data, boss_nine_target[1][0], boss_nine_target[1][1], 4.0f);
		}
	}

	if (ai_stat->timer1 > 0) {
		ai_stat->timer1 -= unit_manager_enemy_get_delta_time((unit_enemy_data_t*)unit_data);
		return;  // waitting
	}

	// attack
	bool attack_dirt = false;
	ai_stat_bullet_t* ai_stat_bullet = (ai_stat_bullet_t*)((unit_enemy_data_t*)unit_data)->bullet[0];
	int anim_stat_flag = ANIM_STAT_FLAG_ATTACK1;

	if (ai_stat->step[AI_STAT_STEP_W] == 1) {
		if (ai_stat->val1 & AI_PARAM_ATTACK) {
			//ai_stat_bullet = (ai_stat_bullet_t*)((unit_enemy_data_t*)unit_data)->bullet[0];
			//anim_stat_flag = ANIM_STAT_FLAG_ATTACK1;
			attack_dirt = true;
		}
	}
	else if (ai_stat->step[AI_STAT_STEP_W] == 2) {
		if (ai_stat->val1 & AI_PARAM_ATTACK) {
			ai_stat_bullet = (ai_stat_bullet_t*)((unit_enemy_data_t*)unit_data)->bullet[1];
			anim_stat_flag = ANIM_STAT_FLAG_ATTACK2;
			attack_dirt = true;
		}
	}

	if (attack_dirt) {
		// set attack action
		unit_manager_enemy_set_anim_stat(unit_data->id, anim_stat_flag);
		ai_stat->step[AI_STAT_STEP_W] += 1;
		ai_stat->timer1 = AI_WAIT_TIMER_BOSS_NINE;
		return;
	}

	// wait
	float vec_x = 0.0f, vec_y = 0.0f;
	if (ai_stat->step[AI_STAT_STEP_W] == 0) {
		ai_stat->step[AI_STAT_STEP_W] += 1;
		ai_stat->timer1 = AI_WAIT_TIMER_BOSS_NINE;
		return;
	}
	else if (ai_stat->step[AI_STAT_STEP_W] == 3) {
		// move (open)
		unit_manager_enemy_set_anim_stat(unit_data->id, ANIM_STAT_FLAG_MOVE);

		// detect direction
		boss_nine_target[0][0] = unit_data->col_shape->x;
		boss_nine_target[0][1] = -256;
		//boss_nine_target[1][0] = g_player.col_shape->x;
		//boss_nine_target[1][1] = g_player.col_shape->y;
		unit_manager_create_effect(unit_data->col_shape->x, unit_data->col_shape->y, unit_manager_search_effect(enemy_shadow_path[ANIM_BASE_SIZE_64x64]));

		unit_data->col_shape->float_x = (float)unit_data->col_shape->x;
		unit_data->col_shape->float_y = (float)unit_data->col_shape->y;

		ai_stat->step[AI_STAT_STEP_W] += 1;
		ai_stat->timer1 = AI_WAIT_TIMER_BOSS_NINE;
		return;
	}
	else if (ai_stat->step[AI_STAT_STEP_W] == 4) {
		ai_stat->step[AI_STAT_STEP_W] += 1;
		ai_stat->timer1 = AI_WAIT_TIMER_BOSS_NINE;
		return;
	}
	else if (ai_stat->step[AI_STAT_STEP_W] == 5) {
		// get current player position
		int player_cx, player_cy;
		unit_manager_get_center_position((unit_data_t*)&g_player, &player_cx, &player_cy);
		int drop_x = player_cx - 32;
		int drop_y = player_cy - 32;
		boss_nine_target[1][0] = MAX(g_tile_width, MIN((g_map_x_max - 1) * g_tile_width - g_tile_width / 2, drop_x));
		boss_nine_target[1][1] = MAX(g_tile_height, MIN((g_map_y_max - 1) * g_tile_height - g_tile_height, drop_y));
		unit_manager_create_effect(boss_nine_target[1][0], boss_nine_target[1][1], unit_manager_search_effect(enemy_shadow_drop_path[ANIM_BASE_SIZE_64x64]));

		ai_stat->step[AI_STAT_STEP_W] += 1;
		ai_stat->timer1 = AI_WAIT_TIMER_BOSS_NINE * 2;
		return;
	}
	else if (ai_stat->step[AI_STAT_STEP_W] > 5) {
		unit_data->col_shape->vec_x = 0.0f;
		unit_data->col_shape->vec_y = 0.0f;
		b2Vec2 new_vec((float32)unit_data->col_shape->vec_x, (float32)unit_data->col_shape->vec_y);
		unit_data->col_shape->b2body->SetLinearVelocity(new_vec);
		unit_manager_enemy_set_anim_stat(unit_data->id, ANIM_STAT_FLAG_IDLE);

		ai_stat->step[AI_STAT_STEP_W] = 0;
	}

	// set wait timer
	ai_stat->timer1 = AI_WAIT_TIMER_BOSS_NINE;
}

#define AI_ENEMY_X_PATH_SIZE  4
static std::string enemy_x_path[AI_ENEMY_X_PATH_SIZE] = {
	"units/enemy/4/lv3/four.unit",
	"units/enemy/6/lv3/six.unit",
	"units/enemy/8/lv3/eight.unit",
	"units/enemy/9/lv3/nine.unit",
};
int ai_manager_boss_spawn_x(ai_data_t* ai_data)
{
	unit_data_t* unit_data = (unit_data_t*)ai_data->obj;
	if (unit_data->type != UNIT_TYPE_ENEMY) {
		LOG_ERROR("Error: spawn_x param error.");
	}

	int enemy_x_path_index = ai_data->val2;

	int enemy_id;
	unit_manager_load_enemy(enemy_x_path[enemy_x_path_index]);

	// left
	enemy_id = unit_manager_create_enemy(128, 128, UNIT_FACE_W, unit_manager_search_enemy(enemy_x_path[enemy_x_path_index]));
	// right
	enemy_id = unit_manager_create_enemy(322, 128, UNIT_FACE_E, unit_manager_search_enemy(enemy_x_path[enemy_x_path_index]));

	// spawn sound
	sound_manager_play(resource_manager_getChunkFromPath("sounds/sfx_drop.ogg"));

	ai_data->val2 += 1;
	if (ai_data->val2 >= AI_ENEMY_X_PATH_SIZE) ai_data->val2 = 0;

	return 0;
}

//-----------------------------------------------
// step0.  start wait
//-----------------------------------------------
// step1.  boss jump start
// step2.   (wait)
// step3.   bom1
// step4.   bom2
// step5.   bom3
// step6.   bom4
// step7.  boss fall start
// step8.  boss fall finish
//-----------------------------------------------
// step9.  attack1
// step10. attack1
//-----------------------------------------------
// step11. (wait for cleaning) attack1
//         (no enemy) attack2 anim
// step12. spawn enemy
//-----------------------------------------------
// step13. (wait)
//-----------------------------------------------
// step14. return step0.
//-----------------------------------------------

// ai_stat->val2: enemy_x_path_index
static int boss_x_target[2][2];
void ai_manager_boss_update_x(ai_data_t* ai_data)
{
	unit_data_t* unit_data = (unit_data_t*)ai_data->obj;
	if (unit_data->type != UNIT_TYPE_ENEMY) {
		LOG_ERROR("Error: update_x param error.");
	}

	ai_stat_data_t* ai_stat = (ai_stat_data_t*)ai_data;
	if ((2 <= ai_stat->step[AI_STAT_STEP_W]) && (ai_stat->step[AI_STAT_STEP_W] <= 8)) {
		if (ai_stat->step[AI_STAT_STEP_W] == 2) {
			ai_manager_move_to(unit_data, boss_x_target[0][0], boss_x_target[0][1], 6.0f);
		}
		if (ai_stat->step[AI_STAT_STEP_W] == 8) {
			ai_manager_move_to(unit_data, boss_x_target[1][0], boss_x_target[1][1], 6.0f);
		}
	}

	if (ai_stat->timer1 > 0) {
		ai_stat->timer1 -= unit_manager_enemy_get_delta_time((unit_enemy_data_t*)unit_data);
		return;  // waitting
	}

	// attack (bom)
	bool attack_dirt = false;
	if (ai_stat->step[AI_STAT_STEP_W] == 3) {
		if (ai_stat->val1 & AI_PARAM_ATTACK) {
			attack_dirt = true;
		}
	}
	else if (ai_stat->step[AI_STAT_STEP_W] == 4) {
		if (ai_stat->val1 & AI_PARAM_ATTACK) {
			attack_dirt = true;
		}
	}
	else if (ai_stat->step[AI_STAT_STEP_W] == 5) {
		if (ai_stat->val1 & AI_PARAM_ATTACK) {
			attack_dirt = true;
		}
	}
	else if (ai_stat->step[AI_STAT_STEP_W] == 6) {
		if (ai_stat->val1 & AI_PARAM_ATTACK) {
			attack_dirt = true;
		}
	}

	if (attack_dirt) {
		// spawn boms
		int bom_x = game_utils_random_gen((g_map_x_max - 2) * g_tile_width + g_tile_width / 2, (g_tile_width + g_tile_width / 2));
		int bom_y = game_utils_random_gen((g_map_y_max - 2) * g_tile_height + g_tile_height / 2, (g_tile_height + g_tile_height / 2));
		int id = unit_manager_create_items(bom_x, bom_y, unit_manager_search_items(fall_bom_path));
		unit_manager_create_effect(bom_x, bom_y, unit_manager_search_effect(enemy_shadow_drop_path[ANIM_BASE_SIZE_32x32]));

		ai_stat->step[AI_STAT_STEP_W] += 1;
		if (ai_stat->step[AI_STAT_STEP_W] == 7) ai_stat->timer1 = AI_WAIT_TIMER_BOSS_X * 3;
		else  ai_stat->timer1 = AI_WAIT_TIMER_BOSS_X;
		return;
	}

	// attack1 (bullet)
	if ((ai_stat->step[AI_STAT_STEP_W] == 9) || (ai_stat->step[AI_STAT_STEP_W] == 10)) {
		ai_stat_bullet_t* ai_bullet = (ai_stat_bullet_t*)((unit_enemy_data_t*)unit_data)->bullet[0];
		if (ai_bullet->val1 & AI_BULLET_PARAM_TARGET) {
			unit_manager_get_center_position((unit_data_t*)&g_player, &ai_bullet->val2, &ai_bullet->val3);
			ai_bullet->val4 = ai_manager_get_player_direction(unit_data);
		}

		unit_manager_enemy_set_anim_stat(unit_data->id, ANIM_STAT_FLAG_ATTACK1);
		ai_stat->step[AI_STAT_STEP_W] += 1;
		ai_stat->timer1 = AI_WAIT_TIMER_BOSS_X * 2;
		return;
	}

	// spawn
	if (ai_stat->step[AI_STAT_STEP_W] == 11) {
		if (unit_manager_enemy_get_enemy_count() == 1) {
			unit_manager_enemy_set_anim_stat(unit_data->id, ANIM_STAT_FLAG_ATTACK2);
			ai_stat->step[AI_STAT_STEP_W] += 1;
			ai_stat->timer1 = AI_WAIT_TIMER_BOSS_X_SPAWN; // => attack amin play time
		}
		else {
			if (ai_stat->val1 & AI_PARAM_ATTACK) {
				ai_stat_bullet_t* ai_bullet = (ai_stat_bullet_t*)((unit_enemy_data_t*)unit_data)->bullet[0];
				if (ai_bullet->val1 & AI_BULLET_PARAM_TARGET) {
					unit_manager_get_center_position((unit_data_t*)&g_player, &ai_bullet->val2, &ai_bullet->val3);
					ai_bullet->val4 = ai_manager_get_player_direction(unit_data);
				}

				unit_manager_enemy_set_anim_stat(unit_data->id, ANIM_STAT_FLAG_ATTACK1);
				ai_stat->timer1 = AI_WAIT_TIMER_BOSS_X * 2;
				return;
			}
			else {
				ai_stat->timer1 = AI_WAIT_TIMER_BOSS_X;
			}
		}
		return;
	}
	else if (ai_stat->step[AI_STAT_STEP_W] == 12) {
		ai_manager_boss_spawn_x(ai_data);
		ai_stat->step[AI_STAT_STEP_W] += 1;
		ai_stat->timer1 = AI_WAIT_TIMER_BOSS_X;
		return;
	}

	// wait
	if (ai_stat->step[AI_STAT_STEP_W] == 13) {
		ai_stat->step[AI_STAT_STEP_W] += 1;
		ai_stat->timer1 = AI_WAIT_TIMER_BOSS_X * 2;
		return;
	}

	float vec_x = 0.0f, vec_y = 0.0f;
	if (ai_stat->step[AI_STAT_STEP_W] == 0) {
		// start wait
		ai_stat->step[AI_STAT_STEP_W] += 1;
		ai_stat->timer1 = AI_WAIT_TIMER_BOSS_X;
		return;
	}
	else if (ai_stat->step[AI_STAT_STEP_W] == 1) {
		// move (open)
		//unit_manager_enemy_set_anim_stat(unit_data->id, ANIM_STAT_FLAG_MOVE);

		// detect direction
		boss_x_target[0][0] = unit_data->col_shape->x;
		boss_x_target[0][1] = -256;
		boss_x_target[1][0] = unit_data->col_shape->x;
		boss_x_target[1][1] = unit_data->col_shape->y;
		unit_manager_create_effect(unit_data->col_shape->x, unit_data->col_shape->y, unit_manager_search_effect(enemy_shadow_path[ANIM_BASE_SIZE_64x64]));

		unit_data->col_shape->float_x = (float)unit_data->col_shape->x;
		unit_data->col_shape->float_y = (float)unit_data->col_shape->y;

		ai_stat->step[AI_STAT_STEP_W] += 1;
		ai_stat->timer1 = AI_WAIT_TIMER_BOSS_X;
		return;
	}
	else if (ai_stat->step[AI_STAT_STEP_W] == 2) {
		ai_stat->step[AI_STAT_STEP_W] += 1;
		ai_stat->timer1 = AI_WAIT_TIMER_BOSS_X / 4;
		return;
	}
	else if (ai_stat->step[AI_STAT_STEP_W] == 7) {
		unit_manager_create_effect(boss_x_target[1][0], boss_x_target[1][1], unit_manager_search_effect(enemy_shadow_drop_path[ANIM_BASE_SIZE_64x64]));

		ai_stat->step[AI_STAT_STEP_W] += 1;
		ai_stat->timer1 = AI_WAIT_TIMER_BOSS_X;
		return;
	}
	else if (ai_stat->step[AI_STAT_STEP_W] == 8) {
		unit_data->col_shape->vec_x = 0.0f;
		unit_data->col_shape->vec_y = 0.0f;
		b2Vec2 new_vec((float32)unit_data->col_shape->vec_x, (float32)unit_data->col_shape->vec_y);
		unit_data->col_shape->b2body->SetLinearVelocity(new_vec);
		//unit_manager_enemy_set_anim_stat(unit_data->id, ANIM_STAT_FLAG_IDLE);

		ai_stat->step[AI_STAT_STEP_W] += 1;
		ai_stat->timer1 = AI_WAIT_TIMER_BOSS_X;
		return;
	}
	else if (ai_stat->step[AI_STAT_STEP_W] > 13) {
		ai_stat->step[AI_STAT_STEP_W] = 0;
	}

	// set wait timer
	ai_stat->timer1 = AI_WAIT_TIMER_BOSS_X;
}
