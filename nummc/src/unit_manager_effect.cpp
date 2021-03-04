#include "game_common.h"
#include "unit_manager.h"

#include "game_timer.h"
#include "game_log.h"
#include "game_utils.h"
#include "game_window.h"
#include "game_event.h"

#include "resource_manager.h"
#include "memory_manager.h"

static unit_effect_data_t effect_base[UNIT_EFFECT_BASE_LIST_SIZE];
static unit_effect_data_t effect[UNIT_EFFECT_LIST_SIZE];
static int effect_base_index_end;
static int effect_index_end;

static void load_effect_unit(char* line);

//
// effect
//
int unit_manager_init_effect()
{
	memset(&effect_base, 0, sizeof(effect_base));
	memset(&effect, 0, sizeof(effect));
	effect_base_index_end = 0;
	effect_index_end = 0;

	return 0;
}

void unit_manager_unload_effect()
{
	for (int i = 0; i < UNIT_EFFECT_BASE_LIST_SIZE; i++) {
		if (effect_base[i].obj) {
			memory_manager_delete_char_buff((char*)effect_base[i].obj);
			effect_base[i].obj = NULL;
		}
	}
}

int unit_manager_search_effect(char* path)
{
	int ii = 0;
	bool effect_found = false;
	while (effect_base[ii].type == UNIT_TYPE_EFFECT) {
		char* regist_path = (char*)effect_base[ii].obj;
		if ((regist_path != NULL) && STRCMP_EQ(regist_path,path)) {
			effect_found = true;
			break;
		}
		ii++;
	}

	if (effect_found) return ii;  // regist already
	else return (-1);             // not found
}

void unit_manager_effect_set_trace_unit(int unit_id, unit_data_t* unit_data)
{
	effect[unit_id].trace_unit = unit_data;
}

void unit_manager_effect_set_anim_stat(int unit_id, int stat)
{
	unit_manager_unit_set_anim_stat((unit_data_t*)&effect[unit_id], stat);

	// DIE
	if ((effect[unit_id].anim->stat == ANIM_STAT_FLAG_DIE) && effect[unit_id].col_shape->b2body) {
		// disable collision body
		g_stage_world->DestroyBody(effect[unit_id].col_shape->b2body);
		effect[unit_id].col_shape->b2body = NULL;
	}
}

void unit_manager_effect_set_b2position(int unit_id, float x, float y)
{
	b2Vec2 b2_effect_pos = { x, y };
	effect[unit_id].col_shape->b2body->SetTransform(b2_effect_pos, 0.0f);
	effect[unit_id].col_shape->x = (int)MET2PIX(effect[unit_id].col_shape->b2body->GetPosition().x);
	effect[unit_id].col_shape->y = (int)MET2PIX(effect[unit_id].col_shape->b2body->GetPosition().y);
}

unit_effect_data_t* unit_manager_get_effect(int index)
{
	return &effect[index];
}

typedef struct _load_effect_callback_data_t load_effect_callback_data_t;
struct _load_effect_callback_data_t {
	bool read_flg[UNIT_TAG_END];
	char* path;
};
static load_effect_callback_data_t load_effect_callback_data;
static void load_effect_callback(char* line, int line_size, int line_num, void* argv)
{
	load_effect_callback_data_t* data = (load_effect_callback_data_t*)argv;

	if (line[0] == '\0') return;
	if (line[0] == '#') return;

	if (line[0] == '[') {
		if (STRCMP_EQ(line, "[unit]")) {
			data->read_flg[UNIT_TAG_UNIT] = true;

			// set base unit data
			char* path_c_str = memory_manager_new_char_buff((int)strlen(data->path));
			game_utils_string_copy(path_c_str, data->path);
			effect_base[effect_base_index_end].obj = (void*)path_c_str;
			effect_base[effect_base_index_end].type = UNIT_TYPE_EFFECT;
			effect_base[effect_base_index_end].id = effect_base_index_end;
			effect_base[effect_base_index_end].clear_type = UNIT_EFFECT_CLEAR_TYPE_NONE;
			return;
		}
		if (STRCMP_EQ(line, "[/unit]"))      { data->read_flg[UNIT_TAG_UNIT]      = false; return; }
		if (STRCMP_EQ(line, "[collision]"))  { data->read_flg[UNIT_TAG_COLLISION] = true;  return; }
		if (STRCMP_EQ(line, "[/collision]")) { data->read_flg[UNIT_TAG_COLLISION] = false; return; }
		if (STRCMP_EQ(line, "[anim]")) {
			data->read_flg[UNIT_TAG_ANIM] = true;
			effect_base[effect_base_index_end].anim = animation_manager_new_anim_data();
			animation_manager_new_anim_stat_base_data(effect_base[effect_base_index_end].anim);
			return;
		}
		if (STRCMP_EQ(line, "[/anim]")) { data->read_flg[UNIT_TAG_ANIM] = false; return; }
	}

	if (data->read_flg[UNIT_TAG_UNIT]) {
		load_effect_unit(line);
	}
	if (data->read_flg[UNIT_TAG_COLLISION]) {
		load_collision(line, &effect_base[effect_base_index_end].col_shape);
	}
	if (data->read_flg[UNIT_TAG_ANIM]) {
		load_anim(line, effect_base[effect_base_index_end].anim);
	}
}

int unit_manager_load_effect(char* path)
{
	if (unit_manager_search_effect(path) > 0) return 0;

	// full_path = g_base_path + "data/" + path;
	char full_path[GAME_FULL_PATH_MAX];
	int tmp_path_size = game_utils_string_cat(full_path, g_base_path, (char*)"data/", path);
	if (tmp_path_size == 0) {
		LOG_ERROR("unit_manager_load_effect failed get %s\n", path);
		return 1;
	}

	// read file
	memset(load_effect_callback_data.read_flg, 0, sizeof(bool) * UNIT_TAG_END);
	load_effect_callback_data.path = path;
	int ret = game_utils_files_read_line(full_path, load_effect_callback, (void*)&load_effect_callback_data);
	if (ret != 0) {
		LOG_ERROR("unit_manager_load_effect %s error\n", path);
		return 1;
	}

	// load anim files
	if (effect_base[effect_base_index_end].anim) {
		for (int i = 0; i < ANIM_STAT_END; i++) {
			char* anim_path = (char*)effect_base[effect_base_index_end].anim->anim_stat_base_list[i]->obj;
			if (anim_path) {
				animation_manager_load_file(anim_path, effect_base[effect_base_index_end].anim, i);
			}
		}
	}

	effect_base_index_end++;

	if (effect_base_index_end >= UNIT_EFFECT_BASE_LIST_SIZE) {
		LOG_ERROR("ERROR: unit_manager_load_effect() effect_base overflow\n");
		return 1;
	}

	return 0;
}

static void load_effect_unit(char* line)
{
	char key[GAME_UTILS_STRING_NAME_BUF_SIZE];
	char value[GAME_UTILS_STRING_NAME_BUF_SIZE];
	game_utils_split_key_value(line, key, value);

	if (STRCMP_EQ(key,"group")) {
		if (STRCMP_EQ(value,"NONE")) {
			effect_base[effect_base_index_end].group = UNIT_EFFECT_GROUP_NONE;
		}
		else if (STRCMP_EQ(value,"STATIC")) {
			effect_base[effect_base_index_end].group = UNIT_EFFECT_GROUP_STATIC;
		}
		else if (STRCMP_EQ(value,"DYNAMIC")) {
			effect_base[effect_base_index_end].group = UNIT_EFFECT_GROUP_DYNAMIC;
		}
		else if (STRCMP_EQ(value,"DIE_AUTO")) {
			effect_base[effect_base_index_end].group = UNIT_EFFECT_GROUP_DIE_AUTO;
		}
		return;
	}
	if (STRCMP_EQ(key, "life_timer")) {
		effect_base[effect_base_index_end].life_timer = atoi(value);
		return;
	}
}

int unit_manager_create_effect(int x, int y, int base_index)
{
	int ret = -1;

	for (int i = effect_index_end; i < UNIT_EFFECT_LIST_SIZE; i++) {
		if (effect[i].type != UNIT_TYPE_EFFECT) {
			ret = effect_index_end = i;
			break;
		}
	}
	if ((ret == -1) && (effect_index_end > 0)) {
		for (int i = 0; i < effect_index_end; i++) {
			if (effect[i].type != UNIT_TYPE_EFFECT) {
				ret = effect_index_end = i;
				break;
			}
		}
	}
	if (ret == -1) {
		LOG_ERROR("Error: unit_manager_create_effect() overflow.");
		return ret;
	}

	if (base_index == -1) base_index = effect_base_index_end - 1;

	// set unit data
	memcpy(&effect[effect_index_end], &effect_base[base_index], sizeof(unit_effect_data_t));
	effect[effect_index_end].base = &effect_base[base_index];
	effect[effect_index_end].id = effect_index_end;
	effect[effect_index_end].type = UNIT_TYPE_EFFECT;
	effect[effect_index_end].group = effect_base[base_index].group;
	effect[effect_index_end].trace_unit = NULL;
	effect[effect_index_end].clear_type = effect_base[base_index].clear_type;

	// collision
	if (effect_base[base_index].col_shape->type & COLLISION_ID_STATIC_SHAPE) {
		effect[effect_index_end].col_shape =
			collision_manager_create_static_shape(effect_base[base_index].col_shape,
				(void*)&effect[effect_index_end], effect_base[base_index].anim->base_w, effect_base[base_index].anim->base_h,
				&x, &y);
	}
	else {
		effect[effect_index_end].col_shape =
			collision_manager_create_dynamic_shape(effect_base[base_index].col_shape,
				(void*)&effect[effect_index_end], effect_base[base_index].anim->base_w, effect_base[base_index].anim->base_h,
				&x, &y);
	}

	// anim
	effect[effect_index_end].anim = animation_manager_new_anim_data();
	effect[effect_index_end].anim->stat = ANIM_STAT_FLAG_IDLE;
	effect[effect_index_end].anim->type = effect[effect_index_end].base->anim->type;
	effect[effect_index_end].anim->obj = effect[effect_index_end].base->anim->obj;
	for (int i = 0; i < ANIM_STAT_END; i++) {
		effect[effect_index_end].anim->anim_stat_base_list[i] = effect[effect_index_end].base->anim->anim_stat_base_list[i];
	}

	// set stat SPAWN
	if (effect[effect_index_end].anim->anim_stat_base_list[ANIM_STAT_SPAWN]->obj) {
		unit_manager_effect_set_anim_stat(effect_index_end, ANIM_STAT_FLAG_SPAWN);
	}

	effect_index_end++;
	if (effect_index_end >= UNIT_EFFECT_LIST_SIZE) {
		effect_index_end = 0;
	}

	return ret;
}

void unit_manager_clear_all_effect(int clear_type)
{
	for (int i = 0; i < UNIT_EFFECT_LIST_SIZE; i++) {
		if (effect[i].type != UNIT_TYPE_EFFECT) continue;
		if (effect[i].clear_type == clear_type) {
			unit_manager_clear_effect(&effect[i]);
		}
	}
	//effect_index_end = 0;
}

void unit_manager_clear_effect(unit_effect_data_t* effect_data)
{
	effect_data->type = UNIT_TYPE_NONE;
	effect_data->base = NULL;
	effect_data->trace_unit = NULL;
	collision_manager_delete_shape(effect_data->col_shape);
	effect_data->col_shape = NULL;
	animation_manager_delete_anim_stat_data(effect_data->anim);
	effect_data->anim = NULL;
}

void unit_manager_effect_update()
{
	for (int i = 0; i < UNIT_EFFECT_LIST_SIZE; i++) {
		if (effect[i].type != UNIT_TYPE_EFFECT) continue;

		if (effect[i].col_shape->stat == COLLISION_STAT_ENABLE) {
			bool set_stat_die = false;
#ifdef _COLLISION_ENABLE_BOX_2D_
			if (effect[i].trace_unit) {
				if (effect[i].trace_unit->col_shape && effect[i].trace_unit->col_shape->b2body) {
					int pos_x, pos_y;
					unit_manager_get_position(effect[i].trace_unit, &pos_x, &pos_y);
					unit_manager_effect_set_b2position(i, PIX2MET(pos_x), PIX2MET(pos_y));
				}
				else { // parent die?
					set_stat_die = true;
				}
			}

			effect[i].col_shape->x = (int)MET2PIX(effect[i].col_shape->b2body->GetPosition().x);
			effect[i].col_shape->y = (int)MET2PIX(effect[i].col_shape->b2body->GetPosition().y);
#endif
			if (effect[i].group == UNIT_EFFECT_GROUP_DIE_AUTO) {
				if (effect[i].life_timer > 0) {
					effect[i].life_timer -= g_delta_time;
				}
				else {
					set_stat_die = true;
				}
			}

			if (set_stat_die) {
				// must set stat die after have referred col_shape
				unit_manager_effect_set_anim_stat(i, ANIM_STAT_FLAG_DIE);
			}
		}

		// anim update
		int stat = unit_manager_unit_get_anim_stat((unit_data_t*)&effect[i]);
		if ((stat != -1) && (effect[i].anim->anim_stat_base_list[stat]->type == ANIM_TYPE_STATIC)) {
			if (effect[i].anim->stat == ANIM_STAT_FLAG_DIE) {
				unit_manager_clear_effect(&effect[i]);
				continue;
			}
		}

		if ((stat != -1)
			&& ((effect[i].anim->anim_stat_base_list[stat]->type == ANIM_TYPE_DYNAMIC) || (effect[i].anim->anim_stat_base_list[stat]->type & ANIM_TYPE_DRAW))) {
			// set current_time
			effect[i].anim->anim_stat_list[stat]->current_time += g_delta_time;

			int new_time = effect[i].anim->anim_stat_list[stat]->current_time;
			int total_time = effect[i].anim->anim_stat_base_list[stat]->total_time;
			if (new_time > total_time) {
				new_time = new_time % total_time;
				effect[i].anim->anim_stat_list[stat]->current_time = new_time;

				// end frame event
				if (effect[i].anim->stat == ANIM_STAT_FLAG_DIE) {
					unit_manager_clear_effect(&effect[i]);
					continue;
				}
				if (effect[i].anim->stat == ANIM_STAT_FLAG_SPAWN) {
					unit_manager_effect_set_anim_stat(i, ANIM_STAT_FLAG_IDLE);
					continue;
				}
			}

			// set current_frame
			int sum_frame_time = 0;
			int frame_size = effect[i].anim->anim_stat_base_list[stat]->frame_size;
			for (int fi = 0; fi < frame_size; fi++) {
				sum_frame_time += effect[i].anim->anim_stat_base_list[stat]->frame_list[fi]->frame_time;
				if (new_time < sum_frame_time) {
					if (effect[i].anim->anim_stat_list[stat]->current_frame != fi) {
						// send command
						if (effect[i].anim->anim_stat_base_list[stat]->frame_list[fi]->command == ANIM_FRAME_COMMAND_ON) {
							game_event_t msg = { (EVENT_MSG_UNIT_EFFECT | (0x00000001 << stat)), NULL };
							game_event_push(&msg);
							effect[i].anim->anim_stat_list[stat]->command_frame = fi;
						}
						// set frame
						effect[i].anim->anim_stat_list[stat]->current_frame = fi;
					}
					break;
				}
			}
		}
	}
}

void unit_manager_effect_display(int layer)
{
	for (int i = 0; i < UNIT_EFFECT_LIST_SIZE; i++) {
		if (effect[i].type != UNIT_TYPE_EFFECT || (effect[i].col_shape->stat & ANIM_STAT_FLAG_HIDE)) continue;

		unit_display((unit_data_t*)&effect[i], layer);
	}
}
