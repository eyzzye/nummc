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

static unit_trap_data_t trap_base[UNIT_TRAP_BASE_LIST_SIZE];
static unit_trap_data_t trap[UNIT_TRAP_LIST_SIZE];
static int trap_base_index_end;
static int trap_index_end;

static void load_trap_unit(std::string& line);

//
// trap
//
int unit_manager_init_trap()
{
	memset(&trap_base, 0, sizeof(trap_base));
	memset(&trap, 0, sizeof(trap));
	trap_base_index_end = 0;
	trap_index_end = 0;

	return 0;
}

void unit_manager_unload_trap()
{
	for (int i = 0; i < UNIT_TRAP_BASE_LIST_SIZE; i++) {
		if (trap_base[i].obj) {
			delete[] trap_base[i].obj;
			trap_base[i].obj = NULL;
		}
	}
}

int unit_manager_search_trap(std::string& path)
{
	int ii = 0;
	bool trap_found = false;
	while (trap_base[ii].type == UNIT_TYPE_TRAP) {
		std::string regist_path = (char*)trap_base[ii].obj;
		if (regist_path == path) {
			trap_found = true;
			break;
		}
		ii++;
	}

	if (trap_found) return ii;  // regist already
	else return (-1);           // not found
}

unit_trap_data_t* unit_manager_get_trap(int index)
{
	return &trap[index];
}

bool unit_manager_trap_within(int x, int y)
{
	bool ret = false;

	for (int i = 0; i < UNIT_TRAP_LIST_SIZE; i++) {
		if ((trap[i].type == UNIT_TYPE_TRAP) && (trap[i].col_shape->stat == COLLISION_STAT_ENABLE)) {
			int trap_x = 0, trap_y = 0, trap_w = 0, trap_h = 0;
			if (trap[i].col_shape->type == COLLISION_TYPE_BOX_S) {
				trap_x = trap[i].col_shape->x + trap[i].col_shape->offset_x;
				trap_y = trap[i].col_shape->y + trap[i].col_shape->offset_y;
				trap_w = ((shape_box_data*)trap[i].col_shape)->w;
				trap_h = ((shape_box_data*)trap[i].col_shape)->h;
			}
			else if (trap[i].col_shape->type == COLLISION_TYPE_ROUND_S) {
				int r = ((shape_round_data*)trap[i].col_shape)->r;
				trap_x = trap[i].col_shape->x - r;
				trap_y = trap[i].col_shape->y - r;
				trap_w = trap_h = 2 * r;
			}
			else {
				// becouse ghost trap is Dynamic, skip decision
				//LOG_ERROR("Error: unit_manager_trap_within() get irregular type.");
				continue;
			}

			if ((trap_x <= x) && (x <= trap_x + trap_w) && (trap_y <= y) && (y <= trap_y + trap_h)) {
				ret = true;
				break;
			}
		}
	}

	return ret;
}

void unit_manager_trap_set_anim_stat(int unit_id, int stat)
{
	unit_manager_unit_set_anim_stat((unit_data_t*)&trap[unit_id], stat);

	// DIE
	if ((trap[unit_id].anim->stat == ANIM_STAT_FLAG_DIE) && trap[unit_id].col_shape->b2body) {
		g_stage_world->DestroyBody(trap[unit_id].col_shape->b2body);
		trap[unit_id].col_shape->b2body = NULL;
	}
}

int unit_manager_load_trap(std::string path)
{
	if (unit_manager_search_trap(path) > 0) return 0;

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
				trap_base[trap_base_index_end].obj = (void*)path_c_str;
				trap_base[trap_base_index_end].type = UNIT_TYPE_TRAP;
				trap_base[trap_base_index_end].id = trap_base_index_end;
				trap_base[trap_base_index_end].sub_id = 0;
				continue;
			}
			if (line == "[/unit]") { read_flg[UNIT_TAG_UNIT] = false; continue; }
			if (line == "[collision]") { read_flg[UNIT_TAG_COLLISION] = true;  continue; }
			if (line == "[/collision]") { read_flg[UNIT_TAG_COLLISION] = false; continue; }
			if (line == "[anim]") {
				read_flg[UNIT_TAG_ANIM] = true;
				trap_base[trap_base_index_end].anim = animation_manager_new_anim_data();
				animation_manager_new_anim_stat_base_data(trap_base[trap_base_index_end].anim);
				continue;
			}
			if (line == "[/anim]") { read_flg[UNIT_TAG_ANIM] = false; continue; }

			if (read_flg[UNIT_TAG_UNIT]) {
				load_trap_unit(line);
			}
			if (read_flg[UNIT_TAG_COLLISION]) {
				load_collision(line, &trap_base[trap_base_index_end].col_shape);
			}
			if (read_flg[UNIT_TAG_ANIM]) {
				load_anim(line, trap_base[trap_base_index_end].anim);
			}
		}
		inFile.close();
	}
	else {
		LOG_ERROR("unit_manager_load_trap %s error\n", path.c_str());
		return 1;
	}

	// load anim files
	if (trap_base[trap_base_index_end].anim) {
		for (int i = 0; i < ANIM_STAT_END; i++) {
			char* cstr_anim_path = (char*)trap_base[trap_base_index_end].anim->anim_stat_base_list[i]->obj;
			if (cstr_anim_path) {
				std::string anim_path = cstr_anim_path;
				animation_manager_load_file(anim_path, trap_base[trap_base_index_end].anim, i);
			}
		}
	}

	trap_base_index_end++;
	return 0;
}

static void load_trap_unit(std::string& line)
{
	std::string key, value;
	game_utils_split_key_value(line, key, value);

	if (value == "") value = "0";
	if (key == "hp") trap_base[trap_base_index_end].hp = atoi(value.c_str());
	if (key == "sub_id") trap_base[trap_base_index_end].sub_id = atoi(value.c_str());
	if (key == "group") {
		if (value == "NONE") {
			trap_base[trap_base_index_end].group = UNIT_TRAP_GROUP_NONE;
		}
		else if (value == "RECOVERY") {
			trap_base[trap_base_index_end].group = UNIT_TRAP_GROUP_RECOVERY;
		}
		else if (value == "DAMAGE") {
			trap_base[trap_base_index_end].group = UNIT_TRAP_GROUP_DAMAGE;
		}
		else if (value == "FIRE") {
			trap_base[trap_base_index_end].group = UNIT_TRAP_GROUP_FIRE;
		}
		else if (value == "ICE") {
			trap_base[trap_base_index_end].group = UNIT_TRAP_GROUP_ICE;
		}
		else if (value == "GATE") {
			trap_base[trap_base_index_end].group = UNIT_TRAP_GROUP_GATE;
		}
	}
}

int unit_manager_create_trap(int x, int y, int base_index)
{
	int ret = -1;

	for (int i = trap_index_end; i < UNIT_TRAP_LIST_SIZE; i++) {
		if (trap[i].type != UNIT_TYPE_TRAP) {
			ret = trap_index_end = i;
			break;
		}
	}
	if ((ret == -1) && (trap_index_end > 0)) {
		for (int i = 0; i < trap_index_end; i++) {
			if (trap[i].type != UNIT_TYPE_TRAP) {
				ret = trap_index_end = i;
				break;
			}
		}
	}
	if (ret == -1) {
		LOG_ERROR("Error: unit_manager_create_trap() overflow.");
		return ret;
	}

	if (base_index == -1) base_index = trap_base_index_end - 1;

	// set unit data
	memcpy(&trap[trap_index_end], &trap_base[base_index], sizeof(unit_trap_data_t));
	trap[trap_index_end].base = &trap_base[base_index];
	trap[trap_index_end].id = trap_index_end;
	trap[trap_index_end].type = UNIT_TYPE_TRAP;
	trap[trap_index_end].group = trap_base[base_index].group;
	trap[trap_index_end].sub_id = trap_base[base_index].sub_id;
	trap[trap_index_end].trace_unit = NULL;

	// collision
	if (trap_base[base_index].col_shape->type & COLLISION_ID_STATIC_SHAPE) {
		trap[trap_index_end].col_shape =
			collision_manager_create_static_shape(trap_base[base_index].col_shape,
				(void*)&trap[trap_index_end], trap_base[base_index].anim->base_w, trap_base[base_index].anim->base_h,
				&x, &y);
	}
	else {
		trap[trap_index_end].col_shape =
			collision_manager_create_dynamic_shape(trap_base[base_index].col_shape,
				(void*)&trap[trap_index_end], trap_base[base_index].anim->base_w, trap_base[base_index].anim->base_h,
				&x, &y);
	}

	// anim
	trap[trap_index_end].anim = animation_manager_new_anim_data();
	trap[trap_index_end].anim->stat = ANIM_STAT_FLAG_IDLE;
	trap[trap_index_end].anim->type = trap[trap_index_end].base->anim->type;
	trap[trap_index_end].anim->obj = trap[trap_index_end].base->anim->obj;
	for (int i = 0; i < ANIM_STAT_END; i++) {
		trap[trap_index_end].anim->anim_stat_base_list[i] = trap[trap_index_end].base->anim->anim_stat_base_list[i];
	}

	// set stat SPAWN
	if (trap[trap_index_end].anim->anim_stat_base_list[ANIM_STAT_SPAWN]->obj) {
		unit_manager_trap_set_anim_stat(trap_index_end, ANIM_STAT_FLAG_SPAWN);
	}

	trap_index_end++;
	if (trap_index_end >= UNIT_TRAP_LIST_SIZE) {
		trap_index_end = 0;
	}

	return ret;
}

void unit_manager_clear_all_trap()
{
	for (int i = 0; i < UNIT_TRAP_LIST_SIZE; i++) {
		if (trap[i].type != UNIT_TYPE_TRAP) continue;
		unit_manager_clear_trap(&trap[i]);
	}
	trap_index_end = 0;
}

void unit_manager_clear_trap(unit_trap_data_t* trap_data)
{
	trap_data->type = UNIT_TYPE_NONE;
	trap_data->base = NULL;
	trap_data->trace_unit = NULL;
	collision_manager_delete_shape(trap_data->col_shape);
	trap_data->col_shape = NULL;
	animation_manager_delete_anim_stat_data(trap_data->anim);
	trap_data->anim = NULL;
}

void unit_manager_trap_update()
{
	for (int i = 0; i < UNIT_TRAP_LIST_SIZE; i++) {
		if (trap[i].type != UNIT_TYPE_TRAP) continue;

		if (trap[i].col_shape->stat == COLLISION_STAT_ENABLE) {
#ifdef _COLLISION_ENABLE_BOX_2D_
			trap[i].col_shape->x = (int)MET2PIX(trap[i].col_shape->b2body->GetPosition().x);
			trap[i].col_shape->y = (int)MET2PIX(trap[i].col_shape->b2body->GetPosition().y);
#endif
		}

		// anim update
		int stat = unit_manager_unit_get_anim_stat((unit_data_t*)&trap[i]);
		if ((stat != -1) && (trap[i].anim->anim_stat_base_list[stat]->type == ANIM_TYPE_STATIC)) {
			if (trap[i].anim->stat == ANIM_STAT_FLAG_DIE) {
				unit_manager_clear_trap(&trap[i]);
				continue;
			}
		}

		if ((stat != -1) && (trap[i].anim->anim_stat_base_list[stat]->type == ANIM_TYPE_DYNAMIC)) {
			// set current_time
			trap[i].anim->anim_stat_list[stat]->current_time += g_delta_time;

			int new_time = trap[i].anim->anim_stat_list[stat]->current_time;
			int total_time = trap[i].anim->anim_stat_base_list[stat]->total_time;
			if (new_time > total_time) {
				new_time = new_time % total_time;
				trap[i].anim->anim_stat_list[stat]->current_time = new_time;

				// end frame event
				if (trap[i].anim->stat == ANIM_STAT_FLAG_DIE) {
					unit_manager_clear_trap(&trap[i]);
					continue;
				}
				if (trap[i].anim->stat == ANIM_STAT_FLAG_SPAWN) {
					unit_manager_trap_set_anim_stat(i, ANIM_STAT_FLAG_IDLE);
					continue;
				}
			}

			// set current_frame
			int sum_frame_time = 0;
			int frame_size = trap[i].anim->anim_stat_base_list[stat]->frame_size;
			for (int fi = 0; fi < frame_size; fi++) {
				sum_frame_time += trap[i].anim->anim_stat_base_list[stat]->frame_list[fi]->frame_time;
				if (new_time < sum_frame_time) {
					if (trap[i].anim->anim_stat_list[stat]->current_frame != fi) {
						// send command
						if (trap[i].anim->anim_stat_base_list[stat]->frame_list[fi]->command == ANIM_FRAME_COMMAND_ON) {
							game_event_unit_t* msg_param = new game_event_unit_t;
							msg_param->obj1 = (unit_data_t*)(&trap[i]);
							game_event_t msg = { (EVENT_MSG_UNIT_TRAP | (0x00000001 << stat)), (void*)msg_param };
							game_event_push(&msg);
							trap[i].anim->anim_stat_list[stat]->command_frame = fi;
						}
						// set frame
						trap[i].anim->anim_stat_list[stat]->current_frame = fi;
					}
					break;
				}
			}
		}
	}
}

void unit_manager_trap_display(int layer)
{
	for (int i = 0; i < UNIT_TRAP_LIST_SIZE; i++) {
		if (trap[i].type != UNIT_TYPE_TRAP) continue;

		unit_display((unit_data_t*)&trap[i], layer);
	}
}
