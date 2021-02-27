#include <fstream>
#include "game_common.h"
#include "unit_manager.h"

#include "game_timer.h"
#include "game_log.h"
#include "game_utils.h"
#include "game_window.h"
#include "game_event.h"

#include "resource_manager.h"
#include "gui_common.h"
#include "stage_manager.h"
#include "map_manager.h"
#include "sound_manager.h"
#include "ai_manager.h"
#include "scene_play_stage.h"

static char tmp_char_buf[GAME_UTILS_STRING_CHAR_BUF_SIZE];

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
	char key[GAME_UTILS_STRING_NAME_BUF_SIZE];
	char value[GAME_UTILS_STRING_NAME_BUF_SIZE];
	game_utils_split_key_value((char*)line.c_str(), key, value);

	if (STRCMP_EQ(key,"type")) {
		if (STRCMP_EQ(value,"BOX")) {
			*col_shape = (shape_data *)collision_manager_new_shape_box();
		}
		else if (STRCMP_EQ(value,"ROUND")) {
			*col_shape = (shape_data *)collision_manager_new_shape_round();
		}
		else if (STRCMP_EQ(value,"BOX_S")) {
			*col_shape = (shape_data*)collision_manager_new_shape_box(COLLISION_ID_STATIC_SHAPE);
		}
		else if (STRCMP_EQ(value,"ROUND_S")) {
			*col_shape = (shape_data*)collision_manager_new_shape_round(COLLISION_ID_STATIC_SHAPE);
		}
		return;
	}
	if (STRCMP_EQ(key, "group")) {
		collision_manager_set_group(*col_shape, value);
		return;
	}
	if (STRCMP_EQ(key, "group_option")) {
		collision_manager_set_group_option(*col_shape, value);
		return;
	}

	if (STRCMP_EQ(key,"x")) {
		(*col_shape)->offset_x = atoi(value);
		return;
	}
	if (STRCMP_EQ(key,"y")) {
		(*col_shape)->offset_y = atoi(value);
		return;
	}

	if (STRCMP_EQ(key,"face_type")) {
		if (STRCMP_EQ(value,"LR")) {
			(*col_shape)->face_type = UNIT_FACE_TYPE_LR;
		}
		else if (STRCMP_EQ(value,"UD")) {
			(*col_shape)->face_type = UNIT_FACE_TYPE_UD;
		}
		else if (STRCMP_EQ(value,"ALL")) {
			(*col_shape)->face_type = UNIT_FACE_TYPE_ALL;
		}
		else {
			(*col_shape)->face_type = UNIT_FACE_TYPE_NONE;
		}
		return;
	}

	if (STRCMP_EQ(key,"vec_x_max")) {
		(*col_shape)->vec_x_max = (float)atof(value);
		return;
	}
	if (STRCMP_EQ(key,"vec_y_max")) {
		(*col_shape)->vec_y_max = (float)atof(value);
		return;
	}
	if (STRCMP_EQ(key,"vec_x_delta")) {
		(*col_shape)->vec_x_delta = (float)atof(value);
		return;
	}
	if (STRCMP_EQ(key,"vec_y_delta")) {
		(*col_shape)->vec_y_delta = (float)atof(value);
		return;
	}

	// joint
	if (STRCMP_EQ(key,"joint_type")) {
		if (STRCMP_EQ(value,"PIN")) {
			(*col_shape)->joint_type = COLLISION_JOINT_TYPE_PIN;
		}
		else if (STRCMP_EQ(value,"PIN_ROUND")) {
			(*col_shape)->joint_type = COLLISION_JOINT_TYPE_PIN_ROUND;
		}
		else {
			(*col_shape)->joint_type = COLLISION_JOINT_TYPE_NONE;
		}
		return;
	}
	if (STRCMP_EQ(key, "joint_x")) {
		(*col_shape)->joint_x = atoi(value);
		return;
	}
	if (STRCMP_EQ(key, "joint_y")) {
		(*col_shape)->joint_y = atoi(value);
		return;
	}
	if (STRCMP_EQ(key, "joint_val1")) {
		(*col_shape)->joint_val1 = atoi(value);
		return;
	}

	if (STRCMP_EQ(key, "w")) {
		((shape_box_data*)*col_shape)->w = atoi(value);
		return;
	}
	if (STRCMP_EQ(key, "h")) {
		((shape_box_data*)*col_shape)->h = atoi(value);
		return;
	}
	if (STRCMP_EQ(key, "r")) {
		((shape_round_data*)*col_shape)->r = atoi(value);
		return;
	}
}

void load_anim(std::string& line, anim_data_t* anim)
{
	char key[GAME_UTILS_STRING_NAME_BUF_SIZE];
	char value[GAME_UTILS_STRING_CHAR_BUF_SIZE];
	game_utils_split_key_value((char*)line.c_str(), key, value);

	if (STRCMP_EQ(key,"base_w")) {
		anim->base_w = atoi(value);
		return;
	}
	if (STRCMP_EQ(key,"base_h")) {
		anim->base_h = atoi(value);
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

	//std::string stat_str = line.substr(index_start, index_end - index_start);
	if (game_utils_string_copy_n(tmp_char_buf, &line[index_start], index_end - index_start) != 0) {
		LOG_ERROR("Error: load_anim() failed copy stat\n");
		return;
	}

	int stat_val;
	if (STRCMP_EQ(tmp_char_buf,"IDLE")) {
		stat_val = ANIM_STAT_IDLE;
	}
	else if (STRCMP_EQ(tmp_char_buf,"MOVE")) {
		stat_val = ANIM_STAT_MOVE;
	}
	else if (STRCMP_EQ(tmp_char_buf,"ATTACK") || STRCMP_EQ(tmp_char_buf,"ATTACK1")) {
		stat_val = ANIM_STAT_ATTACK1;
	}
	else if (STRCMP_EQ(tmp_char_buf,"ATTACK2")) {
		stat_val = ANIM_STAT_ATTACK2;
	}
	else if (STRCMP_EQ(tmp_char_buf,"DEFENCE")) {
		stat_val = ANIM_STAT_DEFENCE;
	}
	else if (STRCMP_EQ(tmp_char_buf,"DIE")) {
		stat_val = ANIM_STAT_DIE;
	}
	else if (STRCMP_EQ(tmp_char_buf,"SPAWN")) {
		stat_val = ANIM_STAT_SPAWN;
	}
	else if (STRCMP_EQ(tmp_char_buf,"HIDE")) {
		stat_val = ANIM_STAT_HIDE;
	}
	else {
		LOG_ERROR("load_anim %s error\n", line.c_str());
		return;
	}

	// set anim path
	char* path_c_str = game_utils_string_new();
	game_utils_string_copy(path_c_str, line.c_str());
	anim->anim_stat_base_list[stat_val]->obj = (void*)path_c_str;
}

void load_ai(std::string& line, ai_data_t* ai_data)
{
	char key[GAME_UTILS_STRING_NAME_BUF_SIZE];
	char value[GAME_UTILS_STRING_CHAR_BUF_SIZE];
	game_utils_split_key_value((char*)line.c_str(), key, value);

	if (STRCMP_EQ(key,"type")) {
		ai_data->type = ai_manager_get_ai_type(value);
		return;
	}

	if (STRCMP_EQ(key,"val1")) {
		ai_data->val1 = atoi(value);
		return;
	}
	if (STRCMP_EQ(key,"val2")) {
		ai_data->val2 = atoi(value);
		return;
	}
	if (STRCMP_EQ(key,"val3")) {
		ai_data->val3 = atoi(value);
		return;
	}
	if (STRCMP_EQ(key,"val4")) {
		ai_data->val4 = atoi(value);
		return;
	}
}

void load_bullet(std::string& line, ai_data_t** bullet_data)
{
	char key[GAME_UTILS_STRING_NAME_BUF_SIZE];
	char value[GAME_UTILS_STRING_CHAR_BUF_SIZE];
	game_utils_split_key_value((char*)line.c_str(), key, value);

	if (STRCMP_EQ(key,"bullet1_ai")) {
		bullet_data[0] = ai_manager_new_ai_base_data();
		bullet_data[0]->type = AI_TYPE_BULLET;

		char* path_c_str = game_utils_string_new();
		game_utils_string_copy(path_c_str, value);
		((ai_bullet_t*)bullet_data[0])->obj = (void*)path_c_str;
		return;
	}
	if (STRCMP_EQ(key,"bullet2_ai")) {
		bullet_data[1] = ai_manager_new_ai_base_data();
		bullet_data[1]->type = AI_TYPE_BULLET;

		char* path_c_str = game_utils_string_new();
		game_utils_string_copy(path_c_str, value);
		((ai_bullet_t*)bullet_data[1])->obj = (void*)path_c_str;
		return;
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
		else if (anim->anim_stat_base_list[stat]->type & ANIM_TYPE_DRAW) {
			frame_num = anim->anim_stat_list[stat]->current_frame;
			frame_data = anim->anim_stat_base_list[stat]->frame_list[frame_num];
		}

		if (frame_data) {
			if ((unit_data->stat_timer) && (unit_data->stat_timer[UNIT_STAT_INVINCIBLE] > 0) && ((unit_data->stat_timer[UNIT_STAT_INVINCIBLE] / UNIT_ANIM_BLINK_TIMER) % 2)) {
				// invisible for blink

			}
			else if (anim->anim_stat_base_list[stat]->type & ANIM_TYPE_DRAW) {
				SDL_SetRenderDrawColor(g_ren,
					anim->anim_stat_base_list[stat]->color_r, anim->anim_stat_base_list[stat]->color_g, anim->anim_stat_base_list[stat]->color_b, anim->anim_stat_base_list[stat]->color_a);

				if (anim->anim_stat_base_list[stat]->type == ANIM_TYPE_DRAW_RECT) {
					int w = ((shape_box_data*)unit_data->col_shape)->w;
					int h = ((shape_box_data*)unit_data->col_shape)->h;
					SDL_Rect rect = VIEW_STAGE_RECT(unit_data->col_shape->x, unit_data->col_shape->y, w, h);
					SDL_RenderDrawRect(g_ren, &rect);
				}
				else if (anim->anim_stat_base_list[stat]->type == ANIM_TYPE_DRAW_CIRCLE) {
					collision_manager_display_circle(unit_data->col_shape);
				}
				else if (anim->anim_stat_base_list[stat]->type == ANIM_TYPE_DRAW_RECT_FILL) {
					int w = ((shape_box_data*)unit_data->col_shape)->w;
					int h = ((shape_box_data*)unit_data->col_shape)->h;
					SDL_Rect rect = VIEW_STAGE_RECT(unit_data->col_shape->x, unit_data->col_shape->y, w, h);
					SDL_RenderFillRect(g_ren, &rect);
				}
			}
			else {
				SDL_Rect* src_rect = &frame_data->src_rect;
#ifdef _COLLISION_ENABLE_BOX_2D_
				int dst_x = 0, dst_y = 0;
				unit_manager_get_position(unit_data, &dst_x, &dst_y);

				SDL_Point* p_dst_center = NULL;
				SDL_Point dst_center;
				if ((col_shape->b2body) && (col_shape->b2body->GetAngle() != 0.0f)) {
					if (col_shape->type & COLLISION_TYPE_BOX) {
						dst_center.x = VIEW_STAGE(col_shape->offset_x + ((shape_box_data*)col_shape)->w / 2);
						dst_center.y = VIEW_STAGE(col_shape->offset_y + ((shape_box_data*)col_shape)->h / 2);
						p_dst_center = &dst_center; // local center
					}
					else if (col_shape->type & COLLISION_TYPE_ROUND) {
						dst_center.x = VIEW_STAGE(col_shape->offset_x);
						dst_center.y = VIEW_STAGE(col_shape->offset_y);
						p_dst_center = &dst_center; // local center
					}
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

				if (col_shape->joint_type == COLLISION_JOINT_TYPE_PIN_ROUND) {
					if (col_shape->b2body) {
						disp_face = UNIT_FACE_NONE;
						disp_angle = col_shape->b2body->GetAngle() * 180.0 / b2_pi;
						disp_flip = SDL_FLIP_NONE;
					}
				}

				if (disp_face != UNIT_FACE_W) {
					GUI_RenderCopyEx(frame_data->res_img, src_rect, &dst_rect, disp_angle, p_dst_center, disp_flip);
				}
				else {
					GUI_RenderCopy(frame_data->res_img, src_rect, &dst_rect);
				}
			}

			// update sounds
			if ((frame_data->res_chunk) && (anim->anim_stat_list[stat]->chunk_frame != frame_num)) {
				sound_manager_set(frame_data->res_chunk, anim->anim_stat_base_list[stat]->snd_channel);
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
	if ((unit_data->col_shape->face_type == UNIT_FACE_TYPE_LR) || (unit_data->col_shape->face_type == UNIT_FACE_TYPE_ALL)) {
		if (vec_x > 0.0f) {
			collision_manager_set_face(unit_data->col_shape, unit_data->base->col_shape, unit_data->base->anim->base_w, unit_data->base->anim->base_h, UNIT_FACE_E);
			collision_manager_set_angle(unit_data->col_shape, b2_pi);
		}
		else if (vec_x < 0.0f) {
			collision_manager_set_face(unit_data->col_shape, unit_data->base->col_shape, unit_data->base->anim->base_w, unit_data->base->anim->base_h, UNIT_FACE_W);
			collision_manager_set_angle(unit_data->col_shape, 0);
		}
	}

	if ((unit_data->col_shape->face_type == UNIT_FACE_TYPE_UD) || (unit_data->col_shape->face_type == UNIT_FACE_TYPE_ALL)) {
		if (ABS(vec_y) > ABS(vec_x))
		{
			if (vec_y > 0.0f) {
				collision_manager_set_face(unit_data->col_shape, unit_data->base->col_shape, unit_data->base->anim->base_w, unit_data->base->anim->base_h, UNIT_FACE_S);
				collision_manager_set_angle(unit_data->col_shape, b2_pi * 3.0f / 2.0f);
			}
			else if (vec_y < 0.0f) {
				collision_manager_set_face(unit_data->col_shape, unit_data->base->col_shape, unit_data->base->anim->base_w, unit_data->base->anim->base_h, UNIT_FACE_N);
				collision_manager_set_angle(unit_data->col_shape, b2_pi / 2.0f);
			}
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
			unit_data->anim->anim_stat_list[stat]->current_time = 0;
			unit_data->anim->anim_stat_list[stat]->current_frame = 0;
			unit_data->anim->anim_stat_list[stat]->chunk_frame = -1;
		}
	}
}

void unit_manager_get_position(unit_data_t* unit_data, int* x, int* y)
{
	int pos_x = unit_data->col_shape->x;
	int pos_y = unit_data->col_shape->y;

	if ((unit_data->col_shape->type & COLLISION_TYPE_BOX) || (unit_data->col_shape->type & COLLISION_TYPE_ROUND)) {
		if (unit_data->col_shape->b2body) {
			float disp_angle = unit_data->col_shape->b2body->GetAngle(); // radian
			if (disp_angle != 0.0f) {
				b2Transform old_xf = unit_data->col_shape->b2body->GetTransform();
				b2Vec2 old_center = unit_data->col_shape->b2body->GetWorldCenter();
				float sin_val = game_utils_sin(-disp_angle);
				float cos_val = game_utils_cos(-disp_angle);
				float vec_x = old_xf.p.x - old_center.x;
				float vec_y = old_xf.p.y - old_center.y;
				float new_x = old_center.x + cos_val * vec_x - sin_val * vec_y;
				float new_y = old_center.y + sin_val * vec_x + cos_val * vec_y;
				pos_x = (int)MET2PIX(new_x);
				pos_y = (int)MET2PIX(new_y);
			}
		}
	}

	*x = pos_x;
	*y = pos_y;
}

void unit_manager_get_center_position(unit_data_t* unit_data, int* x, int* y)
{
	int pos_x, pos_y;
	unit_manager_get_position(unit_data, &pos_x, &pos_y);

	if (unit_data->col_shape->type & COLLISION_TYPE_BOX) {
		int w = ((shape_box_data*)(unit_data->col_shape))->w;
		int h = ((shape_box_data*)(unit_data->col_shape))->h;
		*x = pos_x + unit_data->col_shape->offset_x + w / 2;
		*y = pos_y + unit_data->col_shape->offset_y + h / 2;
	}
	else if (unit_data->col_shape->type & COLLISION_TYPE_ROUND) {
		*x = pos_x + unit_data->col_shape->offset_x;
		*y = pos_y + unit_data->col_shape->offset_y;
	}
	else {
		*x = unit_data->col_shape->x;
		*y = unit_data->col_shape->y;
	}
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
		else if (unit_data->col_shape->joint_type == COLLISION_JOINT_TYPE_PIN_ROUND) {
			if (unit_data->col_shape->b2body) {
				int local_face = 0; // W:0, N:1, E:2, S:3
				if (original_face == UNIT_FACE_W) local_face = 0;
				else if (original_face == UNIT_FACE_N) local_face = 1;
				else if (original_face == UNIT_FACE_E) local_face = 2;
				else if (original_face == UNIT_FACE_S) local_face = 3;

				// rotate clockwise
				float angle = unit_data->col_shape->b2body->GetAngle();
				int count = (int)(angle / (b2_pi / 2.0f));
				local_face = (local_face + count) % 4; // W->N->E->S->W->N ...

				if (local_face == 0) relative_face = UNIT_FACE_W;
				else if (local_face == 1) relative_face = UNIT_FACE_N;
				else if (local_face == 2) relative_face = UNIT_FACE_E;
				else if (local_face == 3) relative_face = UNIT_FACE_S;
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
		float tmp_x = unit_data->col_shape->old_vec_x;
		float tmp_y = unit_data->col_shape->old_vec_y;
		if (ABS(tmp_x) >= ABS(tmp_y)) {
			if (tmp_x <= 0) other_face = UNIT_FACE_E;
			else other_face = UNIT_FACE_W;
		}
		else {
			if (tmp_y <= 0) other_face = UNIT_FACE_S;
			else other_face = UNIT_FACE_N;
		}
	}
	return other_face;
}

static void get_face_velocity_single(float* vec_x, float* vec_y, int face, float abs_velocity, int bullet_track_type, int bullet_num)
{
	if (face == UNIT_FACE_N) { *vec_x = 0.0f; *vec_y = -abs_velocity; }
	else if (face == UNIT_FACE_E) { *vec_x = abs_velocity;  *vec_y = 0.0f; }
	else if (face == UNIT_FACE_W) { *vec_x = -abs_velocity; *vec_y = 0.0f; }
	else if (face == UNIT_FACE_S) { *vec_x = 0.0f;          *vec_y = abs_velocity; }
}

static void get_face_velocity_line(float* vec_x, float* vec_y, int face, float abs_velocity, int bullet_track_type, int bullet_num)
{
	if (bullet_num == UNIT_BULLET_NUM_SINGLE) {
		get_face_velocity_single(vec_x, vec_y, face, abs_velocity, bullet_track_type, bullet_num);
	}
	else if ((bullet_num == UNIT_BULLET_NUM_DOUBLE) || (bullet_num == UNIT_BULLET_NUM_TRIPLE) || (bullet_num == UNIT_BULLET_NUM_QUADRUPLE)) {
		for (int i = 0; i < bullet_num; i++) {
			if (face == UNIT_FACE_N) { *(vec_x + i) = 0.0f; *(vec_y + i) = -abs_velocity; }
			else if (face == UNIT_FACE_E) { *(vec_x + i) = abs_velocity;  *(vec_y + i) = 0.0f; }
			else if (face == UNIT_FACE_W) { *(vec_x + i) = -abs_velocity; *(vec_y + i) = 0.0f; }
			else if (face == UNIT_FACE_S) { *(vec_x + i) = 0.0f;          *(vec_y + i) = abs_velocity; }
		}
	}
}

static void get_face_velocity_radial(float* vec_x, float* vec_y, int face, float abs_velocity, int bullet_track_type, int bullet_num)
{
	if (bullet_num == UNIT_BULLET_NUM_SINGLE) {
		get_face_velocity_single(vec_x, vec_y, face, abs_velocity, bullet_track_type, bullet_num);
	}
	else if (bullet_num == UNIT_BULLET_NUM_DOUBLE) {
		float val_1_r2 = 0.7071067f;

		if (face == UNIT_FACE_N) {
			*(vec_x + 0) = -val_1_r2 * abs_velocity;
			*(vec_y + 0) = -val_1_r2 * abs_velocity;
			*(vec_x + 1) = val_1_r2 * abs_velocity;
			*(vec_y + 1) = -val_1_r2 * abs_velocity;
		}
		else if (face == UNIT_FACE_E) {
			*(vec_x + 0) = val_1_r2 * abs_velocity;
			*(vec_y + 0) = -val_1_r2 * abs_velocity;
			*(vec_x + 1) = val_1_r2 * abs_velocity;
			*(vec_y + 1) = val_1_r2 * abs_velocity;
		}
		else if (face == UNIT_FACE_W) {
			*(vec_x + 0) = -val_1_r2 * abs_velocity;
			*(vec_y + 0) = -val_1_r2 * abs_velocity;
			*(vec_x + 1) = -val_1_r2 * abs_velocity;
			*(vec_y + 1) = val_1_r2 * abs_velocity;
		}
		else if (face == UNIT_FACE_S) {
			*(vec_x + 0) = -val_1_r2 * abs_velocity;
			*(vec_y + 0) = val_1_r2 * abs_velocity;
			*(vec_x + 1) = val_1_r2 * abs_velocity;
			*(vec_y + 1) = val_1_r2 * abs_velocity;
		}
	}
	else if (bullet_num == UNIT_BULLET_NUM_TRIPLE) {
		float val_1_r2 = 0.7071067f;

		if (face == UNIT_FACE_N) {
			*vec_x = 0.0f;
			*vec_y = -abs_velocity;
			*(vec_x + 1) = -val_1_r2 * abs_velocity;
			*(vec_y + 1) = -val_1_r2 * abs_velocity;
			*(vec_x + 2) = val_1_r2 * abs_velocity;
			*(vec_y + 2) = -val_1_r2 * abs_velocity;
		}
		else if (face == UNIT_FACE_E) {
			*vec_x = abs_velocity;
			*vec_y = 0.0f;
			*(vec_x + 1) = val_1_r2 * abs_velocity;
			*(vec_y + 1) = -val_1_r2 * abs_velocity;
			*(vec_x + 2) = val_1_r2 * abs_velocity;
			*(vec_y + 2) = val_1_r2 * abs_velocity;
		}
		else if (face == UNIT_FACE_W) {
			*vec_x = -abs_velocity;
			*vec_y = 0.0f;
			*(vec_x + 1) = -val_1_r2 * abs_velocity;
			*(vec_y + 1) = -val_1_r2 * abs_velocity;
			*(vec_x + 2) = -val_1_r2 * abs_velocity;
			*(vec_y + 2) = val_1_r2 * abs_velocity;
		}
		else if (face == UNIT_FACE_S) {
			*vec_x = 0.0f;
			*vec_y = abs_velocity;
			*(vec_x + 1) = -val_1_r2 * abs_velocity;
			*(vec_y + 1) = val_1_r2 * abs_velocity;
			*(vec_x + 2) = val_1_r2 * abs_velocity;
			*(vec_y + 2) = val_1_r2 * abs_velocity;
		}
	}
}

static void get_face_velocity_wave(float* vec_x, float* vec_y, int face, float abs_velocity, int bullet_track_type, int bullet_num)
{
	if (bullet_num == UNIT_BULLET_NUM_SINGLE) {
		get_face_velocity_single(vec_x, vec_y, face, abs_velocity, bullet_track_type, bullet_num);
	}
}

#define UNIT_BULLET_VELOCITY_RANDOM_ANGLE_MAX  75
#define UNIT_BULLET_VELOCITY_RANDOM_ANGLE_MIN  45
static void get_face_velocity_random(float* vec_x, float* vec_y, int face, float abs_velocity, int bullet_track_type, int bullet_num)
{
	int sign_val = 1;
	for (int i = 0; i < bullet_num; i++) {
		int angle = game_utils_random_gen(UNIT_BULLET_VELOCITY_RANDOM_ANGLE_MAX, UNIT_BULLET_VELOCITY_RANDOM_ANGLE_MIN);
		float sin_val = sinf(b2_pi * angle / 180);
		float cos_val = cosf(b2_pi * angle / 180);

		if ((face == UNIT_FACE_N) || (face == UNIT_FACE_S)) {
			*(vec_x + i) =  abs_velocity * cos_val * sign_val;
			*(vec_y + i) = -abs_velocity * sin_val;
			sign_val = -sign_val;
		}
		else if (face == UNIT_FACE_E) {
			*(vec_x + i) = abs_velocity * cos_val;
			*(vec_y + i) = -abs_velocity * sin_val;
		}
		else if (face == UNIT_FACE_W) {
			*(vec_x + i) = -abs_velocity * cos_val;
			*(vec_y + i) = -abs_velocity * sin_val;
		}
	}
}

static void get_face_velocity_cross(float* vec_x, float* vec_y, int face, float abs_velocity, int bullet_track_type, int bullet_num)
{
	for (int i = 0; i < bullet_num; i++) {
		if (i + 1 == UNIT_FACE_N) { *(vec_x + i) = 0.0f; *(vec_y + i) = -abs_velocity; }
		else if (i + 1 == UNIT_FACE_E) { *(vec_x + i) = abs_velocity;  *(vec_y + i) = 0.0f; }
		else if (i + 1 == UNIT_FACE_W) { *(vec_x + i) = -abs_velocity; *(vec_y + i) = 0.0f; }
		else if (i + 1 == UNIT_FACE_S) { *(vec_x + i) = 0.0f;          *(vec_y + i) = abs_velocity; }
	}
}

static void get_face_velocity_xcross(float* vec_x, float* vec_y, int face, float abs_velocity, int bullet_track_type, int bullet_num)
{
	float abs_vec = abs_velocity * sinf(b2_pi * 45.0f / 180.0f);
	for (int i = 0; i < bullet_num; i++) {
		if (i + 1 == UNIT_FACE_N) { *(vec_x + i) = -abs_vec; *(vec_y + i) = -abs_vec; }
		else if (i + 1 == UNIT_FACE_E) { *(vec_x + i) = abs_vec;  *(vec_y + i) = -abs_vec; }
		else if (i + 1 == UNIT_FACE_W) { *(vec_x + i) = -abs_vec; *(vec_y + i) = abs_vec; }
		else if (i + 1 == UNIT_FACE_S) { *(vec_x + i) = abs_vec;  *(vec_y + i) = abs_vec; }
	}
}

void unit_manager_get_face_velocity(float* vec_x, float* vec_y, int face, float abs_velocity, int bullet_track_type, int bullet_num)
{
	if (bullet_num == UNIT_BULLET_NUM_NONE) {
		// for invisible bullet

	}
	else {
		if (bullet_track_type == UNIT_BULLET_TRACK_LINE) {
			get_face_velocity_line(vec_x, vec_y, face, abs_velocity, bullet_track_type, bullet_num);
		}
		else if (bullet_track_type == UNIT_BULLET_TRACK_RADIAL) {
			get_face_velocity_radial(vec_x, vec_y, face, abs_velocity, bullet_track_type, bullet_num);
		}
		else if (bullet_track_type == UNIT_BULLET_TRACK_WAVE) {
			get_face_velocity_wave(vec_x, vec_y, face, abs_velocity, bullet_track_type, bullet_num);
		}
		else if (bullet_track_type == UNIT_BULLET_TRACK_CROSS) {
			get_face_velocity_cross(vec_x, vec_y, face, abs_velocity, bullet_track_type, bullet_num);
		}
		else if (bullet_track_type == UNIT_BULLET_TRACK_XCROSS) {
			get_face_velocity_xcross(vec_x, vec_y, face, abs_velocity, bullet_track_type, bullet_num);
		}
		else if (bullet_track_type == UNIT_BULLET_TRACK_RANDOM) {
			get_face_velocity_random(vec_x, vec_y, face, abs_velocity, bullet_track_type, bullet_num);
		}
	}
}

static void get_target_velocity_line(float* vec_x, float* vec_y, unit_data_t* unit_data, int target_x, int target_y, float abs_velocity, int bullet_track_type, int bullet_num)
{
	int unit_x;
	int unit_y;
	unit_manager_get_center_position(unit_data, &unit_x, &unit_y);

	int dist_x = target_x - unit_x;
	int dist_y = target_y - unit_y;
	float length = sqrtf(float(dist_x * dist_x + dist_y * dist_y));
	if (length <= FLOAT_NEAR_ZERO) length = abs_velocity;

	if (bullet_num == UNIT_BULLET_NUM_SINGLE) {
		*vec_x = dist_x * abs_velocity / length;
		*vec_y = dist_y * abs_velocity / length;
	}
}

void unit_manager_get_target_velocity(float* vec_x, float* vec_y, unit_data_t* unit_data, int target_x, int target_y, float abs_velocity, int bullet_track_type, int bullet_num)
{
	if (bullet_num == UNIT_BULLET_NUM_NONE) {
		// for invisible bullet

	}
	else {
		if (bullet_track_type == UNIT_BULLET_TRACK_LINE) {
			get_target_velocity_line(vec_x, vec_y, unit_data, target_x, target_y, abs_velocity, bullet_track_type, bullet_num);
		}
	}
}

static void get_bullet_start_pos_single(unit_data_t* unit_data, unit_data_t* unit_bullet_data, int bullet_num, int face, int* x, int* y)
{
	int bullet_offset_x = 0, bullet_offset_y = 0;
	int bullet_w = 0, bullet_h = 0;
	if (unit_bullet_data->col_shape->type == COLLISION_TYPE_BOX_D) {
		bullet_offset_x = unit_bullet_data->col_shape->offset_x;
		bullet_offset_y = unit_bullet_data->col_shape->offset_y;
		bullet_w = ((shape_box_data*)unit_bullet_data->col_shape)->w;
		bullet_h = ((shape_box_data*)unit_bullet_data->col_shape)->h;
	}

	int offset_vec_x = 0;
	int offset_vec_y = 0;
	if (unit_data->col_shape->type == COLLISION_TYPE_BOX_D) {
		int w, h;
		w = ((shape_box_data*)unit_data->col_shape)->w;
		h = ((shape_box_data*)unit_data->col_shape)->h;

		if (face == UNIT_FACE_N) {
			offset_vec_x = unit_data->col_shape->offset_x + w / 2 - bullet_h / 2;
			offset_vec_y = unit_data->col_shape->offset_y - 1 - bullet_h;
		}
		else if (face == UNIT_FACE_S) {
			offset_vec_x = unit_data->col_shape->offset_x + w / 2 - bullet_h / 2;
			offset_vec_y = unit_data->col_shape->offset_y + h + 1;
		}
		else if (face == UNIT_FACE_W) {
			offset_vec_x = unit_data->col_shape->offset_x - 1 - bullet_w;
			offset_vec_y = unit_data->col_shape->offset_y + h / 2 - bullet_h / 2;
		}
		else if (face == UNIT_FACE_E) {
			offset_vec_x = unit_data->col_shape->offset_x + w;
			offset_vec_y = unit_data->col_shape->offset_y + h / 2 - bullet_h / 2;
		}
		else {
			offset_vec_x = unit_data->col_shape->offset_x + w / 2 - bullet_h / 2;
			offset_vec_y = unit_data->col_shape->offset_y + h / 2 - bullet_h / 2;
		}
	}
	else if (unit_data->col_shape->type == COLLISION_TYPE_ROUND_D) {
		int r = ((shape_round_data*)unit_data->col_shape)->r;

		if (face == UNIT_FACE_N) {
			offset_vec_x = unit_data->col_shape->offset_x;
			offset_vec_y = unit_data->col_shape->offset_y - 1;
		}
		else if (face == UNIT_FACE_S) {
			offset_vec_x = unit_data->col_shape->offset_x;
			offset_vec_y = unit_data->col_shape->offset_y + r + 1;
		}
		else if (face == UNIT_FACE_W) {
			offset_vec_x = unit_data->col_shape->offset_x - 1;
			offset_vec_y = unit_data->col_shape->offset_y;
		}
		else if (face == UNIT_FACE_E) {
			offset_vec_x = unit_data->col_shape->offset_x + r + 1;
			offset_vec_y = unit_data->col_shape->offset_y;
		}
		else {
			offset_vec_x = unit_data->col_shape->offset_x;
			offset_vec_y = unit_data->col_shape->offset_y;
		}
	}

	int pos_x, pos_y;
	unit_manager_get_position(unit_data, &pos_x, &pos_y);

	*x = pos_x + offset_vec_x;
	*y = pos_y + offset_vec_y;
}

static void get_bullet_start_pos_line(unit_data_t* unit_data, unit_data_t* unit_bullet_data, int bullet_num, int face, int* x, int* y)
{
	if (bullet_num == UNIT_BULLET_NUM_SINGLE) {
		get_bullet_start_pos_single(unit_data, unit_bullet_data, bullet_num, face, x, y);
	}
	else {
		int pos_x, pos_y;
		unit_manager_get_position(unit_data, &pos_x, &pos_y);

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

			if (bullet_num == UNIT_BULLET_NUM_DOUBLE) {
				if (face == UNIT_FACE_N) {
					*x = pos_x + unit_data->col_shape->offset_x + w * 1 / 4 - bullet_h / 2;
					*y = pos_y + unit_data->col_shape->offset_y - 1 - bullet_h;
					*(x + 1) = pos_x + unit_data->col_shape->offset_x + w * 3 / 4 - bullet_h / 2;
					*(y + 1) = *y;
				}
				else if (face == UNIT_FACE_S) {
					*x = pos_x + unit_data->col_shape->offset_x + w * 1 / 4 - bullet_h / 2;
					*y = pos_y + unit_data->col_shape->offset_y + h + 1;
					*(x + 1) = pos_x + unit_data->col_shape->offset_x + w * 3 / 4 - bullet_h / 2;
					*(y + 1) = *y;
				}
				else if (face == UNIT_FACE_W) {
					*x = pos_x + unit_data->col_shape->offset_x - 1 - bullet_w;
					*y = pos_y + unit_data->col_shape->offset_y + h * 1 / 4 - bullet_h / 2;
					*(x + 1) = *x;
					*(y + 1) = pos_y + unit_data->col_shape->offset_y + h * 3 / 4 - bullet_h / 2;
				}
				else if (face == UNIT_FACE_E) {
					*x = pos_x + unit_data->col_shape->offset_x + w;
					*y = pos_y + unit_data->col_shape->offset_y + h * 1 / 4 - bullet_h / 2;
					*(x + 1) = *x;
					*(y + 1) = pos_y + unit_data->col_shape->offset_y + h * 3 / 4 - bullet_h / 2;
				}
			}
			else if (bullet_num == UNIT_BULLET_NUM_TRIPLE) {
				if (face == UNIT_FACE_N) {
					*x = pos_x + unit_data->col_shape->offset_x + w * 1 / 4 - bullet_h / 2;
					*y = pos_y + unit_data->col_shape->offset_y - 1 - bullet_h;
					*(x + 1) = pos_x + unit_data->col_shape->offset_x + w * 2 / 4 - bullet_h / 2;
					*(y + 1) = *y;
					*(x + 2) = pos_x + unit_data->col_shape->offset_x + w * 3 / 4 - bullet_h / 2;
					*(y + 2) = *y;
				}
				else if (face == UNIT_FACE_S) {
					*x = pos_x + unit_data->col_shape->offset_x + w * 1 / 4 - bullet_h / 2;
					*y = pos_y + unit_data->col_shape->offset_y + h + 1;
					*(x + 1) = pos_x + unit_data->col_shape->offset_x + w * 2 / 4 - bullet_h / 2;
					*(y + 1) = *y;
					*(x + 2) = pos_x + unit_data->col_shape->offset_x + w * 3 / 4 - bullet_h / 2;
					*(y + 2) = *y;
				}
				else if (face == UNIT_FACE_W) {
					*x = pos_x + unit_data->col_shape->offset_x - 1 - bullet_w;
					*y = pos_y + unit_data->col_shape->offset_y + h * 1 / 4 - bullet_h / 2;
					*(x + 1) = *x;
					*(y + 1) = pos_y + unit_data->col_shape->offset_y + h * 2 / 4 - bullet_h / 2;
					*(x + 2) = *x;
					*(y + 2) = pos_y + unit_data->col_shape->offset_y + h * 3 / 4 - bullet_h / 2;
				}
				else if (face == UNIT_FACE_E) {
					*x = pos_x + unit_data->col_shape->offset_x + w;
					*y = pos_y + unit_data->col_shape->offset_y + h * 1 / 4 - bullet_h / 2;
					*(x + 1) = *x;
					*(y + 1) = pos_y + unit_data->col_shape->offset_y + h * 2 / 4 - bullet_h / 2;
					*(x + 2) = *x;
					*(y + 2) = pos_y + unit_data->col_shape->offset_y + h * 3 / 4 - bullet_h / 2;
				}
			}
			else if (bullet_num == UNIT_BULLET_NUM_QUADRUPLE) {
				if (face == UNIT_FACE_N) {
					*x = pos_x + unit_data->col_shape->offset_x + w * 1 / 5 - bullet_h / 2;
					*y = pos_y + unit_data->col_shape->offset_y - 1 - bullet_h;
					*(x + 1) = pos_x + unit_data->col_shape->offset_x + w * 2 / 5 - bullet_h / 2;
					*(y + 1) = *y;
					*(x + 2) = pos_x + unit_data->col_shape->offset_x + w * 3 / 5 - bullet_h / 2;
					*(y + 2) = *y;
					*(x + 3) = pos_x + unit_data->col_shape->offset_x + w * 4 / 5 - bullet_h / 2;
					*(y + 3) = *y;
				}
				else if (face == UNIT_FACE_S) {
					*x = pos_x + unit_data->col_shape->offset_x + w * 1 / 5 - bullet_h / 2;
					*y = pos_y + unit_data->col_shape->offset_y + h + 1;
					*(x + 1) = pos_x + unit_data->col_shape->offset_x + w * 2 / 5 - bullet_h / 2;
					*(y + 1) = *y;
					*(x + 2) = pos_x + unit_data->col_shape->offset_x + w * 3 / 5 - bullet_h / 2;
					*(y + 2) = *y;
					*(x + 3) = pos_x + unit_data->col_shape->offset_x + w * 4 / 5 - bullet_h / 2;
					*(y + 3) = *y;
				}
				else if (face == UNIT_FACE_W) {
					*x = pos_x + unit_data->col_shape->offset_x - 1 - bullet_w;
					*y = pos_y + unit_data->col_shape->offset_y + h * 1 / 5 - bullet_h / 2;
					*(x + 1) = *x;
					*(y + 1) = pos_y + unit_data->col_shape->offset_y + h * 2 / 5 - bullet_h / 2;
					*(x + 2) = *x;
					*(y + 2) = pos_y + unit_data->col_shape->offset_y + h * 3 / 5 - bullet_h / 2;
					*(x + 3) = *x;
					*(y + 3) = pos_y + unit_data->col_shape->offset_y + h * 4 / 5 - bullet_h / 2;
				}
				else if (face == UNIT_FACE_E) {
					*x = pos_x + unit_data->col_shape->offset_x + w;
					*y = pos_y + unit_data->col_shape->offset_y + h * 1 / 5 - bullet_h / 2;
					*(x + 1) = *x;
					*(y + 1) = pos_y + unit_data->col_shape->offset_y + h * 2 / 5 - bullet_h / 2;
					*(x + 2) = *x;
					*(y + 2) = pos_y + unit_data->col_shape->offset_y + h * 3 / 5 - bullet_h / 2;
					*(x + 3) = *x;
					*(y + 3) = pos_y + unit_data->col_shape->offset_y + h * 4 / 5 - bullet_h / 2;
				}
			}
		}
		else if (unit_data->col_shape->type == COLLISION_TYPE_ROUND_D) {
			int r = ((shape_round_data*)unit_data->col_shape)->r;

			if (bullet_num == UNIT_BULLET_NUM_DOUBLE) {
				if (face == UNIT_FACE_N) {
					*x = pos_x + unit_data->col_shape->offset_x - r;
					*y = pos_y + unit_data->col_shape->offset_y - 1;
					*(x + 1) = pos_x + unit_data->col_shape->offset_x + r;
					*(y + 1) = *y;
				}
				else if (face == UNIT_FACE_S) {
					*x = pos_x + unit_data->col_shape->offset_x - r;
					*y = pos_y + unit_data->col_shape->offset_y + r + 1;
					*(x + 1) = pos_x + unit_data->col_shape->offset_x + r;
					*(y + 1) = *y;
				}
				else if (face == UNIT_FACE_W) {
					*x = pos_x + unit_data->col_shape->offset_x - 1;
					*y = pos_y + unit_data->col_shape->offset_y - r;
					*(x + 1) = *x;
					*(y + 1) = pos_y + unit_data->col_shape->offset_y + r;
				}
				else if (face == UNIT_FACE_E) {
					*x = pos_x + unit_data->col_shape->offset_x + r + 1;
					*y = pos_y + unit_data->col_shape->offset_y - r;
					*(x + 1) = *x;
					*(y + 1) = pos_y + unit_data->col_shape->offset_y + r;
				}
			}
			else if (bullet_num == UNIT_BULLET_NUM_TRIPLE) {
				if (face == UNIT_FACE_N) {
					*x = pos_x + unit_data->col_shape->offset_x - r;
					*y = pos_y + unit_data->col_shape->offset_y - 1;
					*(x + 1) = pos_x + unit_data->col_shape->offset_x;
					*(y + 1) = *y;
					*(x + 2) = pos_x + unit_data->col_shape->offset_x + r;
					*(y + 2) = *y;
				}
				else if (face == UNIT_FACE_S) {
					*x = pos_x + unit_data->col_shape->offset_x - r;
					*y = pos_y + unit_data->col_shape->offset_y + r + 1;
					*(x + 1) = pos_x + unit_data->col_shape->offset_x;
					*(y + 1) = *y;
					*(x + 2) = pos_x + unit_data->col_shape->offset_x + r;
					*(y + 2) = *y;
				}
				else if (face == UNIT_FACE_W) {
					*x = pos_x + unit_data->col_shape->offset_x - 1;
					*y = pos_y + unit_data->col_shape->offset_y - r;
					*(x + 1) = *x;
					*(y + 1) = pos_y + unit_data->col_shape->offset_y;
					*(x + 2) = *x;
					*(y + 2) = pos_y + unit_data->col_shape->offset_y + r;
				}
				else if (face == UNIT_FACE_E) {
					*x = pos_x + unit_data->col_shape->offset_x + r + 1;
					*y = pos_y + unit_data->col_shape->offset_y - r;
					*(x + 1) = *x;
					*(y + 1) = pos_y + unit_data->col_shape->offset_y;
					*(x + 2) = *x;
					*(y + 2) = pos_y + unit_data->col_shape->offset_y + r;
				}
			}
			else if (bullet_num == UNIT_BULLET_NUM_QUADRUPLE) {
				if (face == UNIT_FACE_N) {
					*x = pos_x + unit_data->col_shape->offset_x - r + (2 * r * 1 / 5);
					*y = pos_y + unit_data->col_shape->offset_y - 1;
					*(x + 1) = pos_x + unit_data->col_shape->offset_x - r + (2 * r * 2 / 5);
					*(y + 1) = *y;
					*(x + 2) = pos_x + unit_data->col_shape->offset_x - r + (2 * r * 3 / 5);
					*(y + 2) = *y;
					*(x + 3) = pos_x + unit_data->col_shape->offset_x - r + (2 * r * 4 / 5);
					*(y + 3) = *y;
				}
				else if (face == UNIT_FACE_S) {
					*x = pos_x + unit_data->col_shape->offset_x - r + (2 * r * 1 / 5);
					*y = pos_y + unit_data->col_shape->offset_y + r + 1;
					*(x + 1) = pos_x + unit_data->col_shape->offset_x - r + (2 * r * 2 / 5);
					*(y + 1) = *y;
					*(x + 2) = pos_x + unit_data->col_shape->offset_x - r + (2 * r * 3 / 5);
					*(y + 2) = *y;
					*(x + 3) = pos_x + unit_data->col_shape->offset_x - r + (2 * r * 4 / 5);
					*(y + 3) = *y;
				}
				else if (face == UNIT_FACE_W) {
					*x = pos_x + unit_data->col_shape->offset_x - 1;
					*y = pos_y + unit_data->col_shape->offset_y - r + (2 * r * 1 / 5);
					*(x + 1) = *x;
					*(y + 1) = pos_y + unit_data->col_shape->offset_y - r + (2 * r * 2 / 5);
					*(x + 2) = *x;
					*(y + 2) = pos_y + unit_data->col_shape->offset_y - r + (2 * r * 3 / 5);
					*(x + 3) = *x;
					*(y + 3) = pos_y + unit_data->col_shape->offset_y - r + (2 * r * 4 / 5);
				}
				else if (face == UNIT_FACE_E) {
					*x = pos_x + unit_data->col_shape->offset_x + r + 1;
					*y = pos_y + unit_data->col_shape->offset_y - r + (2 * r * 1 / 5);
					*(x + 1) = *x;
					*(y + 1) = pos_y + unit_data->col_shape->offset_y - r + (2 * r * 2 / 5);
					*(x + 2) = *x;
					*(y + 2) = pos_y + unit_data->col_shape->offset_y - r + (2 * r * 3 / 5);
					*(x + 3) = *x;
					*(y + 3) = pos_y + unit_data->col_shape->offset_y - r + (2 * r * 4 / 5);
				}
			}
		}
	}
}

static void get_bullet_start_pos_radial(unit_data_t* unit_data, unit_data_t* unit_bullet_data, int bullet_num, int face, int* x, int* y)
{
	if (bullet_num == UNIT_BULLET_NUM_SINGLE) {
		get_bullet_start_pos_single(unit_data, unit_bullet_data, bullet_num, face, x, y);
	}
	else {
		int pos_x, pos_y;
		unit_manager_get_position(unit_data, &pos_x, &pos_y);

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

			if ((bullet_num == UNIT_BULLET_NUM_DOUBLE) || (bullet_num == UNIT_BULLET_NUM_TRIPLE)) {
				for (int i = 0; i < bullet_num; i++) {
					if (face == UNIT_FACE_N) {
						*x = pos_x + unit_data->col_shape->offset_x + w / 2 - bullet_h / 2;
						*y = pos_y + unit_data->col_shape->offset_y - 1 - bullet_h;
					}
					else if (face == UNIT_FACE_S) {
						*x = pos_x + unit_data->col_shape->offset_x + w / 2 - bullet_h / 2;
						*y = pos_y + unit_data->col_shape->offset_y + h + 1;
					}
					else if (face == UNIT_FACE_W) {
						*x = pos_x + unit_data->col_shape->offset_x - 1 - bullet_w;
						*y = pos_y + unit_data->col_shape->offset_y + h / 2 - bullet_h / 2;
					}
					else if (face == UNIT_FACE_E) {
						*x = pos_x + unit_data->col_shape->offset_x + w;
						*y = pos_y + unit_data->col_shape->offset_y + h / 2 - bullet_h / 2;
					}
					x++;
					y++;
				}
			}
		}
		else if (unit_data->col_shape->type == COLLISION_TYPE_ROUND_D) {
			int r = ((shape_round_data*)unit_data->col_shape)->r;

			if ((bullet_num == UNIT_BULLET_NUM_DOUBLE) || (bullet_num == UNIT_BULLET_NUM_TRIPLE)) {
				for (int i = 0; i < bullet_num; i++) {
					if (face == UNIT_FACE_N) {
						*x = pos_x + unit_data->col_shape->offset_x;
						*y = pos_y + unit_data->col_shape->offset_y - 1;
					}
					else if (face == UNIT_FACE_S) {
						*x = pos_x + unit_data->col_shape->offset_x;
						*y = pos_y + unit_data->col_shape->offset_y + r + 1;
					}
					else if (face == UNIT_FACE_W) {
						*x = pos_x + unit_data->col_shape->offset_x - 1;
						*y = pos_y + unit_data->col_shape->offset_y;
					}
					else if (face == UNIT_FACE_E) {
						*x = pos_x + unit_data->col_shape->offset_x + r + 1;
						*y = pos_y + unit_data->col_shape->offset_y;
					}
					x++;
					y++;
				}
			}
		}
	}
}

static void get_bullet_start_pos_wave(unit_data_t* unit_data, unit_data_t* unit_bullet_data, int bullet_num, int face, int* x, int* y)
{
	if (bullet_num == UNIT_BULLET_NUM_SINGLE) {
		get_bullet_start_pos_single(unit_data, unit_bullet_data, bullet_num, face, x, y);
	}
}

static void get_bullet_start_pos_random(unit_data_t* unit_data, unit_data_t* unit_bullet_data, int bullet_num, int face, int* x, int* y)
{
	get_bullet_start_pos_single(unit_data, unit_bullet_data, bullet_num, face, x, y);

	int src_x = *x;
	int src_y = *y;
	for (int i = 1; i < bullet_num; i++) {
		*(x + i) = src_x;
		*(y + i) = src_y;
	}
}

static void get_bullet_start_pos_cross(unit_data_t* unit_data, unit_data_t* unit_bullet_data, int bullet_num, int face, int* x, int* y)
{
	get_bullet_start_pos_single(unit_data, unit_bullet_data, bullet_num, face, x, y);

	int src_x = *x;
	int src_y = *y;
	for (int i = 1; i < bullet_num; i++) {
		*(x + i) = src_x;
		*(y + i) = src_y;
	}
}

static void get_bullet_start_pos_xcross(unit_data_t* unit_data, unit_data_t* unit_bullet_data, int bullet_num, int face, int* x, int* y)
{
	get_bullet_start_pos_single(unit_data, unit_bullet_data, bullet_num, face, x, y);

	int src_x = *x;
	int src_y = *y;
	for (int i = 1; i < bullet_num; i++) {
		*(x + i) = src_x;
		*(y + i) = src_y;
	}
}

void unit_manager_get_bullet_start_pos(unit_data_t* unit_data, unit_data_t* unit_bullet_data, int bullet_track_type, int bullet_num, int face, int* x, int* y)
{
	if (bullet_num == UNIT_BULLET_NUM_NONE) {
		// for invisible bullet

	}
	else if ((bullet_num == UNIT_BULLET_NUM_SINGLE)
		|| (bullet_num == UNIT_BULLET_NUM_DOUBLE) || (bullet_num == UNIT_BULLET_NUM_TRIPLE) || (bullet_num == UNIT_BULLET_NUM_QUADRUPLE))
	{
		if (bullet_track_type == UNIT_BULLET_TRACK_LINE) {
			get_bullet_start_pos_line(unit_data, unit_bullet_data, bullet_num, face, x, y);
		}
		else if (bullet_track_type == UNIT_BULLET_TRACK_RADIAL) {
			get_bullet_start_pos_radial(unit_data, unit_bullet_data, bullet_num, face, x, y);
		}
		else if (bullet_track_type == UNIT_BULLET_TRACK_WAVE) {
			get_bullet_start_pos_wave(unit_data, unit_bullet_data, bullet_num, face, x, y);
		}
		else if (bullet_track_type == UNIT_BULLET_TRACK_CROSS) {
			get_bullet_start_pos_cross(unit_data, unit_bullet_data, bullet_num, face, x, y);
		}
		else if (bullet_track_type == UNIT_BULLET_TRACK_XCROSS) {
			get_bullet_start_pos_xcross(unit_data, unit_bullet_data, bullet_num, face, x, y);
		}
	}

	// random any bullets
	if (bullet_track_type == UNIT_BULLET_TRACK_RANDOM) {
		get_bullet_start_pos_random(unit_data, unit_bullet_data, bullet_num, face, x, y);
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
