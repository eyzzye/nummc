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

const char* g_player_bullet_path[UNIT_BULLET_ID_END] = {
	"",
	"units/bullet/point/point.unit",
	"units/bullet/big_point/big_point.unit",
	"units/bullet/fire/fire.unit",
	"units/bullet/ice/ice.unit",
	"units/bullet/leaser/leaser.unit",
};

static unit_player_bullet_data_t player_bullet_base[UNIT_PLAYER_BULLET_BASE_LIST_SIZE];
static unit_player_bullet_data_t player_bullet[UNIT_PLAYER_BULLET_LIST_SIZE];
static int player_bullet_base_index_end;
static int player_bullet_index_end;

static void load_bullet_unit(std::string& line);

//
// player bullet
//
int unit_manager_init_player_bullet()
{
	memset(&player_bullet_base, 0, sizeof(player_bullet_base));
	memset(&player_bullet, 0, sizeof(player_bullet));
	player_bullet_base_index_end = 0;
	player_bullet_index_end = 0;

	return 0;
}

void unit_manager_unload_player_bullet()
{
	for (int i = 0; i < UNIT_PLAYER_BULLET_BASE_LIST_SIZE; i++) {
		if (player_bullet_base[i].obj) {
			delete[] player_bullet_base[i].obj;
			player_bullet_base[i].obj = NULL;
		}
	}
}

int unit_manager_search_player_bullet(std::string& path)
{
	int i = 0;
	bool player_bullet_found = false;
	while (player_bullet_base[i].type == UNIT_TYPE_PLAYER_BULLET) {
		std::string regist_path = (char*)player_bullet_base[i].obj;
		if (regist_path == path) {
			player_bullet_found = true;
			break;
		}
		i++;
	}

	if (player_bullet_found) return i;  // regist already
	else return (-1);                   // not found
}

void unit_manager_player_bullet_set_anim_stat(int unit_id, int stat)
{
	unit_manager_unit_set_anim_stat((unit_data_t*)&player_bullet[unit_id], stat);

	// DIE
	if ((player_bullet[unit_id].anim->stat == ANIM_STAT_FLAG_DIE) && player_bullet[unit_id].col_shape->b2body) {
		g_stage_world->DestroyBody(player_bullet[unit_id].col_shape->b2body);
		player_bullet[unit_id].col_shape->b2body = NULL;
	}
}

void unit_manager_player_bullet_set_effect_stat(int unit_id, int stat)
{
	player_bullet[unit_id].effect_stat = stat;
}

void unit_manager_player_bullet_set_hp(int unit_id, int hp)
{
	player_bullet[unit_id].hp = hp;
}

void unit_manager_player_bullet_set_bullet_life_timer(int unit_id, int bullet_life_timer)
{
	player_bullet[unit_id].bullet_life_timer = bullet_life_timer;
}

unit_player_bullet_data_t* unit_manager_get_player_bullet_base(int index) {
	return &player_bullet_base[index];
}

int unit_manager_load_player_bullet(std::string path)
{
	if (unit_manager_search_player_bullet(path) > 0) return 0;

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
				player_bullet_base[player_bullet_base_index_end].obj = (void*)path_c_str;
				player_bullet_base[player_bullet_base_index_end].type = UNIT_TYPE_PLAYER_BULLET;
				player_bullet_base[player_bullet_base_index_end].id = player_bullet_base_index_end;
				continue;
			}
			if (line == "[/unit]") { read_flg[UNIT_TAG_UNIT] = false; continue; }
			if (line == "[collision]") { read_flg[UNIT_TAG_COLLISION] = true;  continue; }
			if (line == "[/collision]") { read_flg[UNIT_TAG_COLLISION] = false; continue; }
			if (line == "[anim]") {
				read_flg[UNIT_TAG_ANIM] = true;
				player_bullet_base[player_bullet_base_index_end].anim = animation_manager_new_anim_data();
				animation_manager_new_anim_stat_base_data(player_bullet_base[player_bullet_base_index_end].anim);
				continue;
			}
			if (line == "[/anim]") { read_flg[UNIT_TAG_ANIM] = false; continue; }

			if (read_flg[UNIT_TAG_UNIT]) {
				load_bullet_unit(line);
			}
			if (read_flg[UNIT_TAG_COLLISION]) {
				load_collision(line, &player_bullet_base[player_bullet_base_index_end].col_shape);
			}
			if (read_flg[UNIT_TAG_ANIM]) {
				load_anim(line, player_bullet_base[player_bullet_base_index_end].anim);
			}
		}
		inFile.close();
	}
	else {
		LOG_ERROR("unit_manager_load_player_bullet %s error\n", path.c_str());
		return 1;
	}

	// load anim files
	if (player_bullet_base[player_bullet_base_index_end].anim) {
		for (int i = 0; i < ANIM_STAT_END; i++) {
			char* cstr_anim_path = (char*)player_bullet_base[player_bullet_base_index_end].anim->anim_stat_base_list[i]->obj;
			if (cstr_anim_path) {
				std::string anim_path = cstr_anim_path;
				animation_manager_load_file(anim_path, player_bullet_base[player_bullet_base_index_end].anim, i);
			}
		}
	}

	player_bullet_base_index_end++;

	return 0;
}

static void load_bullet_unit(std::string& line)
{
	std::string key, value;
	game_utils_split_key_value(line, key, value);

	if (value == "") value = "0";
	if (key == "hp") player_bullet_base[player_bullet_base_index_end].hp = atoi(value.c_str());
	if (key == "speed") player_bullet_base[player_bullet_base_index_end].speed = atoi(value.c_str());
	if (key == "special") {
		if (value == "NONE") {
			player_bullet_base[player_bullet_base_index_end].special = UNIT_BULLET_TYPE_NONE;
		}
		else if (value == "FIRE") {
			player_bullet_base[player_bullet_base_index_end].special = UNIT_BULLET_TYPE_FIRE;
		}
		else if (value == "ICE") {
			player_bullet_base[player_bullet_base_index_end].special = UNIT_BULLET_TYPE_ICE;
		}
	}
	if (key == "special_value") player_bullet_base[player_bullet_base_index_end].special_value = atoi(value.c_str());
}

int unit_manager_create_player_bullet(int x, int y, float vec_x, float vec_y, int face, int base_index)
{
	int ret = player_bullet_index_end;

	if (player_bullet[player_bullet_index_end].type == UNIT_TYPE_PLAYER_BULLET) {
		unit_manager_clear_player_bullet(&player_bullet[player_bullet_index_end]);
	}

	if (base_index == -1) base_index = player_bullet_base_index_end - 1;

	// set unit data
	memcpy(&player_bullet[player_bullet_index_end], &player_bullet_base[base_index], sizeof(unit_player_bullet_data_t));
	player_bullet[player_bullet_index_end].base = &player_bullet_base[base_index];
	player_bullet[player_bullet_index_end].id = player_bullet_index_end;
	player_bullet[player_bullet_index_end].type = UNIT_TYPE_PLAYER_BULLET;

	// collision
	int tmp_x, tmp_y;
	if ((player_bullet[player_bullet_index_end].base->col_shape->type == COLLISION_TYPE_BOX_D)
		&& (player_bullet[player_bullet_index_end].base->col_shape->face_type == UNIT_FACE_TYPE_ALL)) {
		if ((face == UNIT_FACE_N) || (face == UNIT_FACE_S)) {
			tmp_x = x - player_bullet[player_bullet_index_end].col_shape->offset_y;
			tmp_y = y - player_bullet[player_bullet_index_end].col_shape->offset_x;
		}
		else {
			tmp_x = x - player_bullet[player_bullet_index_end].col_shape->offset_x;
			tmp_y = y - player_bullet[player_bullet_index_end].col_shape->offset_y;
		}
	}
	else {
		tmp_x = x - player_bullet[player_bullet_index_end].col_shape->offset_x;
		tmp_y = y - player_bullet[player_bullet_index_end].col_shape->offset_y;
	}

	float tmp_vec_x = vec_x * player_bullet[player_bullet_index_end].col_shape->vec_x_delta;
	float tmp_vec_y = vec_y * player_bullet[player_bullet_index_end].col_shape->vec_y_delta;
	player_bullet[player_bullet_index_end].col_shape =
		collision_manager_create_dynamic_shape(player_bullet_base[base_index].col_shape,
			(void*)&player_bullet[player_bullet_index_end], player_bullet_base[base_index].anim->base_w, player_bullet_base[base_index].anim->base_h,
			&tmp_x, &tmp_y, &tmp_vec_x, &tmp_vec_y, &face);
	collision_manager_set_mass(player_bullet[player_bullet_index_end].col_shape, 0.01f);

	// anim
	player_bullet[player_bullet_index_end].anim = animation_manager_new_anim_data();
	player_bullet[player_bullet_index_end].anim->stat = ANIM_STAT_FLAG_IDLE;
	player_bullet[player_bullet_index_end].anim->type = player_bullet[player_bullet_index_end].base->anim->type;
	player_bullet[player_bullet_index_end].anim->obj = player_bullet[player_bullet_index_end].base->anim->obj;
	for (int i = 0; i < ANIM_STAT_END; i++) {
		player_bullet[player_bullet_index_end].anim->anim_stat_base_list[i] = player_bullet[player_bullet_index_end].base->anim->anim_stat_base_list[i];
	}

	player_bullet_index_end++;
	if (player_bullet_index_end >= UNIT_PLAYER_BULLET_LIST_SIZE) {
		player_bullet_index_end = 0;
	}

	return ret;
}

void unit_manager_clear_player_bullet(unit_player_bullet_data_t* bullet)
{
	bullet->type = UNIT_TYPE_NONE;
	bullet->base = NULL;
	collision_manager_delete_shape(bullet->col_shape);
	bullet->col_shape = NULL;
	animation_manager_delete_anim_stat_data(bullet->anim);
	bullet->anim = NULL;
}

void unit_manager_player_bullet_update()
{
	for (int i = 0; i < UNIT_PLAYER_BULLET_LIST_SIZE; i++) {
		if (player_bullet[i].type != UNIT_TYPE_PLAYER_BULLET) continue;

		if (player_bullet[i].col_shape->stat == COLLISION_STAT_ENABLE) {
#ifdef _COLLISION_ENABLE_BOX_2D_
			player_bullet[i].col_shape->x = (int)MET2PIX(player_bullet[i].col_shape->b2body->GetPosition().x);
			player_bullet[i].col_shape->y = (int)MET2PIX(player_bullet[i].col_shape->b2body->GetPosition().y);
			player_bullet[i].col_shape->vec_x = player_bullet[i].col_shape->b2body->GetLinearVelocity().x;  // box2d METER value
			player_bullet[i].col_shape->vec_y = player_bullet[i].col_shape->b2body->GetLinearVelocity().y;  // box2d METER value
#endif

			// bullet life update
			if (player_bullet[i].bullet_life_timer > 0) {
				player_bullet[i].bullet_life_timer -= g_delta_time;
			}
			else {
				unit_manager_player_bullet_set_anim_stat(i, ANIM_STAT_FLAG_DIE);
			}
		}

		// anim update
		int stat = unit_manager_unit_get_anim_stat((unit_data_t*)&player_bullet[i]);
		if ((stat != -1) && (player_bullet[i].anim->anim_stat_base_list[stat]->type == ANIM_TYPE_STATIC)) {
			if (player_bullet[i].anim->stat == ANIM_STAT_FLAG_DIE) {
				unit_manager_clear_player_bullet(&player_bullet[i]);
				continue;
			}
		}

		if ((stat != -1) && (player_bullet[i].anim->anim_stat_base_list[stat]->type == ANIM_TYPE_DYNAMIC)) {
			// set current_time
			player_bullet[i].anim->anim_stat_list[stat]->current_time += g_delta_time;

			int new_time = player_bullet[i].anim->anim_stat_list[stat]->current_time;
			int total_time = player_bullet[i].anim->anim_stat_base_list[stat]->total_time;
			if (new_time > total_time) {
				new_time = new_time % total_time;
				player_bullet[i].anim->anim_stat_list[stat]->current_time = new_time;

				// end frame event
				if (player_bullet[i].anim->stat == ANIM_STAT_FLAG_DIE) {
					unit_manager_clear_player_bullet(&player_bullet[i]);
					continue;
				}
			}

			// set current_frame
			int sum_frame_time = 0;
			int frame_size = player_bullet[i].anim->anim_stat_base_list[stat]->frame_size;
			for (int fi = 0; fi < frame_size; fi++) {
				sum_frame_time += player_bullet[i].anim->anim_stat_base_list[stat]->frame_list[fi]->frame_time;
				if (new_time < sum_frame_time) {
					if (player_bullet[i].anim->anim_stat_list[stat]->current_frame != fi) {
						// send command
						if (player_bullet[i].anim->anim_stat_base_list[stat]->frame_list[fi]->command == ANIM_FRAME_COMMAND_ON) {
							game_event_unit_t* msg_param = new game_event_unit_t;
							msg_param->obj1 = (unit_data_t*)(&player_bullet[i]);
							game_event_t msg = { (EVENT_MSG_UNIT_PLAYER_BULLET | (0x00000001 << stat)), (void*)msg_param };
							game_event_push(&msg);
							player_bullet[i].anim->anim_stat_list[stat]->command_frame = fi;
						}
						// set frame
						player_bullet[i].anim->anim_stat_list[stat]->current_frame = fi;
					}
					break;
				}
			}
		}
	}
}

void unit_manager_player_bullet_display(int layer)
{
	for (int i = 0; i < UNIT_PLAYER_BULLET_LIST_SIZE; i++) {
		if (player_bullet[i].type != UNIT_TYPE_PLAYER_BULLET) continue;

		unit_display((unit_data_t*)&player_bullet[i], layer);
	}
}
