#include <fstream>
#include "game_common.h"
#include "unit_manager.h"

#include "game_timer.h"
#include "game_log.h"
#include "game_utils.h"
#include "game_window.h"
#include "game_event.h"

#include "resource_manager.h"
#include "stage_manager.h"
#include "map_manager.h"
#include "sound_manager.h"
#include "scene_play_stage.h"

int unit_manager_init()
{
	unit_manager_init_player();
	unit_manager_init_enemy();
	unit_manager_init_items();
	unit_manager_init_trap();
	unit_manager_init_effect();
	unit_manager_init_player_bullet();
	unit_manager_init_enemy_bullet();

	return 0;
}

void unit_manager_unload()
{
	unit_manager_unload_player();
	unit_manager_unload_enemy();
	unit_manager_unload_items();
	unit_manager_unload_trap();
	unit_manager_unload_effect();
	unit_manager_unload_player_bullet();
	unit_manager_unload_enemy_bullet();
}

void load_collision(std::string& line, shape_data** col_shape)
{
	std::string key, value;
	game_utils_split_key_value(line, key, value);

	if (value == "") value = "0";
	if (key == "type") {
		if (value == "BOX") {
			*col_shape = (shape_data *)collision_manager_new_shape_box();
		}
		else if (value == "ROUND") {
			*col_shape = (shape_data *)collision_manager_new_shape_round();
		}
		else if (value == "BOX_S") {
			*col_shape = (shape_data*)collision_manager_new_shape_box(COLLISION_ID_STATIC_SHAPE);
		}
		else if (value == "ROUND_S") {
			*col_shape = (shape_data*)collision_manager_new_shape_round(COLLISION_ID_STATIC_SHAPE);
		}
	}
	if (key == "group") collision_manager_set_group(*col_shape, value);

	if (key == "x") {
		(*col_shape)->offset_x = atoi(value.c_str());
	}
	if (key == "y") {
		(*col_shape)->offset_y = atoi(value.c_str());
	}

	if (key == "face_type") {
		if (value == "LR") {
			(*col_shape)->face_type = UNIT_FACE_TYPE_LR;
		}
		else if (value == "UD") {
			(*col_shape)->face_type = UNIT_FACE_TYPE_UD;
		}
		else if (value == "ALL") {
			(*col_shape)->face_type = UNIT_FACE_TYPE_ALL;
		}
		else {
			(*col_shape)->face_type = UNIT_FACE_TYPE_NONE;
		}
		return;
	}

	if (key == "vec_x_max") {
		(*col_shape)->vec_x_max = (float)atof(value.c_str());
	}
	if (key == "vec_y_max") {
		(*col_shape)->vec_y_max = (float)atof(value.c_str());
	}
	if (key == "vec_x_delta") {
		(*col_shape)->vec_x_delta = (float)atof(value.c_str());
	}
	if (key == "vec_y_delta") {
		(*col_shape)->vec_y_delta = (float)atof(value.c_str());
	}

	// joint
	if (key == "joint_type") {
		if (value == "PIN") {
			(*col_shape)->joint_type = COLLISION_JOINT_TYPE_PIN;
		}
		else if (value == "PIN_ROUND") {
			(*col_shape)->joint_type = COLLISION_JOINT_TYPE_PIN_ROUND;
		}
		else {
			(*col_shape)->joint_type = COLLISION_JOINT_TYPE_NONE;
		}
		return;
	}
	if (key == "joint_x") (*col_shape)->joint_x = atoi(value.c_str());
	if (key == "joint_y") (*col_shape)->joint_y = atoi(value.c_str());
	if (key == "joint_val1") (*col_shape)->joint_val1 = atoi(value.c_str());

	if (key == "w") ((shape_box_data*) *col_shape)->w = atoi(value.c_str());
	if (key == "h") ((shape_box_data*) *col_shape)->h = atoi(value.c_str());
	if (key == "r") ((shape_round_data*) *col_shape)->r = atoi(value.c_str());
}

void load_anim(std::string& line, anim_data_t* anim)
{
	std::string key, value;
	game_utils_split_key_value(line, key, value);
	if (key == "base_w") {
		anim->base_w = atoi(value.c_str());
		return;
	}
	if (key == "base_h") {
		anim->base_h = atoi(value.c_str());
		return;
	}

	// find stat name
	int index_start = -1;
	int index_end = -1;
	for (int i = (int)line.size(); i >= 0; i--) {
		if (line[i] == '.') {
			if (index_end == -1) {
				index_end = i;
			}
			else {
				index_start = i + 1; break;
			}
		}
	}

	std::string stat_str = line.substr(index_start, index_end - index_start);
	int stat_val;
	if (stat_str == "IDLE") {
		stat_val = ANIM_STAT_IDLE;
	}
	else if (stat_str == "MOVE") {
		stat_val = ANIM_STAT_MOVE;
	}
	else if ((stat_str == "ATTACK") || (stat_str == "ATTACK1")) {
		stat_val = ANIM_STAT_ATTACK1;
	}
	else if (stat_str == "ATTACK2") {
		stat_val = ANIM_STAT_ATTACK2;
	}
	else if (stat_str == "DEFENCE") {
		stat_val = ANIM_STAT_DEFENCE;
	}
	else if (stat_str == "DIE") {
		stat_val = ANIM_STAT_DIE;
	}
	else if (stat_str == "SPAWN") {
		stat_val = ANIM_STAT_SPAWN;
	}
	else if (stat_str == "HIDE") {
		stat_val = ANIM_STAT_HIDE;
	}
	else {
		LOG_ERROR("load_anim %s error\n", line.c_str());
		return;
	}

	// set anim path
	char* path_c_str = new char[line.size() + 1];
	memcpy(path_c_str, line.c_str(), line.size());
	path_c_str[line.size()] = '\0';
	anim->anim_stat_base_list[stat_val]->obj = (void*)path_c_str;
}

void load_ai(std::string& line, ai_data_t* ai_data)
{
	std::string key, value;
	game_utils_split_key_value(line, key, value);

	if (value == "") value = "0";
	if (key == "type") {
		if (value == "SIMPLE") {
			ai_data->type = AI_TYPE_SIMPLE;
		}
		else if (value == "SIMPLE_FIRE") {
			ai_data->type = AI_TYPE_SIMPLE_FIRE;
		}
		else if (value == "LEFT_RIGHT") {
			ai_data->type = AI_TYPE_LEFT_RIGHT;
		}
		else if (value == "UP_DOWN") {
			ai_data->type = AI_TYPE_UP_DOWN;
		}
		else if (value == "STAY") {
			ai_data->type = AI_TYPE_STAY;
		}
		else if (value == "FACE_ROUND") {
			ai_data->type = AI_TYPE_FACE_ROUND;
		}
		else if (value == "ROUND") {
			ai_data->type = AI_TYPE_ROUND;
		}
		else if (value == "ROUND_LR") {
			ai_data->type = AI_TYPE_ROUND_LR;
		}
		else if (value == "ROUND_MOVE") {
			ai_data->type = AI_TYPE_ROUND_MOVE;
		}
		else if (value == "RANDOM") {
			ai_data->type = AI_TYPE_RANDOM;
		}
		else if (value == "RANDOM_GRID") {
			ai_data->type = AI_TYPE_RANDOM_GRID;
		}
		else if (value == "GO_TO_BOM") {
			ai_data->type = AI_TYPE_GO_TO_BOM;
		}
		else {
			ai_data->type = AI_TYPE_NONE;
		}
	}

	if (ai_data->type == AI_TYPE_LEFT_RIGHT) {
		if (key == "x") ((ai_common_data_t*)ai_data)->x = atoi(value.c_str());
		if (key == "y") ((ai_common_data_t*)ai_data)->y = atoi(value.c_str());
		if (key == "w") ((ai_common_data_t*)ai_data)->w = atoi(value.c_str());
		if (key == "h") ((ai_common_data_t*)ai_data)->h = atoi(value.c_str());
	}

	// enemy only
	if (key == "bullet1") {
		const char* bullet_path = value.c_str();
		for (int i = 0; i < UNIT_BULLET_ID_END; i++) {
			if (strcmp(bullet_path, g_enemy_bullet_path[i]) == 0) {
				((ai_common_data_t*)ai_data)->bullet1 = i;
				break;
			}
		}
	}
	if (key == "bullet2") {
		const char* bullet_path = value.c_str();
		for (int i = 0; i < UNIT_BULLET_ID_END; i++) {
			if (strcmp(bullet_path, g_enemy_bullet_path[i]) == 0) {
				((ai_common_data_t*)ai_data)->bullet2 = i;
				break;
			}
		}
	}

	int bullet_num = UNIT_BULLET_NUM_NONE;
	if ((key == "bullet1_num") || (key == "bullet2_num")) {
		const char* bullet_path = value.c_str();
		if (strcmp(bullet_path, "SINGLE") == 0) {
			bullet_num = UNIT_BULLET_NUM_SINGLE;
		}
		else if (strcmp(bullet_path, "DOUBLE") == 0) {
			bullet_num = UNIT_BULLET_NUM_DOUBLE;
		}
		else if (strcmp(bullet_path, "TRIPLE") == 0) {
			bullet_num = UNIT_BULLET_NUM_TRIPLE;
		}
	}
	if (key == "bullet1_num") {
		((ai_common_data_t*)ai_data)->bullet1_num = bullet_num;
	}
	if (key == "bullet2_num") {
		((ai_common_data_t*)ai_data)->bullet2_num = bullet_num;
	}

	int bullet_face = UNIT_FACE_NONE;
	if ((key == "bullet1_face") || (key == "bullet2_face")) {
		const char* bullet_face_str = value.c_str();
		if (*bullet_face_str == 'N') {
			bullet_face = UNIT_FACE_N;
		}
		else if (*bullet_face_str == 'E') {
			bullet_face = UNIT_FACE_E;
		}
		else if (*bullet_face_str == 'W') {
			bullet_face = UNIT_FACE_W;
		}
		else if (*bullet_face_str == 'S') {
			bullet_face = UNIT_FACE_S;
		}
	}
	if (key == "bullet1_face") {
		((ai_common_data_t*)ai_data)->bullet1_face = bullet_face;
	}
	if (key == "bullet2_face") {
		((ai_common_data_t*)ai_data)->bullet2_face = bullet_face;
	}
}

void unit_display(unit_data_t* unit_data, int layer)
{
	shape_data* col_shape = unit_data->col_shape;
	anim_data_t* anim = unit_data->anim;

	int stat = unit_manager_unit_get_anim_stat(unit_data);
	if (stat != -1) {
		int frame_num = -1;
		anim_frame_data_t* frame_data = NULL;
		if (anim->anim_stat_base_list[stat]->tex_layer != layer) return;  // escape other layer

		if (anim->anim_stat_base_list[stat]->type == ANIM_TYPE_STATIC) {
			frame_num = 0;
			frame_data = anim->anim_stat_base_list[stat]->frame_list[frame_num];
		}
		else if (anim->anim_stat_base_list[stat]->type == ANIM_TYPE_DYNAMIC) {
			frame_num = anim->anim_stat_list[stat]->current_frame;
			frame_data = anim->anim_stat_base_list[stat]->frame_list[frame_num];
		}

		if (frame_data) {
			if ((unit_data->stat_timer) && (unit_data->stat_timer[UNIT_STAT_INVINCIBLE] > 0) && ((unit_data->stat_timer[UNIT_STAT_INVINCIBLE] / UNIT_ANIM_BLINK_TIMER) % 2)) {
				// invisible for blink

			}
			else if (col_shape->joint_type == COLLISION_JOINT_TYPE_PIN_ROUND) {
				SDL_Texture* tex = frame_data->tex;
				SDL_Rect* src_rect = &frame_data->src_rect;

				int dst_x = 0, dst_y = 0;
				if (col_shape->b2body) {
					float disp_angle = col_shape->b2body->GetAngle(); // radian
					float sin_val = game_utils_sin(disp_angle);
					float cos_val = game_utils_cos(disp_angle);
					float pin_offset_x = cos_val * col_shape->joint_x - sin_val * col_shape->joint_y;
					float pin_offset_y = sin_val * col_shape->joint_x + cos_val * col_shape->joint_y;

					dst_x = (int)MET2PIX(col_shape->b2body->GetPosition().x) + (int)pin_offset_x - col_shape->joint_x;
					dst_y = (int)MET2PIX(col_shape->b2body->GetPosition().y) + (int)pin_offset_y - col_shape->joint_y;

					SDL_Rect dst_rect = VIEW_STAGE_RECT(dst_x, dst_y, src_rect->w, src_rect->h);
					SDL_Point dst_center = { VIEW_STAGE(col_shape->joint_x), VIEW_STAGE(col_shape->joint_y) };
					if (disp_angle != 0.0f) {
						SDL_RenderCopyEx(g_ren, tex, src_rect, &dst_rect, disp_angle * 180.0f / b2_pi, &dst_center, SDL_FLIP_NONE);
					}
					else {
						SDL_RenderCopy(g_ren, tex, src_rect, &dst_rect);
					}
				}
			}
			else {
				SDL_Texture* tex = frame_data->tex;
				SDL_Rect* src_rect = &frame_data->src_rect;
#ifdef _COLLISION_ENABLE_BOX_2D_
				int dst_x = 0, dst_y = 0;
				if (col_shape->b2body) {
					dst_x = (int)MET2PIX(col_shape->b2body->GetPosition().x);
					dst_y = (int)MET2PIX(col_shape->b2body->GetPosition().y);
				}
				else {
					dst_x = col_shape->x;
					dst_y = col_shape->y;
				}
				SDL_Rect dst_rect = VIEW_STAGE_RECT(dst_x, dst_y, src_rect->w, src_rect->h);
#endif
				int disp_face = UNIT_FACE_W;
				double disp_angle = 0.0;
				SDL_RendererFlip disp_flip;
				if ((col_shape->face_type == UNIT_FACE_TYPE_UD) || (col_shape->face_type == UNIT_FACE_TYPE_ALL)) {
					if (col_shape->face == UNIT_FACE_N) {
						disp_face = UNIT_FACE_N;
						disp_angle = 90.0;
						disp_flip = SDL_FLIP_NONE;
					}
					if (col_shape->face == UNIT_FACE_S) {
						disp_face = UNIT_FACE_S;
						disp_angle = 270.0;
						disp_flip = SDL_FLIP_NONE;
					}
				}

				if ((col_shape->face_type == UNIT_FACE_TYPE_LR) || (col_shape->face_type == UNIT_FACE_TYPE_ALL)) {
					if (col_shape->face == UNIT_FACE_E) {
						disp_face = UNIT_FACE_E;
						disp_angle = 0.0;
						disp_flip = SDL_FLIP_HORIZONTAL;
					}
				}

				if (disp_face != UNIT_FACE_W) {
					SDL_RenderCopyEx(g_ren, tex, src_rect, &dst_rect, disp_angle, NULL, disp_flip);
				}
				else {
					SDL_RenderCopy(g_ren, tex, src_rect, &dst_rect);
				}
			}

			// update sounds
			if ((frame_data->chunk) && (anim->anim_stat_list[stat]->chunk_frame != frame_num)) {
				sound_manager_play(frame_data->chunk, anim->anim_stat_base_list[stat]->snd_channel);
				anim->anim_stat_list[stat]->chunk_frame = frame_num;
			}
		}
	}
}

void unit_manager_update_unit_friction(unit_data_t* unit_data)
{
	unit_data->col_shape->vec_x = unit_data->col_shape->b2body->GetLinearVelocity().x;
	unit_data->col_shape->vec_y = unit_data->col_shape->b2body->GetLinearVelocity().y;

	bool update_flg = false;
	float abs_vec_x = ABS(unit_data->col_shape->vec_x);
	if (abs_vec_x > 0) {
		float delta_vec_x = unit_data->col_shape->vec_x > 0 ? -unit_data->col_shape->vec_x_delta : unit_data->col_shape->vec_x_delta;
		unit_data->col_shape->vec_x += (g_stage_data->friction_coef * delta_vec_x * g_delta_time);
		if (ABS(unit_data->col_shape->vec_x) < FLOAT_NEAR_ZERO) unit_data->col_shape->vec_x = 0.0f;
		update_flg = true;
	}

	float abs_vec_y = ABS(unit_data->col_shape->vec_y);
	if (abs_vec_y > 0) {
		float delta_vec_y = unit_data->col_shape->vec_y > 0 ? -unit_data->col_shape->vec_y_delta : unit_data->col_shape->vec_y_delta;
		unit_data->col_shape->vec_y += (g_stage_data->friction_coef * delta_vec_y * g_delta_time);
		if (ABS(unit_data->col_shape->vec_y) < FLOAT_NEAR_ZERO) unit_data->col_shape->vec_y = 0.0f;
		update_flg = true;
	}

	if (update_flg) {
		b2Vec2 new_vec(unit_data->col_shape->vec_x, unit_data->col_shape->vec_y);
		unit_data->col_shape->b2body->SetLinearVelocity(new_vec);
	}
}

void unit_manager_unit_move(unit_data_t* unit_data, float vec_x, float vec_y, float speed)
{
	unit_data->col_shape->vec_x += vec_x * speed;
	unit_data->col_shape->vec_y += vec_y * speed;

	float x_max = unit_data->col_shape->vec_x_max * speed;
	if (ABS(unit_data->col_shape->vec_x) > x_max) {
		unit_data->col_shape->vec_x = unit_data->col_shape->vec_x > 0.0f ? x_max : (-x_max);
	}
	float y_max = unit_data->col_shape->vec_y_max * speed;
	if (ABS(unit_data->col_shape->vec_y) > y_max) {
		unit_data->col_shape->vec_y = unit_data->col_shape->vec_y > 0.0f ? y_max : (-y_max);
	}

#ifdef _COLLISION_ENABLE_BOX_2D_
	b2Vec2 new_vec((float32)unit_data->col_shape->vec_x, (float32)unit_data->col_shape->vec_y);
	unit_data->col_shape->b2body->SetLinearVelocity(new_vec);
#endif

	// set face
	if (vec_x > 0.0f) {
		collision_manager_set_face(unit_data->col_shape, unit_data->base->col_shape, unit_data->base->anim->base_w, unit_data->base->anim->base_h, UNIT_FACE_E);
	}
	else if (vec_x < 0.0f) {
		collision_manager_set_face(unit_data->col_shape, unit_data->base->col_shape, unit_data->base->anim->base_w, unit_data->base->anim->base_h, UNIT_FACE_W);
	}

	if (ABS(vec_y) > ABS(vec_x))
	{
		if (vec_y > 0.0f) {
			collision_manager_set_face(unit_data->col_shape, unit_data->base->col_shape, unit_data->base->anim->base_w, unit_data->base->anim->base_h, UNIT_FACE_S);
		}
		else if (vec_y < 0.0f) {
			collision_manager_set_face(unit_data->col_shape, unit_data->base->col_shape, unit_data->base->anim->base_w, unit_data->base->anim->base_h, UNIT_FACE_N);
		}
	}

	if (unit_data->anim->anim_stat_base_list[ANIM_STAT_MOVE]->frame_size > 0) {
		unit_manager_unit_set_anim_stat(unit_data, ANIM_STAT_FLAG_MOVE);
	}
}

int unit_manager_unit_get_anim_stat(unit_data_t* unit_data)
{
	anim_data_t* anim = unit_data->anim;
	int stat = -1;
	for (int si = ANIM_STAT_END - 1; si >= ANIM_STAT_IDLE; si--) {
		int stat_flg = 0x00000001 << si;
		if (anim->stat & stat_flg) {
			stat = si;
			break;
		}
	}
	return stat;
}

void unit_manager_unit_set_anim_stat(unit_data_t* unit_data, int stat)
{
	if (unit_data->anim->stat != stat) {
		unit_data->anim->stat = stat;

		// update col_shape->stat
		if ((unit_data->anim->stat == ANIM_STAT_FLAG_DIE) || (unit_data->anim->stat == ANIM_STAT_FLAG_SPAWN) || (unit_data->anim->stat == ANIM_STAT_FLAG_HIDE)) {
			unit_data->col_shape->stat = COLLISION_STAT_DISABLE;
		}
		else {
			unit_data->col_shape->stat = COLLISION_STAT_ENABLE;
		}

		// reset stat frame
		int stat = unit_manager_unit_get_anim_stat(unit_data);
		if (stat != -1) {
			unit_data->anim->anim_stat_list[stat]->chunk_frame = -1;
		}
	}
}

void unit_manager_get_position(unit_data_t* unit_data, int* x, int* y)
{
	*x = unit_data->col_shape->x;
	*y = unit_data->col_shape->y;
}

float unit_manager_get_distance(unit_data_t* main_unit, unit_data_t* target_unit, int* p_x, int* p_y)
{
	int main_x = 0, main_y = 0;
	if (main_unit->col_shape->type & COLLISION_TYPE_BOX) {
		int w = ((shape_box_data*)(main_unit->col_shape))->w;
		int h = ((shape_box_data*)(main_unit->col_shape))->h;
		main_x = main_unit->col_shape->x + main_unit->col_shape->offset_x + w / 2;
		main_y = main_unit->col_shape->y + main_unit->col_shape->offset_y + h / 2;
	}
	else if (main_unit->col_shape->type & COLLISION_TYPE_ROUND) {
		main_x = main_unit->col_shape->x;
		main_y = main_unit->col_shape->y;
	}

	int target_x = 0, target_y = 0;
	if (target_unit->col_shape->type & COLLISION_TYPE_BOX) {
		int w = ((shape_box_data*)(target_unit->col_shape))->w;
		int h = ((shape_box_data*)(target_unit->col_shape))->h;
		target_x = target_unit->col_shape->x + target_unit->col_shape->offset_x + w / 2;
		target_y = target_unit->col_shape->y + target_unit->col_shape->offset_y + h / 2;
	}
	else if (target_unit->col_shape->type & COLLISION_TYPE_ROUND) {
		target_x = target_unit->col_shape->x;
		target_y = target_unit->col_shape->y;
	}

	int dist_x = target_x - main_x;
	int dist_y = target_y - main_y;
	float length = sqrtf(float(dist_x * dist_x + dist_y * dist_y));

	if (p_x) *p_x = dist_x;
	if (p_y) *p_y = dist_y;
	return length;
}

int unit_manager_get_face(unit_data_t* unit_data)
{
	if (unit_data->col_shape) return unit_data->col_shape->face;
	else return UNIT_FACE_W;
}

int unit_manager_get_face_relative(unit_data_t* unit_data, int original_face)
{
	int relative_face = UNIT_FACE_W;
	bool revers = false;
	if (unit_data->col_shape) {
		if (unit_data->col_shape->face_type == UNIT_FACE_TYPE_ALL) {
			if (unit_data->col_shape->face == UNIT_FACE_E) {
				if (original_face == UNIT_FACE_E) relative_face = UNIT_FACE_W;
				else if (original_face == UNIT_FACE_W) relative_face = UNIT_FACE_E;
				else relative_face = original_face;
			}
			else if (unit_data->col_shape->face == UNIT_FACE_N) {
				if (original_face == UNIT_FACE_N) relative_face = UNIT_FACE_E;
				else if (original_face == UNIT_FACE_E) relative_face = UNIT_FACE_S;
				else if (original_face == UNIT_FACE_W) relative_face = UNIT_FACE_N;
				else if (original_face == UNIT_FACE_S) relative_face = UNIT_FACE_W;
			}
			else if (unit_data->col_shape->face == UNIT_FACE_S) {
				if (original_face == UNIT_FACE_N) relative_face = UNIT_FACE_W;
				else if (original_face == UNIT_FACE_E) relative_face = UNIT_FACE_N;
				else if (original_face == UNIT_FACE_W) relative_face = UNIT_FACE_S;
				else if (original_face == UNIT_FACE_S) relative_face = UNIT_FACE_E;
			}
			else {
				relative_face = original_face;
			}
		}
		else if (unit_data->col_shape->face_type == UNIT_FACE_TYPE_LR) {
			if (unit_data->col_shape->face == UNIT_FACE_E) { // flip LR
				if (original_face == UNIT_FACE_E) relative_face = UNIT_FACE_W;
				else if (original_face == UNIT_FACE_W) relative_face = UNIT_FACE_E;
			}
		}
		else if (unit_data->col_shape->face_type == UNIT_FACE_TYPE_UD) {
			if (unit_data->col_shape->face == UNIT_FACE_S) { // flip UD
				if (original_face == UNIT_FACE_S) relative_face = UNIT_FACE_N;
				else if (original_face == UNIT_FACE_N) relative_face = UNIT_FACE_S;
			}
		}
		else {
			relative_face = original_face;
		}
	}
	return relative_face;
}

int unit_manager_get_face_other_side(unit_data_t* unit_data)
{
	int other_face = UNIT_FACE_W;
	if (unit_data->col_shape) {
		if (unit_data->col_shape->face == UNIT_FACE_N) other_face = UNIT_FACE_S;
		else if (unit_data->col_shape->face == UNIT_FACE_E) other_face = UNIT_FACE_W;
		else if (unit_data->col_shape->face == UNIT_FACE_W) other_face = UNIT_FACE_E;
		else if (unit_data->col_shape->face == UNIT_FACE_S) other_face = UNIT_FACE_N;
	}
	return other_face;
}

void unit_manager_get_face_velocity(float* vec_x, float* vec_y, int face, float abs_velocity, int bullet_num)
{
	if (bullet_num == UNIT_BULLET_NUM_NONE) {
		// for invisible bullet

	}
	else if (bullet_num == UNIT_BULLET_NUM_SINGLE) {
		if (face == UNIT_FACE_N) { *vec_x = 0.0f; *vec_y = -abs_velocity; }
		else if (face == UNIT_FACE_E) { *vec_x = abs_velocity;  *vec_y = 0.0f; }
		else if (face == UNIT_FACE_W) { *vec_x = -abs_velocity; *vec_y = 0.0f; }
		else if (face == UNIT_FACE_S) { *vec_x = 0.0f;          *vec_y = abs_velocity; }
	}
	else if (bullet_num == UNIT_BULLET_NUM_TRIPLE) {
		float val_1_r2 = 0.7071067f;

		if (face == UNIT_FACE_N) {
			*vec_x = 0.0f;
			*vec_y = -abs_velocity;
			*(vec_x + 1) = -val_1_r2 * abs_velocity;
			*(vec_y + 1) = -val_1_r2 * abs_velocity;
			*(vec_x + 2) =  val_1_r2 * abs_velocity;
			*(vec_y + 2) = -val_1_r2 * abs_velocity;
		}
		else if (face == UNIT_FACE_E) {
			*vec_x = abs_velocity;
			*vec_y = 0.0f; 
			*(vec_x + 1) =  val_1_r2 * abs_velocity;
			*(vec_y + 1) = -val_1_r2 * abs_velocity;
			*(vec_x + 2) =  val_1_r2 * abs_velocity;
			*(vec_y + 2) =  val_1_r2 * abs_velocity;
		}
		else if (face == UNIT_FACE_W) {
			*vec_x = -abs_velocity;
			*vec_y = 0.0f;
			*(vec_x + 1) = -val_1_r2 * abs_velocity;
			*(vec_y + 1) = -val_1_r2 * abs_velocity;
			*(vec_x + 2) = -val_1_r2 * abs_velocity;
			*(vec_y + 2) =  val_1_r2 * abs_velocity;
		}
		else if (face == UNIT_FACE_S) {
			*vec_x = 0.0f;
			*vec_y = abs_velocity;
			*(vec_x + 1) = -val_1_r2 * abs_velocity;
			*(vec_y + 1) =  val_1_r2 * abs_velocity;
			*(vec_x + 2) =  val_1_r2 * abs_velocity;
			*(vec_y + 2) =  val_1_r2 * abs_velocity;
		}
	}
}

void unit_manager_get_bullet_start_pos(unit_data_t* unit_data, unit_data_t* unit_bullet_data, int bullet_num, int face, int* x, int* y)
{
	if (bullet_num == UNIT_BULLET_NUM_NONE) {
		// for invisible bullet

	}
	else if ((bullet_num == UNIT_BULLET_NUM_SINGLE)
		|| (bullet_num == UNIT_BULLET_NUM_DOUBLE) || (bullet_num == UNIT_BULLET_NUM_TRIPLE))
	{
		int bullet_offset_x = 0, bullet_offset_y = 0;
		int bullet_w = 0, bullet_h = 0;
		if (unit_bullet_data->col_shape->type == COLLISION_TYPE_BOX_D) {
			bullet_offset_x = unit_bullet_data->col_shape->offset_x;
			bullet_offset_y = unit_bullet_data->col_shape->offset_y;
			bullet_w = ((shape_box_data*)unit_bullet_data->col_shape)->w;
			bullet_h = ((shape_box_data*)unit_bullet_data->col_shape)->h;
		}

		if (unit_data->col_shape->type == COLLISION_TYPE_BOX_D) {
			int w, h;
			w = ((shape_box_data*)unit_data->col_shape)->w;
			h = ((shape_box_data*)unit_data->col_shape)->h;

			for (int i = 0; i < bullet_num; i++) {
				if (face == UNIT_FACE_N) {
					*x = unit_data->col_shape->x + unit_data->col_shape->offset_x + w / 2 - bullet_h / 2;
					*y = unit_data->col_shape->y + unit_data->col_shape->offset_y - 1 - bullet_h;
				}
				else if (face == UNIT_FACE_S) {
					*x = unit_data->col_shape->x + unit_data->col_shape->offset_x + w / 2 - bullet_h / 2;
					*y = unit_data->col_shape->y + unit_data->col_shape->offset_y + h + 1;
				}
				else if (face == UNIT_FACE_W) {
					*x = unit_data->col_shape->x + unit_data->col_shape->offset_x - 1 - bullet_w;
					*y = unit_data->col_shape->y + unit_data->col_shape->offset_y + h / 2 - bullet_h / 2;
				}
				else if (face == UNIT_FACE_E) {
					*x = unit_data->col_shape->x + unit_data->col_shape->offset_x + w;
					*y = unit_data->col_shape->y + unit_data->col_shape->offset_y + h / 2 - bullet_h / 2;
				}
				x++;
				y++;
			}
		}
		else if (unit_data->col_shape->type == COLLISION_TYPE_ROUND_D) {
			int r = ((shape_round_data*)unit_data->col_shape)->r;
			for (int i = 0; i < bullet_num; i++) {
				if (face == UNIT_FACE_N) {
					*x = unit_data->col_shape->x + unit_data->col_shape->offset_x;
					*y = unit_data->col_shape->y + unit_data->col_shape->offset_y - 1;
				}
				else if (face == UNIT_FACE_S) {
					*x = unit_data->col_shape->x + unit_data->col_shape->offset_x;
					*y = unit_data->col_shape->y + unit_data->col_shape->offset_y + r + 1;
				}
				else if (face == UNIT_FACE_W) {
					*x = unit_data->col_shape->x + unit_data->col_shape->offset_x - 1;
					*y = unit_data->col_shape->y + unit_data->col_shape->offset_y;
				}
				else if (face == UNIT_FACE_E) {
					*x = unit_data->col_shape->x + unit_data->col_shape->offset_x + r + 1;
					*y = unit_data->col_shape->y + unit_data->col_shape->offset_y;
				}
				x++;
				y++;
			}
		}
	}
}

void unit_manager_get_spawn_items_pos(unit_data_t* spawner_unit, unit_data_t* avoid_unit, int item_num, int* x, int* y) {
	int spawn_x = spawner_unit->col_shape->x;
	int spawn_y = spawner_unit->col_shape->y;

	if (avoid_unit) {
		int avoid_x = avoid_unit->col_shape->x;
		int avoid_y = avoid_unit->col_shape->y;

		// same position
		if ((avoid_x == spawn_x) && (avoid_y == spawn_y)) {
			// opposite side of current velocity
			float vec_x = avoid_unit->col_shape->vec_x;
			float vec_y = avoid_unit->col_shape->vec_y;
			int direction_x = vec_x < 0 ? -1 : 1;
			int direction_y = vec_y < 0 ? -1 : 1;

			for (int i = 0; i < item_num; i++) {
				int x_offset = game_utils_random_gen(1, 0);
				int y_offset = game_utils_random_gen(1, 0);
				x[i] = spawn_x + direction_x * (x_offset + 1) * g_tile_width;
				y[i] = spawn_y + direction_y * (y_offset + 1) * g_tile_height;
			}
		}
		// avoid near side
		else {
			int direction_x = avoid_x > spawn_x ? -1 : 1;
			int direction_y = avoid_y > spawn_y ? -1 : 1;

			for (int i = 0; i < item_num; i++) {
				int x_offset = game_utils_random_gen(4, 1);
				int y_offset = game_utils_random_gen(4, 1);
				x[i] = spawn_x + direction_x * x_offset * g_tile_width / 2;
				y[i] = spawn_y + direction_y * y_offset * g_tile_height / 2;
			}
		}
	}
	else {
		for (int i = 0; i < item_num; i++) {
			int x_offset = game_utils_random_gen(6, 0);
			int y_offset = game_utils_random_gen(6, 0);
			x[i] = spawn_x + x_offset * g_tile_width / 4;
			y[i] = spawn_y + y_offset * g_tile_height / 4;
		}
	}

	int min_x = g_tile_width;
	int max_x = (g_map_x_max - 2) * g_tile_width;
	int min_y = g_tile_height;
	int max_y = (g_map_y_max - 2) * g_tile_height;
	for (int i = 0; i < item_num; i++) {
		x[i] = MAX(min_x, MIN(max_x, x[i]));
		y[i] = MAX(min_y, MIN(max_y, y[i]));
	}
}

void unit_manager_get_spawn_items_pos_under_foot(unit_data_t* spawner_unit, int item_num, int* x, int* y) {
	int spawn_x = spawner_unit->col_shape->x;
	int spawn_y = spawner_unit->col_shape->y;

	for (int i = 0; i < item_num; i++) {
		int x_offset = game_utils_random_gen(3, 0);
		x[i] = spawn_x + x_offset * g_tile_width / 4;
		y[i] = spawn_y + g_tile_height / 4;
	}

	int min_x = g_tile_width;
	int max_x = (g_map_x_max - 2) * g_tile_width;
	int min_y = g_tile_height;
	int max_y = (g_map_y_max - 2) * g_tile_height;
	for (int i = 0; i < item_num; i++) {
		x[i] = MAX(min_x, MIN(max_x, x[i]));
		y[i] = MAX(min_y, MIN(max_y, y[i]));
	}
}

void unit_manager_get_spawn_items_pos_for_target(unit_data_t* spawner_unit, unit_data_t* target_unit, int item_num, int* x, int* y) {
	int spawn_x = spawner_unit->col_shape->x;
	int spawn_y = spawner_unit->col_shape->y;

	if (target_unit) {
		int target_x = target_unit->col_shape->x;
		int target_y = target_unit->col_shape->y;

		int direction_x = target_x > spawn_x ? 1 : -1;
		int direction_y = target_y > spawn_y ? 1 : -1;

		for (int i = 0; i < item_num; i++) {
			int x_offset = game_utils_random_gen(3, 0);
			int y_offset = game_utils_random_gen(3, 0);
			x[i] = spawn_x + direction_x * x_offset * (g_tile_width / 4);
			y[i] = spawn_y + direction_y * y_offset * (g_tile_height / 4);
		}
	}

	int min_x = g_tile_width;
	int max_x = (g_map_x_max - 2) * g_tile_width;
	int min_y = g_tile_height;
	int max_y = (g_map_y_max - 2) * g_tile_height;
	for (int i = 0; i < item_num; i++) {
		x[i] = MAX(min_x, MIN(max_x, x[i]));
		y[i] = MAX(min_y, MIN(max_y, y[i]));
	}
}

void unit_manager_enemy_drop_item(unit_enemy_data_t* unit_data) {
	if (unit_data->level == 1) {
		int drop_judge = game_utils_random_gen(g_stage_data->current_section_data->level1_drop_rate - 1, 0);
		if ((drop_judge == 1) // hit on 1/N
			|| (g_stage_data->level1_drop_judge_count >= (g_stage_data->current_section_data->level1_drop_rate - 1))) {

			// drop random
			int max = (int)g_stage_data->current_section_data->drop_items_id_list.size() - 1;
			int item_id = (max <= 0) ? 0 : game_utils_random_gen(max, 0);
			int spawn_id = unit_manager_create_items(unit_data->col_shape->x, unit_data->col_shape->y, g_stage_data->current_section_data->drop_items_id_list[item_id]);

			unit_items_data_t* new_item = unit_manager_get_items(spawn_id);
			std::string star_path = "units/effect/star/star.unit";
			int effect_id = unit_manager_create_effect(new_item->col_shape->x, new_item->col_shape->y, unit_manager_search_effect(star_path));
			unit_manager_effect_set_trace_unit(effect_id, (unit_data_t*)new_item);

			g_stage_data->level1_drop_judge_count = 0;
		}
		else {
			g_stage_data->level1_drop_judge_count += 1;
		}
	}
	else if ((unit_data->level == 2) && (unit_data->col_shape)) {
		if (unit_data->drop_item >= 0) {
			unit_manager_create_items(unit_data->col_shape->x, unit_data->col_shape->y, unit_data->drop_item);
		}
		else {
			// drop random
			int max = (int)g_stage_data->current_section_data->drop_items_id_list.size() - 1;
			int item_id = (max <= 0) ? 0 : game_utils_random_gen(max, 0);
			unit_manager_create_items(unit_data->col_shape->x, unit_data->col_shape->y, g_stage_data->current_section_data->drop_items_id_list[item_id]);
		}
	}
}
