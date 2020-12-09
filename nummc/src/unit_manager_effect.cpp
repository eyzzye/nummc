#include <fstream>
#include "game_common.h"
#include "unit_manager.h"

#include "game_timer.h"
#include "game_log.h"
#include "game_utils.h"
#include "game_window.h"
#include "game_event.h"

#include "resource_manager.h"

#define EFFECT_BASE_LIST_SIZE (UNIT_EFFECT_LIST_SIZE/8)
static unit_effect_data_t effect_base[EFFECT_BASE_LIST_SIZE];
static unit_effect_data_t effect[UNIT_EFFECT_LIST_SIZE];
static int effect_base_index_end;
static int effect_index_end;

static void load_effect_unit(std::string& line);

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
	for (int i = 0; i < EFFECT_BASE_LIST_SIZE; i++) {
		if (effect_base[i].obj) {
			delete[] effect_base[i].obj;
			effect_base[i].obj = NULL;
		}
	}
}

int unit_manager_search_effect(std::string& path)
{
	int ii = 0;
	bool effect_found = false;
	while (effect_base[ii].type == UNIT_TYPE_EFFECT) {
		std::string regist_path = (char*)effect_base[ii].obj;
		if (regist_path == path) {
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
}

unit_effect_data_t* unit_manager_get_effect(int index)
{
	return &effect[index];
}

int unit_manager_load_effect(std::string path)
{
	if (unit_manager_search_effect(path) > 0) return 0;

	bool read_flg[UNIT_TAG_END] = { false };
	std::ifstream inFile(g_base_path + "data/" + path);
	if (inFile.is_open()) {
		std::string line;
		while (std::getline(inFile, line)) {
			if (line == "") continue;
			if (line[0] == '#') continue;

			if (line == "[unit]") {
				read_flg[UNIT_TAG_UNIT] = true;

				// set base unit data
				char* path_c_str = new char[path.size() + 1];
				memcpy(path_c_str, path.c_str(), path.size());
				path_c_str[path.size()] = '\0';
				effect_base[effect_base_index_end].obj = (void*)path_c_str;
				effect_base[effect_base_index_end].type = UNIT_TYPE_EFFECT;
				effect_base[effect_base_index_end].id = effect_base_index_end;
				continue;
			}
			if (line == "[/unit]") { read_flg[UNIT_TAG_UNIT] = false; continue; }
			if (line == "[collision]") { read_flg[UNIT_TAG_COLLISION] = true;  continue; }
			if (line == "[/collision]") { read_flg[UNIT_TAG_COLLISION] = false; continue; }
			if (line == "[anim]") {
				read_flg[UNIT_TAG_ANIM] = true;
				effect_base[effect_base_index_end].anim = animation_manager_new_anim_data();
				animation_manager_new_anim_stat_base_data(effect_base[effect_base_index_end].anim);
				continue;
			}
			if (line == "[/anim]") { read_flg[UNIT_TAG_ANIM] = false; continue; }

			if (read_flg[UNIT_TAG_UNIT]) {
				load_effect_unit(line);
			}
			if (read_flg[UNIT_TAG_COLLISION]) {
				load_collision(line, &effect_base[effect_base_index_end].col_shape);
			}
			if (read_flg[UNIT_TAG_ANIM]) {
				load_anim(line, effect_base[effect_base_index_end].anim);
			}
		}
		inFile.close();
	}
	else {
		LOG_ERROR("unit_manager_load_effect %s error\n", path.c_str());
		return 1;
	}

	// load anim files
	if (effect_base[effect_base_index_end].anim) {
		for (int i = 0; i < ANIM_STAT_END; i++) {
			char* cstr_anim_path = (char*)effect_base[effect_base_index_end].anim->anim_stat_base_list[i]->obj;
			if (cstr_anim_path) {
				std::string anim_path = cstr_anim_path;
				animation_manager_load_file(anim_path, effect_base[effect_base_index_end].anim, i);
			}
		}
	}

	effect_base_index_end++;
	return 0;
}

static void load_effect_unit(std::string& line)
{
	std::string key, value;
	game_utils_split_key_value(line, key, value);

	if (value == "") value = "0";
	if (key == "group") {
		if (value == "NONE") {
			effect_base[effect_base_index_end].group = UNIT_EFFECT_GROUP_NONE;
		}
		else if (value == "STATIC") {
			effect_base[effect_base_index_end].group = UNIT_EFFECT_GROUP_STATIC;
		}
		else if (value == "DYNAMIC") {
			effect_base[effect_base_index_end].group = UNIT_EFFECT_GROUP_DYNAMIC;
		}
		else if (value == "DIE_AUTO") {
			effect_base[effect_base_index_end].group = UNIT_EFFECT_GROUP_DIE_AUTO;
		}
	}
	if (key == "life_timer") effect_base[effect_base_index_end].life_timer = atoi(value.c_str());
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

void unit_manager_clear_effect(unit_effect_data_t* effect_data)
{
	effect_data->type = UNIT_TYPE_NONE;
	effect_data->base = NULL;
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
					unit_manager_effect_set_b2position(i, effect[i].trace_unit->col_shape->b2body->GetPosition().x, effect[i].trace_unit->col_shape->b2body->GetPosition().y);
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

		if ((stat != -1) && (effect[i].anim->anim_stat_base_list[stat]->type == ANIM_TYPE_DYNAMIC)) {
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
