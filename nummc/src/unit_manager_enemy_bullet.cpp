#include "game_common.h"
#include "unit_manager.h"

#include "game_timer.h"
#include "game_log.h"
#include "game_utils.h"
#include "game_window.h"
#include "game_event.h"

#include "resource_manager.h"
#include "memory_manager.h"
#include "stage_manager.h"
#include "map_manager.h"
#include "sound_manager.h"
#include "scene_play_stage.h"

const char* g_enemy_bullet_path[UNIT_BULLET_ID_END] = {
	"",
	"units/bullet/point/enemy/point.unit",
	"units/bullet/big_point/enemy/big_point.unit",
	"units/bullet/fire/enemy/fire.unit",
	"units/bullet/ice/enemy/ice.unit",
	"units/bullet/ice_ball/enemy/ice_ball.unit",
	"units/bullet/leaser/enemy/leaser.unit",
};

static unit_enemy_bullet_data_t enemy_bullet_base[UNIT_ENEMY_BULLET_BASE_LIST_SIZE];
static unit_enemy_bullet_data_t enemy_bullet[UNIT_ENEMY_BULLET_LIST_SIZE];
static int enemy_bullet_base_index_end;
static int enemy_bullet_index_end;

static void load_bullet_unit(char* line);

//
// enemy bullet
//
int unit_manager_init_enemy_bullet()
{
	memset(&enemy_bullet_base, 0, sizeof(enemy_bullet_base));
	memset(&enemy_bullet, 0, sizeof(enemy_bullet));
	enemy_bullet_base_index_end = 0;
	enemy_bullet_index_end = 0;

	return 0;
}

void unit_manager_unload_enemy_bullet()
{
	for (int i = 0; i < UNIT_ENEMY_BULLET_BASE_LIST_SIZE; i++) {
		if (enemy_bullet_base[i].obj) {
			memory_manager_delete_char_buff((char*)enemy_bullet_base[i].obj);
			enemy_bullet_base[i].obj = NULL;
		}
	}
}

int unit_manager_search_enemy_bullet(char* path)
{
	int i = 0;
	bool enemy_bullet_found = false;
	while (enemy_bullet_base[i].type == UNIT_TYPE_ENEMY_BULLET) {
		char* regist_path = (char*)enemy_bullet_base[i].obj;
		if ((regist_path != NULL) && STRCMP_EQ(regist_path,path)) {
			enemy_bullet_found = true;
			break;
		}
		i++;
	}

	if (enemy_bullet_found) return i;  // regist already
	else return (-1);                  // not found
}

void unit_manager_enemy_bullet_set_anim_stat(int unit_id, int stat)
{
	unit_manager_unit_set_anim_stat((unit_data_t*)&enemy_bullet[unit_id], stat);

	// DIE
	if ((enemy_bullet[unit_id].anim->stat == ANIM_STAT_FLAG_DIE) && enemy_bullet[unit_id].col_shape->b2body) {
		g_stage_world->DestroyBody(enemy_bullet[unit_id].col_shape->b2body);
		enemy_bullet[unit_id].col_shape->b2body = NULL;
	}
}

void unit_manager_enemy_bullet_set_effect_stat(int unit_id, int stat)
{
	enemy_bullet[unit_id].effect_stat = stat;
}

void unit_manager_enemy_bullet_set_hp(int unit_id, int hp)
{
	enemy_bullet[unit_id].hp = hp;
}

void unit_manager_enemy_bullet_set_bullet_life_timer(int unit_id, int bullet_life_timer)
{
	enemy_bullet[unit_id].bullet_life_timer = bullet_life_timer;
}

void unit_manager_enemy_bullet_set_force(int unit_id, float strength_x, float strength_y)
{
	collision_manager_set_force(enemy_bullet[unit_id].col_shape, strength_x, strength_y);
}

unit_enemy_bullet_data_t* unit_manager_get_enemy_bullet_base(int index) {
	return &enemy_bullet_base[index];
}

int unit_manager_enemy_bullet_get_delta_time(unit_enemy_bullet_data_t* enemy_bullet_data)
{
	int enemy_bullet_delta_time = g_delta_time;
	if ((g_stage_data->section_circumstance & SECTION_CIRCUMSTANCE_FLAG_SLOWED_ENEMY) || (enemy_bullet_data->effect_stat & UNIT_EFFECT_FLAG_E_FREEZE_UP)) {
		enemy_bullet_delta_time >>= 1; // div 2
	}
	return enemy_bullet_delta_time;
}

typedef struct _load_enemy_bullet_callback_data_t load_enemy_bullet_callback_data_t;
struct _load_enemy_bullet_callback_data_t {
	bool read_flg[UNIT_TAG_END];
	char* path;
};
static load_enemy_bullet_callback_data_t load_enemy_bullet_callback_data;
static void load_enemy_bullet_callback(char* line, int line_size, int line_num, void* argv)
{
	load_enemy_bullet_callback_data_t* data = (load_enemy_bullet_callback_data_t*)argv;

	if (line[0] == '\0') return;
	if (line[0] == '#') return;

	if (line[0] == '[') {
		if (STRCMP_EQ(line, "[unit]")) {
			data->read_flg[UNIT_TAG_UNIT] = true;

			// set base unit data
			char* path_c_str = memory_manager_new_char_buff((int)strlen(data->path));
			game_utils_string_copy(path_c_str, data->path);
			enemy_bullet_base[enemy_bullet_base_index_end].obj = (void*)path_c_str;
			enemy_bullet_base[enemy_bullet_base_index_end].type = UNIT_TYPE_ENEMY_BULLET;
			enemy_bullet_base[enemy_bullet_base_index_end].id = enemy_bullet_base_index_end;
			return;
		}
		if (STRCMP_EQ(line, "[/unit]"))      { data->read_flg[UNIT_TAG_UNIT]      = false; return; }
		if (STRCMP_EQ(line, "[collision]"))  { data->read_flg[UNIT_TAG_COLLISION] = true;  return; }
		if (STRCMP_EQ(line, "[/collision]")) { data->read_flg[UNIT_TAG_COLLISION] = false; return; }
		if (STRCMP_EQ(line, "[anim]")) {
			data->read_flg[UNIT_TAG_ANIM] = true;
			enemy_bullet_base[enemy_bullet_base_index_end].anim = animation_manager_new_anim_data();
			animation_manager_new_anim_stat_base_data(enemy_bullet_base[enemy_bullet_base_index_end].anim);
			return;
		}
		if (STRCMP_EQ(line, "[/anim]")) { data->read_flg[UNIT_TAG_ANIM] = false; return; }
	}

	if (data->read_flg[UNIT_TAG_UNIT]) {
		load_bullet_unit(line);
	}
	if (data->read_flg[UNIT_TAG_COLLISION]) {
		load_collision(line, &enemy_bullet_base[enemy_bullet_base_index_end].col_shape);
	}
	if (data->read_flg[UNIT_TAG_ANIM]) {
		load_anim(line, enemy_bullet_base[enemy_bullet_base_index_end].anim);
	}
}

int unit_manager_load_enemy_bullet(char* path)
{
	if (unit_manager_search_enemy_bullet(path) > 0) return 0;

	// full_path = g_base_path + "data/" + path;
	char full_path[GAME_FULL_PATH_MAX];
	int tmp_path_size = game_utils_string_cat(full_path, g_base_path, (char*)"data/", path);
	if (tmp_path_size == 0) { LOG_ERROR("unit_manager_load_enemy_bullet failed get %s\n", path); return 1; }

	// read file
	memset(load_enemy_bullet_callback_data.read_flg, 0, sizeof(bool) * UNIT_TAG_END);
	load_enemy_bullet_callback_data.path = path;
	int ret = game_utils_files_read_line(full_path, load_enemy_bullet_callback, (void*)&load_enemy_bullet_callback_data);
	if (ret != 0) { LOG_ERROR("unit_manager_load_enemy_bullet %s error\n", path); return 1; }

	// load anim files
	if (enemy_bullet_base[enemy_bullet_base_index_end].anim) {
		for (int i = 0; i < ANIM_STAT_END; i++) {
			char* anim_path = (char*)enemy_bullet_base[enemy_bullet_base_index_end].anim->anim_stat_base_list[i]->obj;
			if (anim_path) {
				animation_manager_load_file(anim_path, enemy_bullet_base[enemy_bullet_base_index_end].anim, i);
			}
		}
	}

	enemy_bullet_base_index_end++;

	if (enemy_bullet_base_index_end >= UNIT_ENEMY_BULLET_BASE_LIST_SIZE) {
		LOG_ERROR("ERROR: unit_manager_load_enemy_bullet() enemy_bullet_base overflow\n");
		return 1;
	}
	return 0;
}

static void load_bullet_unit(char* line)
{
	char key[GAME_UTILS_STRING_NAME_BUF_SIZE];
	char value[GAME_UTILS_STRING_NAME_BUF_SIZE];
	game_utils_split_key_value(line, key, value);

	if (STRCMP_EQ(key, "hp")) {
		enemy_bullet_base[enemy_bullet_base_index_end].hp = atoi(value);
		return;
	}
	if (STRCMP_EQ(key, "speed")) {
		enemy_bullet_base[enemy_bullet_base_index_end].speed = atoi(value);
		return;
	}
	if (STRCMP_EQ(key,"special")) {
		if (STRCMP_EQ(value,"NONE")) {
			enemy_bullet_base[enemy_bullet_base_index_end].special = UNIT_BULLET_TYPE_NONE;
		}
		else if (STRCMP_EQ(value,"FIRE")) {
			enemy_bullet_base[enemy_bullet_base_index_end].special = UNIT_BULLET_TYPE_FIRE;
		}
		else if (STRCMP_EQ(value,"ICE")) {
			enemy_bullet_base[enemy_bullet_base_index_end].special = UNIT_BULLET_TYPE_ICE;
		}
		return;
	}
	if (STRCMP_EQ(key, "special_value")) {
		enemy_bullet_base[enemy_bullet_base_index_end].special_value = atoi(value);
		return;
	}
}

int unit_manager_create_enemy_bullet(int x, int y, float vec_x, float vec_y, int face, int owner_base_id, int base_index, ai_data_t* ai_bullet)
{
	int ret = enemy_bullet_index_end;

	if (enemy_bullet[enemy_bullet_index_end].type == UNIT_TYPE_ENEMY_BULLET) {
		unit_manager_clear_enemy_bullet(&enemy_bullet[enemy_bullet_index_end]);
	}

	if (base_index == -1) base_index = enemy_bullet_base_index_end - 1;

	// set unit data
	memcpy(&enemy_bullet[enemy_bullet_index_end], &enemy_bullet_base[base_index], sizeof(unit_enemy_bullet_data_t));
	enemy_bullet[enemy_bullet_index_end].base = &enemy_bullet_base[base_index];
	enemy_bullet[enemy_bullet_index_end].id = enemy_bullet_index_end;
	enemy_bullet[enemy_bullet_index_end].type = UNIT_TYPE_ENEMY_BULLET;
	enemy_bullet[enemy_bullet_index_end].owner_base_id = owner_base_id;

	// collision
	int tmp_x, tmp_y;
	if ((enemy_bullet[enemy_bullet_index_end].base->col_shape->type == COLLISION_TYPE_BOX_D)
		&& (enemy_bullet[enemy_bullet_index_end].base->col_shape->face_type == UNIT_FACE_TYPE_ALL)) {
		if ((face == UNIT_FACE_N) || (face == UNIT_FACE_S)) {
			tmp_x = x - enemy_bullet[enemy_bullet_index_end].col_shape->offset_y;
			tmp_y = y - enemy_bullet[enemy_bullet_index_end].col_shape->offset_x;
		}
		else {
			tmp_x = x - enemy_bullet[enemy_bullet_index_end].col_shape->offset_x;
			tmp_y = y - enemy_bullet[enemy_bullet_index_end].col_shape->offset_y;
		}
	}
	else {
		tmp_x = x - enemy_bullet[enemy_bullet_index_end].col_shape->offset_x;
		tmp_y = y - enemy_bullet[enemy_bullet_index_end].col_shape->offset_y;
	}

	float tmp_vec_x = vec_x * enemy_bullet[enemy_bullet_index_end].col_shape->vec_x_delta;
	float tmp_vec_y = vec_y * enemy_bullet[enemy_bullet_index_end].col_shape->vec_y_delta;
	enemy_bullet[enemy_bullet_index_end].col_shape =
		collision_manager_create_dynamic_shape(enemy_bullet_base[base_index].col_shape,
			(void*)&enemy_bullet[enemy_bullet_index_end], enemy_bullet_base[base_index].anim->base_w, enemy_bullet_base[base_index].anim->base_h,
			&tmp_x, &tmp_y, &tmp_vec_x, &tmp_vec_y, &face);
	collision_manager_set_mass(enemy_bullet[enemy_bullet_index_end].col_shape, 0.01f);

	// anim
	enemy_bullet[enemy_bullet_index_end].anim = animation_manager_new_anim_data();
	enemy_bullet[enemy_bullet_index_end].anim->stat = ANIM_STAT_FLAG_IDLE;
	enemy_bullet[enemy_bullet_index_end].anim->type = enemy_bullet[enemy_bullet_index_end].base->anim->type;
	enemy_bullet[enemy_bullet_index_end].anim->obj = enemy_bullet[enemy_bullet_index_end].base->anim->obj;
	for (int i = 0; i < ANIM_STAT_END; i++) {
		enemy_bullet[enemy_bullet_index_end].anim->anim_stat_base_list[i] = enemy_bullet[enemy_bullet_index_end].base->anim->anim_stat_base_list[i];
	}

	// ai
	if (ai_bullet) {
		ai_stat_bullet_t* tmp_ai = (ai_stat_bullet_t*)ai_manager_new_ai_data();
		ai_manager_bullet_copy((ai_bullet_t*)tmp_ai, (ai_bullet_t*)ai_bullet);
		tmp_ai->timer1 = 0;
		tmp_ai->obj    = (void*)&enemy_bullet[enemy_bullet_index_end];
		enemy_bullet[enemy_bullet_index_end].ai_bullet = (ai_data_t*)tmp_ai;
	}

	enemy_bullet_index_end++;
	if (enemy_bullet_index_end >= UNIT_ENEMY_BULLET_LIST_SIZE) {
		enemy_bullet_index_end = 0;
	}

	return ret;
}

void unit_manager_clear_all_enemy_bullet()
{
	for (int i = 0; i < UNIT_ENEMY_BULLET_LIST_SIZE; i++) {
		if (enemy_bullet[i].type != UNIT_TYPE_ENEMY_BULLET) continue;
		unit_manager_clear_enemy_bullet(&enemy_bullet[i]);
	}
	enemy_bullet_index_end = 0;
}

void unit_manager_clear_enemy_bullet(unit_enemy_bullet_data_t* bullet)
{
	bullet->type = UNIT_TYPE_NONE;
	bullet->base = NULL;
	collision_manager_delete_shape(bullet->col_shape);
	bullet->col_shape = NULL;
	animation_manager_delete_anim_stat_data(bullet->anim);
	bullet->anim = NULL;

	if (bullet->ai_bullet) {
		ai_manager_delete_ai_data(bullet->ai_bullet);
		bullet->ai_bullet = NULL;
	}
}

void unit_manager_enemy_bullet_update()
{
	for (int i = 0; i < UNIT_ENEMY_BULLET_LIST_SIZE; i++) {
		if (enemy_bullet[i].type != UNIT_TYPE_ENEMY_BULLET) continue;

		if (enemy_bullet[i].col_shape->stat == COLLISION_STAT_ENABLE) {
#ifdef _COLLISION_ENABLE_BOX_2D_
			enemy_bullet[i].col_shape->x = (int)MET2PIX(enemy_bullet[i].col_shape->b2body->GetPosition().x);
			enemy_bullet[i].col_shape->y = (int)MET2PIX(enemy_bullet[i].col_shape->b2body->GetPosition().y);
			enemy_bullet[i].col_shape->vec_x = enemy_bullet[i].col_shape->b2body->GetLinearVelocity().x;  // box2d METER value
			enemy_bullet[i].col_shape->vec_y = enemy_bullet[i].col_shape->b2body->GetLinearVelocity().y;  // box2d METER value
#endif

			// bullet life update
			if (enemy_bullet[i].bullet_life_timer > 0) {
				enemy_bullet[i].bullet_life_timer -= g_delta_time;

				// ai update
				ai_manager_bullet_update(enemy_bullet[i].ai_bullet);
			}
			else {
				unit_manager_enemy_bullet_set_anim_stat(i, ANIM_STAT_FLAG_DIE);
			}
		}

		// anim update
		int stat = unit_manager_unit_get_anim_stat((unit_data_t*)&enemy_bullet[i]);
		if ((stat != -1) && (enemy_bullet[i].anim->anim_stat_base_list[stat]->type == ANIM_TYPE_STATIC)) {
			if (enemy_bullet[i].anim->stat == ANIM_STAT_FLAG_DIE) {
				unit_manager_clear_enemy_bullet(&enemy_bullet[i]);
				continue;
			}
		}

		if ((stat != -1) && (enemy_bullet[i].anim->anim_stat_base_list[stat]->type == ANIM_TYPE_DYNAMIC)) {
			// set current_time
			enemy_bullet[i].anim->anim_stat_list[stat]->current_time += g_delta_time;

			int new_time = enemy_bullet[i].anim->anim_stat_list[stat]->current_time;
			int total_time = enemy_bullet[i].anim->anim_stat_base_list[stat]->total_time;
			if (new_time > total_time) {
				new_time = new_time % total_time;
				enemy_bullet[i].anim->anim_stat_list[stat]->current_time = new_time;

				if (enemy_bullet[i].anim->stat == ANIM_STAT_FLAG_DIE) {
					unit_manager_clear_enemy_bullet(&enemy_bullet[i]);
					continue;
				}
			}

			// set current_frame
			int sum_frame_time = 0;
			int frame_size = enemy_bullet[i].anim->anim_stat_base_list[stat]->frame_size;
			for (int fi = 0; fi < frame_size; fi++) {
				sum_frame_time += enemy_bullet[i].anim->anim_stat_base_list[stat]->frame_list[fi]->frame_time;
				if (new_time < sum_frame_time) {
					if (enemy_bullet[i].anim->anim_stat_list[stat]->current_frame != fi) {
						// send command
						if (enemy_bullet[i].anim->anim_stat_base_list[stat]->frame_list[fi]->command == ANIM_FRAME_COMMAND_ON) {
							game_event_unit_t* msg_param = (game_event_unit_t*)game_event_get_new_param();
							msg_param->obj1 = (unit_data_t*)(&enemy_bullet[i]);
							game_event_t msg = { (EVENT_MSG_UNIT_ENEMY_BULLET | (0x00000001 << stat)), (void*)msg_param };
							game_event_push(&msg);
							enemy_bullet[i].anim->anim_stat_list[stat]->command_frame = fi;
						}
						// set frame
						enemy_bullet[i].anim->anim_stat_list[stat]->current_frame = fi;
					}
					break;
				}
			}
		}
	}
}

void unit_manager_enemy_bullet_display(int layer)
{
	for (int i = 0; i < UNIT_ENEMY_BULLET_LIST_SIZE; i++) {
		if (enemy_bullet[i].type != UNIT_TYPE_ENEMY_BULLET) continue;

		unit_display((unit_data_t*)&enemy_bullet[i], layer);
	}
}
