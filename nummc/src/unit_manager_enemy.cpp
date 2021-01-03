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
#include "quest_log_manager.h"

#define ENEMY_BASE_LIST_SIZE (UNIT_ENEMY_LIST_SIZE)
static unit_enemy_data_t enemy_base[ENEMY_BASE_LIST_SIZE];
static unit_enemy_data_t enemy[UNIT_ENEMY_LIST_SIZE];
static int enemy_base_index_end;
static int enemy_index_end;
static int enemy_count;

static unit_effect_stat_data_t enemy_effect[UNIT_ENEMY_LIST_SIZE][UNIT_EFFECT_ID_E_END];
const unit_effect_stat_data_t enemy_effect_default[UNIT_EFFECT_ID_E_END] = {
	// id, timer,  counter,  delta_time,  damage
	{   0, 0,      0,        0,           0       },
	{   0, 4000,   1,        2000,        5       }, // FIRE_UP
	{   0, 1000,   0,        0,           0       }, // FREEZE_UP
};

// rank tables
#define UNIT_ENEMY_BULLET_CURVING_RANK_MIN    3
#define UNIT_ENEMY_BULLET_CURVING_RANK_MAX   10
#define UNIT_ENEMY_BULLET_CURVING_RANK_SIZE  11
static float enemy_bullet_curving_rank[UNIT_ENEMY_BULLET_CURVING_RANK_SIZE] = {
	0.0f,	// 0
	0.0f,
	0.0f,
	0.0f,
	0.02f,
	0.04f,	// 5
	0.08f,
	0.10f,
	0.14f,
	0.18f,
	0.20f,	// 10
};

// rank tables
#define UNIT_ENEMY_SPEED_RANK_MIN    1
#define UNIT_ENEMY_SPEED_RANK_MAX   10
#define UNIT_ENEMY_SPEED_RANK_SIZE  11
static float enemy_speed_rank[UNIT_ENEMY_SPEED_RANK_SIZE] = {
	0.0f,	// 0
	0.2f,
	0.4f,
	0.6f,
	0.8f,
	1.0f,	// 5
	1.2f,
	1.4f,
	1.8f,
	2.4f,
	3.0f,	// 10
};

#define UNIT_ENEMY_STRENGTH_RANK_MIN    4
#define UNIT_ENEMY_STRENGTH_RANK_MAX    8
#define UNIT_ENEMY_STRENGTH_RANK_SIZE  11
static int enemy_strength_rank[UNIT_ENEMY_STRENGTH_RANK_SIZE] = {
	0,	// 0
	5,
	5,
	5,
	5,
	5,	// 5
	10,
	15,
	20,
	20,
	20,	// 10
};

static void load_unit_enemy(std::string& line, unit_enemy_data_t* enemy_data);

//
// enemy
//
int unit_manager_init_enemy()
{
	memset(enemy_base, 0, sizeof(enemy_base));
	memset(enemy, 0, sizeof(enemy));
	enemy_base_index_end = 0;
	enemy_index_end = 0;
	enemy_count = 0;

	return 0;
}

void unit_manager_unload_enemy()
{
	for (int i = 0; i < ENEMY_BASE_LIST_SIZE; i++) {
		if (enemy_base[i].obj) {
			delete[] enemy_base[i].obj;
			enemy_base[i].obj = NULL;
		}
		if (enemy_base[i].next_level) {
			delete[] enemy_base[i].next_level;
			enemy_base[i].next_level = NULL;
		}

		for (int bi = 0; bi < UNIT_ENEMY_BULLET_NUM; bi++) {
			if (enemy_base[i].bullet[bi]) {
				if (enemy_base[i].bullet[bi]->obj) {
					delete[] enemy_base[i].bullet[bi]->obj; // g_enemy_bullet_path
					enemy_base[i].bullet[bi]->obj = NULL;
				}
			}
		}
	}
}

int unit_manager_search_enemy(std::string& path)
{
	int ei = 0;
	bool enemy_found = false;
	while (enemy_base[ei].type == UNIT_TYPE_ENEMY) {
		std::string regist_path = (char*)enemy_base[ei].obj;
		if (regist_path == path) {
			enemy_found = true;
			break;
		}
		ei++;
	}
	if (enemy_found) return ei; // regist already
	else return (-1);           // not found
}

void unit_manager_enemy_set_anim_stat(int unit_id, int stat)
{
	unit_manager_unit_set_anim_stat((unit_data_t*)&enemy[unit_id], stat);

	// DIE
	if ((enemy[unit_id].anim->stat == ANIM_STAT_FLAG_DIE) && enemy[unit_id].col_shape->b2body) {
		g_stage_world->DestroyBody(enemy[unit_id].col_shape->b2body);
		enemy[unit_id].col_shape->b2body = NULL;
	}
}

void unit_manager_enemy_set_effect_stat(int unit_id, int stat, bool off_on)
{
	if (enemy[unit_id].resistance_stat != UNIT_EFFECT_FLAG_E_NONE) {
		stat &= (~enemy[unit_id].resistance_stat);
	}
	if (stat == 0) return;

	if (enemy[unit_id].effect_stat & stat) { // on
		if (off_on == false) {
			enemy[unit_id].effect_stat &= (~stat);

			int i = 0; int flg = 0x00000001;
			while (stat != flg) { i++; flg <<= 1; }

			if ((stat & UNIT_EFFECT_FLAG_E_FREEZE_UP) && (enemy[unit_id].col_shape->joint_type == COLLISION_JOINT_TYPE_PIN_ROUND)) {
				// reset moter speed
				collision_manager_set_moter_speed(enemy[unit_id].col_shape, ((float)enemy[unit_id].col_shape->joint_val1 / 1000.0f)* b2_pi);
			}

			unit_manager_effect_set_anim_stat(enemy[unit_id].effect_param[i].id, ANIM_STAT_FLAG_HIDE);
			enemy[unit_id].effect_param[i].counter = 0;
		}
	}
	else { // off
		if (off_on == true) {
			enemy[unit_id].effect_stat |= stat;

			int i = 0; int flg = 0x00000001;
			while (stat != flg) { i++; flg <<= 1; }

			// joint
			float b2_effect_x = enemy[unit_id].col_shape->b2body->GetPosition().x;
			float b2_effect_y = enemy[unit_id].col_shape->b2body->GetPosition().y;
			if (enemy[unit_id].col_shape->joint_type == COLLISION_JOINT_TYPE_PIN_ROUND) {
				float disp_angle = enemy[unit_id].col_shape->b2body->GetAngle(); // radian
				float sin_val = game_utils_sin(disp_angle);
				float cos_val = game_utils_cos(disp_angle);
				float pin_offset_x = cos_val * enemy[unit_id].col_shape->joint_x - sin_val * enemy[unit_id].col_shape->joint_y;
				float pin_offset_y = sin_val * enemy[unit_id].col_shape->joint_x + cos_val * enemy[unit_id].col_shape->joint_y;

				b2_effect_x += PIX2MET(pin_offset_x - enemy[unit_id].col_shape->joint_x);
				b2_effect_y += PIX2MET(pin_offset_y - enemy[unit_id].col_shape->joint_y);

				// set moter speed (1/3)
				if (stat & UNIT_EFFECT_FLAG_E_FREEZE_UP) {
					collision_manager_set_moter_speed(enemy[unit_id].col_shape, ((float)enemy[unit_id].col_shape->joint_val1 / (3 * 1000.0f)) * b2_pi);
				}
			}

			unit_manager_effect_set_b2position(enemy[unit_id].effect_param[i].id, b2_effect_x, b2_effect_y);
			unit_manager_effect_set_anim_stat(enemy[unit_id].effect_param[i].id, ANIM_STAT_FLAG_IDLE);
			enemy[unit_id].effect_param[i].timer = enemy_effect_default[i].timer;
			enemy[unit_id].effect_param[i].counter = enemy_effect_default[i].counter;
		}
	}
}

void unit_manager_enemy_get_face_velocity(unit_enemy_data_t* enemy_data, float* vec_x, float* vec_y, int face, float abs_velocity, int bullet_track_type, int bullet_num)
{
	unit_manager_get_face_velocity(vec_x, vec_y, face, abs_velocity, bullet_track_type, bullet_num);

	// rotate unit
	if ((enemy_data->col_shape->joint_type == COLLISION_JOINT_TYPE_PIN_ROUND) && (enemy_data->col_shape->b2body)) {
		float disp_angle = enemy_data->col_shape->b2body->GetAngle(); // radian
		float sin_val = game_utils_sin(disp_angle);
		float cos_val = game_utils_cos(disp_angle);

		for (int i = 0; i < bullet_num; i++) {
			float rotate_x = cos_val * (*(vec_x + i)) - sin_val * (*(vec_y + i));
			float rotate_y = sin_val * (*(vec_x + i)) + cos_val * (*(vec_y + i));
			*(vec_x + i) = rotate_x;
			*(vec_y + i) = rotate_y;
		}
	}

	// curving velocity
	float curving_coef = unit_manager_enemy_get_bullet_curving(enemy_data->base->id);
	if ((face == UNIT_FACE_N) || (face == UNIT_FACE_S)) {
		for (int i = 0; i < bullet_num; i++) {
			*(vec_x + i) = *(vec_x + i) + curving_coef * enemy_data->col_shape->vec_x;
		}
	}
	else if ((face == UNIT_FACE_E) || (face == UNIT_FACE_W)) {
		for (int i = 0; i < bullet_num; i++) {
			*(vec_y + i) = *(vec_y + i) + curving_coef * enemy_data->col_shape->vec_y;
		}
	}

	// freeze bullet speed (= 1/2)
	if (enemy_data->effect_stat & UNIT_EFFECT_FLAG_E_FREEZE_UP) {
		for (int i = 0; i < bullet_num; i++) {
			*(vec_x + i) = *(vec_x + i) * 0.5f;
			*(vec_y + i) = *(vec_y + i) * 0.5f;
		}
	}
}

int unit_manager_enemy_get_bullet_strength(int base_id)
{
	int rank = enemy_base[base_id].strength;
	return enemy_strength_rank[rank];
}

int unit_manager_enemy_get_bullet_life_timer(int base_id)
{
	return enemy_base[base_id].bullet_life_timer;
}

float unit_manager_enemy_get_bullet_curving(int base_id)
{
	int rank = enemy_base[base_id].bullet_curving;
	return enemy_bullet_curving_rank[rank];
}

int unit_manager_load_enemy(std::string path)
{
	if (unit_manager_search_enemy(path) >= 0) return 0; // regist already

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
				enemy_base[enemy_base_index_end].obj = (void*)path_c_str;
				enemy_base[enemy_base_index_end].type = UNIT_TYPE_ENEMY;
				enemy_base[enemy_base_index_end].id = enemy_base_index_end;
				enemy_base[enemy_base_index_end].effect_stat = UNIT_EFFECT_FLAG_E_NONE;
				enemy_base[enemy_base_index_end].effect_param = NULL;
				enemy_base[enemy_base_index_end].drop_item = -1; // disable
				continue;
			}
			if (line == "[/unit]") { read_flg[UNIT_TAG_UNIT] = false; continue; }
			if (line == "[collision]") { read_flg[UNIT_TAG_COLLISION] = true;  continue; }
			if (line == "[/collision]") { read_flg[UNIT_TAG_COLLISION] = false; continue; }
			if (line == "[anim]") {
				read_flg[UNIT_TAG_ANIM] = true;
				enemy_base[enemy_base_index_end].anim = animation_manager_new_anim_data();
				animation_manager_new_anim_stat_base_data(enemy_base[enemy_base_index_end].anim);
				continue;
			}
			if (line == "[/anim]") { read_flg[UNIT_TAG_ANIM] = false; continue; }
			if (line == "[ai]") {
				read_flg[UNIT_TAG_AI] = true;
				enemy_base[enemy_base_index_end].ai = ai_manager_new_ai_base_data();
				enemy_base[enemy_base_index_end].ai->obj = NULL;
				continue;
			}
			if (line == "[/ai]") { read_flg[UNIT_TAG_AI] = false; continue; }
			if (line == "[bullet]") { read_flg[UNIT_TAG_BULLET] = true;  continue; }
			if (line == "[/bullet]") { read_flg[UNIT_TAG_BULLET] = false; continue; }


			if (read_flg[UNIT_TAG_UNIT]) {
				load_unit_enemy(line, &enemy_base[enemy_base_index_end]);
			}
			if (read_flg[UNIT_TAG_COLLISION]) {
				load_collision(line, &enemy_base[enemy_base_index_end].col_shape);
			}
			if (read_flg[UNIT_TAG_ANIM]) {
				load_anim(line, enemy_base[enemy_base_index_end].anim);
			}
			if (read_flg[UNIT_TAG_AI]) {
				load_ai(line, enemy_base[enemy_base_index_end].ai);
			}
			if (read_flg[UNIT_TAG_BULLET]) {
				load_bullet(line, enemy_base[enemy_base_index_end].bullet);
			}
		}
		inFile.close();
	}
	else {
		LOG_ERROR("unit_manager_load_enemy %s error\n", path.c_str());
		return 1;
	}

	// load anim files
	if (enemy_base[enemy_base_index_end].anim) {
		for (int i = 0; i < ANIM_STAT_END; i++) {
			char* cstr_anim_path = (char*)enemy_base[enemy_base_index_end].anim->anim_stat_base_list[i]->obj;
			if (cstr_anim_path) {
				std::string anim_path = cstr_anim_path;
				animation_manager_load_file(anim_path, enemy_base[enemy_base_index_end].anim, i);
			}
		}
	}

	// load ai_bullet files
	for (int i = 0; i < UNIT_ENEMY_BULLET_NUM; i++) {
		if (enemy_base[enemy_base_index_end].bullet[i]) {
			char* cstr_bullet_path = (char*)enemy_base[enemy_base_index_end].bullet[i]->obj;
			if (cstr_bullet_path) {
				std::string bullet_path = cstr_bullet_path;
				ai_manager_load_bullet_file(bullet_path, (ai_bullet_t*)enemy_base[enemy_base_index_end].bullet[i]);
			}
		}
	}

	enemy_base_index_end++;
	return 0;
}

static void load_unit_enemy(std::string& line, unit_enemy_data_t* enemy_data)
{
	std::string key, value;
	game_utils_split_key_value(line, key, value);

	if (value == "") value = "0";
	if (key == "hp") enemy_data->hp = atoi(value.c_str());
	if (key == "bullet_life_timer") enemy_data->bullet_life_timer = atoi(value.c_str());
	if (key == "bullet_curving") enemy_data->bullet_curving = atoi(value.c_str());
	if (key == "speed") enemy_data->speed = atoi(value.c_str());
	if (key == "strength") enemy_data->strength = atoi(value.c_str());
	if (key == "exp") enemy_data->exp = atoi(value.c_str());
	if (key == "hp_max") enemy_data->hp_max = atoi(value.c_str());
	if (key == "exp_max") enemy_data->exp_max = atoi(value.c_str());
	if (key == "level") enemy_data->level = atoi(value.c_str());
	if (key == "next_level") {
		char* next_level_c_str = new char[value.size() + 1];
		memcpy(next_level_c_str, value.c_str(), value.size());
		next_level_c_str[value.size()] = '\0';
		enemy_data->next_level = next_level_c_str;
	}

	if (key == "drop_item") {
		enemy_data->drop_item = unit_manager_search_items(value);
	}
	if (key == "resistance") {
		std::vector<std::string> stat_list;
		game_utils_split_conmma(value, stat_list);
		int resistance_val = UNIT_EFFECT_FLAG_E_NONE;
		for (int i = 0; i < stat_list.size(); i++) {
			if (stat_list[i] == "FIRE_UP") {
				resistance_val |= UNIT_EFFECT_FLAG_E_FIRE_UP;
			}
			else if (stat_list[i] == "FREEZE_UP") {
				resistance_val |= UNIT_EFFECT_FLAG_E_FREEZE_UP;
			}
		}
		enemy_data->resistance_stat = resistance_val;
	}
}

int unit_manager_create_enemy(int x, int y, int face, int base_index)
{
	int ret = -1;

	for (int i = enemy_index_end; i < UNIT_ENEMY_LIST_SIZE; i++) {
		if (enemy[i].type != UNIT_TYPE_ENEMY) {
			ret = enemy_index_end = i;
			break;
		}
	}
	if ((ret == -1) && (enemy_index_end > 0)) {
		for (int i = 0; i < enemy_index_end; i++) {
			if (enemy[i].type != UNIT_TYPE_ENEMY) {
				ret = enemy_index_end = i;
				break;
			}
		}
	}
	if (ret == -1) {
		LOG_ERROR("Error: unit_manager_create_enemy() overflow.");
		return ret;
	}

	if (base_index == -1) base_index = enemy_base_index_end - 1;
  
	// set unit data
	memcpy(&enemy[enemy_index_end], &enemy_base[base_index], sizeof(unit_enemy_data_t));
	enemy[enemy_index_end].base = &enemy_base[base_index];
	enemy[enemy_index_end].id = enemy_index_end;
	enemy[enemy_index_end].type = UNIT_TYPE_ENEMY;

	// set effect
	if (unit_manager_load_enemy_effects(enemy_index_end) == 0) {
		enemy[enemy_index_end].effect_stat  = UNIT_EFFECT_FLAG_P_NONE;
		enemy[enemy_index_end].effect_param = &enemy_effect[enemy_index_end][0];
	}

	// collision
	enemy[enemy_index_end].col_shape =
		collision_manager_create_dynamic_shape(enemy_base[base_index].col_shape,
			(void*)&enemy[enemy_index_end], enemy_base[base_index].anim->base_w, enemy_base[base_index].anim->base_h,
			&x, &y, NULL, NULL, &face);

	// anim
	enemy[enemy_index_end].anim = animation_manager_new_anim_data();
	enemy[enemy_index_end].anim->stat = ANIM_STAT_FLAG_IDLE;
	enemy[enemy_index_end].anim->type = enemy[enemy_index_end].base->anim->type;
	enemy[enemy_index_end].anim->obj = enemy[enemy_index_end].base->anim->obj;
	for (int i = 0; i < ANIM_STAT_END; i++) {
		enemy[enemy_index_end].anim->anim_stat_base_list[i] = enemy[enemy_index_end].base->anim->anim_stat_base_list[i];
	}

	// set stat SPAWN
	if (enemy[enemy_index_end].anim->anim_stat_base_list[ANIM_STAT_SPAWN]->obj) {
		unit_manager_enemy_set_anim_stat(enemy_index_end, ANIM_STAT_FLAG_SPAWN);
	}

	// ai
	enemy[enemy_index_end].ai = ai_manager_new_ai_data();
	enemy[enemy_index_end].ai->obj = (void*)&enemy[enemy_index_end];
	ai_manager_copy(enemy[enemy_index_end].ai, enemy[enemy_index_end].base->ai);

	for (int i = 0; i < UNIT_ENEMY_BULLET_NUM; i++) {
		ai_bullet_t* ai_bullet_base_data = (ai_bullet_t*)enemy[enemy_index_end].base->bullet[i];
		if (ai_bullet_base_data != NULL) {
			ai_stat_bullet_t* ai_stat_bullet_data = (ai_stat_bullet_t*)ai_manager_new_ai_data();
			enemy[enemy_index_end].bullet[i] = (ai_data_t*)ai_stat_bullet_data;

			if (ai_stat_bullet_data != NULL) {
				ai_manager_bullet_copy((ai_bullet_t*)ai_stat_bullet_data, ai_bullet_base_data);
			}
			else {
				LOG_ERROR("Error: unit_manager_create_enemy %s\n", (char*)ai_bullet_base_data->obj);
			}
		}
	}

	enemy_count += 1;
	enemy_index_end++;
	if (enemy_index_end >= UNIT_ENEMY_LIST_SIZE) {
		enemy_index_end = 0;
	}

	return ret;
}

void unit_manager_clear_enemy(unit_enemy_data_t* enemy)
{
	enemy->type = UNIT_TYPE_NONE;
	enemy->base = NULL;
	for (int i = UNIT_EFFECT_ID_E_FIRE_UP; i < UNIT_EFFECT_ID_E_END; i++) {
		unit_manager_clear_effect(unit_manager_get_effect(enemy->effect_param[i].id));
	}
	enemy->effect_param = NULL;
	collision_manager_delete_shape(enemy->col_shape);
	enemy->col_shape = NULL;
	animation_manager_delete_anim_stat_data(enemy->anim);
	enemy->anim = NULL;
	ai_manager_delete_ai_data(enemy->ai);
	enemy->ai = NULL;

	for (int i = 0; i < UNIT_ENEMY_BULLET_NUM; i++) {
		if (enemy->bullet[i]) {
			ai_manager_delete_ai_data(enemy->bullet[i]);
			enemy->bullet[i] = NULL;
		}
	}
}

int unit_manager_load_enemy_effects(int unit_id)
{
	std::string effect_path[UNIT_EFFECT_ID_E_END] = {
		"",
		"units/effect/fire_up/fire_up.unit",
		"units/effect/freeze_up/freeze_up.unit",
	};

	for (int i = UNIT_EFFECT_ID_E_FIRE_UP; i < UNIT_EFFECT_ID_E_END; i++) {
		memcpy(&enemy_effect[unit_id][i], &enemy_effect_default[i], sizeof(unit_effect_stat_data_t));
		enemy_effect[unit_id][i].id = unit_manager_create_effect(0, 0, unit_manager_search_effect(effect_path[i]));
		unit_manager_effect_set_anim_stat(enemy_effect[unit_id][i].id, ANIM_STAT_FLAG_HIDE);
	}

	return 0;
}

bool unit_manager_enemy_exist()
{
	return enemy_count > 0;
}

int unit_manager_enemy_get_damage_force(unit_enemy_data_t* enemy_data, int hp)
{
	enemy_data->hp += hp;
	sound_manager_play(resource_manager_getChunkFromPath("sounds/sfx_error1.ogg"), SOUND_MANAGER_CH_SUB1);

	char buff[32] = { '\0' };
	sprintf_s(buff, "enemy %d damaged(force): %d", enemy_data->id, hp);
	quest_log_manager_set_new_message((char*)buff, (int)strlen(buff));

	if (enemy_data->hp <= 0) {
		// give exp
		unit_manager_player_get_exp(enemy_data->exp);

		// die enemy
		unit_manager_enemy_set_anim_stat(enemy_data->id, ANIM_STAT_FLAG_DIE);
		enemy_count -= 1;
		return 1;
	}
	return 0;
}

int unit_manager_enemy_get_damage(unit_enemy_data_t* enemy_data, int hp)
{
	enemy_data->hp += hp;
	sound_manager_play(resource_manager_getChunkFromPath("sounds/sfx_error1.ogg"), SOUND_MANAGER_CH_SUB1);

	char buff[32] = { '\0' };
	sprintf_s(buff, "enemy %d damaged: %d", enemy_data->id, hp);
	quest_log_manager_set_new_message((char*)buff, (int)strlen(buff));

	if (enemy_data->hp <= 0) {
		// give exp
		unit_manager_player_get_exp(enemy_data->exp);

		// die enemy
		unit_manager_enemy_set_anim_stat(enemy_data->id, ANIM_STAT_FLAG_DIE);
		enemy_count -= 1;
		return 1;
	}
	return 0;
}

int unit_manager_enemy_get_damage_with_bullet(unit_enemy_data_t* enemy_data, unit_player_bullet_data_t* player_bullet_data)
{
	int ret = 0;
	if (player_bullet_data->special == UNIT_BULLET_TYPE_NONE) {
		ret = unit_manager_enemy_get_damage(enemy_data, -player_bullet_data->hp);
	}
	else if (player_bullet_data->special == UNIT_BULLET_TYPE_FIRE) {
		ret = unit_manager_enemy_get_damage(enemy_data, -player_bullet_data->hp);
		if (ret == 0) unit_manager_enemy_set_effect_stat(enemy_data->id, UNIT_EFFECT_FLAG_E_FIRE_UP, true);
	}
	else if (player_bullet_data->special == UNIT_BULLET_TYPE_ICE) {
		ret = unit_manager_enemy_get_damage(enemy_data, -player_bullet_data->hp);
		if (ret == 0) unit_manager_enemy_set_effect_stat(enemy_data->id, UNIT_EFFECT_FLAG_E_FREEZE_UP, true);
	}
	return ret;
}

int unit_manager_enemy_attack(unit_enemy_data_t* enemy_data, int stat)
{
	int x[UNIT_BULLET_NUM_MAX], y[UNIT_BULLET_NUM_MAX];
	float vec_x[UNIT_BULLET_NUM_MAX], vec_y[UNIT_BULLET_NUM_MAX];
	int bullet, bullet_track_type, bullet_num, bullet_face;
	float abs_vec = 1.0f;

	ai_bullet_t* ai_bullet = NULL;
	if (stat == ANIM_STAT_FLAG_ATTACK1) {
		ai_bullet = (ai_bullet_t*)enemy_data->bullet[0];
	}
	else if (stat == ANIM_STAT_FLAG_ATTACK2) {
		ai_bullet = (ai_bullet_t*)enemy_data->bullet[1];
	}
	else {
		// do nothing
		return 1;
	}

	// get bullet setting
	bullet = ai_bullet->bullet_path_index;
	bullet_track_type = ai_bullet->bullet_track_type;
	bullet_num = ai_bullet->bullet_num;
	bullet_face = ai_bullet->bullet_face;

	// convert face (face_W -> face_X)
	bullet_face = unit_manager_get_face_relative((unit_data_t*)enemy_data, bullet_face);

	std::string bullet_path = g_enemy_bullet_path[bullet];
	int bullet_base_id = unit_manager_search_enemy_bullet(bullet_path);
	unit_manager_get_bullet_start_pos((unit_data_t*)enemy_data, (unit_data_t*)unit_manager_get_enemy_bullet_base(bullet_base_id), bullet_track_type, bullet_num, bullet_face, x, y);
	unit_manager_enemy_get_face_velocity(enemy_data, vec_x, vec_y, bullet_face, abs_vec, bullet_track_type, bullet_num);

	for (int i = 0; i < bullet_num; i++) {
		int unit_id = unit_manager_create_enemy_bullet(x[i], y[i], vec_x[i], vec_y[i], bullet_face, enemy_data->base->id, bullet_base_id, (ai_data_t*)ai_bullet);
		unit_manager_enemy_bullet_set_anim_stat(unit_id, ANIM_STAT_FLAG_ATTACK);
		unit_manager_enemy_bullet_set_hp(unit_id, unit_manager_enemy_get_bullet_strength(enemy_data->base->id));
		unit_manager_enemy_bullet_set_bullet_life_timer(unit_id, unit_manager_enemy_get_bullet_life_timer(enemy_data->base->id));
	}

	return 0;
}

void unit_manager_enemy_trap(unit_enemy_data_t* enemy_data, unit_trap_data_t* trap_data)
{
	if (trap_data->group == UNIT_TRAP_GROUP_NONE) {
		// do nothing
	}
	else if (trap_data->group == UNIT_TRAP_GROUP_RECOVERY) {
		//
		// work in progress
		//
	}
	else if (trap_data->group == UNIT_TRAP_GROUP_DAMAGE) {
		unit_manager_enemy_get_damage(enemy_data, -trap_data->base->hp);
	}
	else if ((trap_data->group == UNIT_TRAP_GROUP_FIRE) && !(enemy_data->resistance_stat & UNIT_EFFECT_FLAG_E_FIRE_UP)) {
		if (unit_manager_enemy_get_damage(enemy_data, -trap_data->base->hp) == 0) {
			unit_manager_enemy_set_effect_stat(enemy_data->id, UNIT_EFFECT_FLAG_E_FIRE_UP, true);
		}
	}
	else if ((trap_data->group == UNIT_TRAP_GROUP_ICE) && !(enemy_data->resistance_stat & UNIT_EFFECT_FLAG_E_FREEZE_UP)) {
		if (unit_manager_enemy_get_damage(enemy_data, -trap_data->base->hp) == 0) {
			unit_manager_enemy_set_effect_stat(enemy_data->id, UNIT_EFFECT_FLAG_E_FREEZE_UP, true);
		}
	}
}

void unit_manager_enemy_move(unit_enemy_data_t* enemy_data, float vec_x, float vec_y, int speed)
{
	if (speed == -1) {
		speed = enemy_data->speed;
	}

	if (enemy_data->effect_stat & UNIT_EFFECT_FLAG_E_FREEZE_UP) {
		speed = MAX(UNIT_ENEMY_SPEED_RANK_MIN, enemy_data->speed - 3);
	}

	unit_manager_unit_move((unit_data_t*)enemy_data, vec_x, vec_y, enemy_speed_rank[speed]);
}

void unit_manager_enemy_update()
{
	for (int ei = 0; ei < UNIT_ENEMY_LIST_SIZE; ei++) {
		if (enemy[ei].type != UNIT_TYPE_ENEMY) continue;

		// enemy delta time
		int enemy_delta_time = g_delta_time;
		if (enemy[ei].effect_stat & UNIT_EFFECT_FLAG_E_FREEZE_UP) {
			enemy_delta_time >>= 1;  // div 2
		}

		if (enemy[ei].col_shape->stat == COLLISION_STAT_ENABLE) {
#ifdef _COLLISION_ENABLE_BOX_2D_
			enemy[ei].col_shape->x = (int)MET2PIX(enemy[ei].col_shape->b2body->GetPosition().x);
			enemy[ei].col_shape->y = (int)MET2PIX(enemy[ei].col_shape->b2body->GetPosition().y);
			unit_manager_update_unit_friction((unit_data_t*)&enemy[ei]);
#endif

			// update *base* effect position
			float b2_effect_x = enemy[ei].col_shape->b2body->GetPosition().x;
			float b2_effect_y = enemy[ei].col_shape->b2body->GetPosition().y;
			for (int ef_i = UNIT_EFFECT_ID_E_FIRE_UP; ef_i < UNIT_EFFECT_ID_E_END; ef_i++) {
				int effect_flg = 0x00000001 << ef_i;
				if (enemy[ei].effect_stat & effect_flg) {

					// joint
					if (enemy[ei].col_shape->joint_type == COLLISION_JOINT_TYPE_PIN_ROUND) {
						float disp_angle = enemy[ei].col_shape->b2body->GetAngle(); // radian
						float sin_val = game_utils_sin(disp_angle);
						float cos_val = game_utils_cos(disp_angle);
						float pin_offset_x = cos_val * enemy[ei].col_shape->joint_x - sin_val * enemy[ei].col_shape->joint_y;
						float pin_offset_y = sin_val * enemy[ei].col_shape->joint_x + cos_val * enemy[ei].col_shape->joint_y;

						b2_effect_x += PIX2MET(pin_offset_x - enemy[ei].col_shape->joint_x);
						b2_effect_y += PIX2MET(pin_offset_y - enemy[ei].col_shape->joint_y);
					}

					// set position
					unit_manager_effect_set_b2position(enemy[ei].effect_param[ef_i].id, b2_effect_x, b2_effect_y);

					enemy[ei].effect_param[ef_i].timer -= enemy_delta_time;

					if (enemy[ei].effect_param[ef_i].timer < 0) {
						unit_manager_enemy_set_effect_stat(ei, effect_flg, false);
					}

					// timer damage (fire up)
					else if (enemy[ei].effect_param[ef_i].counter > 0) {
						int damage_count = enemy[ei].effect_param[ef_i].counter;
						if (enemy[ei].effect_param[ef_i].timer < enemy[ei].effect_param[ef_i].delta_time * damage_count) {
							// damage 5 hp
							if (unit_manager_enemy_get_damage_force(&enemy[ei], -enemy[ei].effect_param[ef_i].damage) == 0) {
								std::string damage_path = "units/effect/damage/damage.unit";
								int effect_id = unit_manager_create_effect(enemy[ei].col_shape->x, enemy[ei].col_shape->y, unit_manager_search_effect(damage_path));
								unit_manager_effect_set_trace_unit(effect_id, (unit_data_t*)&enemy[ei]);
							}

							enemy[ei].effect_param[ef_i].counter -= 1;
						}
					}
				}
			}
		}

		// anim update
		int stat = unit_manager_unit_get_anim_stat((unit_data_t*)&enemy[ei]);
		if ((stat != -1) && (enemy[ei].anim->anim_stat_base_list[stat]->type == ANIM_TYPE_DYNAMIC)) {
			// set current_time
			enemy[ei].anim->anim_stat_list[stat]->current_time += enemy_delta_time;

			int new_time = enemy[ei].anim->anim_stat_list[stat]->current_time;
			int total_time = enemy[ei].anim->anim_stat_base_list[stat]->total_time;
			if (new_time > total_time) {
				new_time = new_time % total_time;
				enemy[ei].anim->anim_stat_list[stat]->current_time = new_time;

				// end frame event
				if ((enemy[ei].anim->stat == ANIM_STAT_FLAG_ATTACK1) || (enemy[ei].anim->stat == ANIM_STAT_FLAG_ATTACK2)) {
					unit_manager_enemy_set_anim_stat(ei, ANIM_STAT_FLAG_IDLE);
				}
				else if (enemy[ei].anim->stat == ANIM_STAT_FLAG_DIE) {
					unit_manager_enemy_drop_item(&enemy[ei]);
					unit_manager_clear_enemy(&enemy[ei]);
					continue;
				}
				else if (enemy[ei].anim->stat == ANIM_STAT_FLAG_SPAWN) {
					unit_manager_enemy_set_anim_stat(ei, ANIM_STAT_FLAG_IDLE);
					continue;
				}
			}

			// set current_frame
			int sum_frame_time = 0;
			int frame_size = enemy[ei].anim->anim_stat_base_list[stat]->frame_size;
			for (int i = 0; i < frame_size; i++) {
				sum_frame_time += enemy[ei].anim->anim_stat_base_list[stat]->frame_list[i]->frame_time;
				if (new_time < sum_frame_time) {
					if (enemy[ei].anim->anim_stat_list[stat]->current_frame != i) {
						// send command
						if (enemy[ei].anim->anim_stat_base_list[stat]->frame_list[i]->command == ANIM_FRAME_COMMAND_ON) {
							game_event_unit_t* msg_param = new game_event_unit_t;
							msg_param->obj1 = (unit_data_t*)&enemy[ei];
							game_event_t msg = { (EVENT_MSG_UNIT_ENEMY | (0x00000001 << stat)), msg_param };
							game_event_push(&msg);
							enemy[ei].anim->anim_stat_list[stat]->command_frame = i;
						}
						// set frame
						enemy[ei].anim->anim_stat_list[stat]->current_frame = i;
					}
					break;
				}
			}
		}
	}
}

void unit_manager_enemy_ai_update()
{
	for (int i = 0; i < UNIT_ENEMY_LIST_SIZE; i++) {
		if (enemy[i].type != UNIT_TYPE_ENEMY) continue;
		if (enemy[i].col_shape->stat == COLLISION_STAT_DISABLE) continue;

		ai_manager_update(enemy[i].ai);
	}
}

void unit_manager_enemy_display(int layer)
{
	for (int i = 0; i < UNIT_ENEMY_LIST_SIZE; i++) {
		if (enemy[i].type != UNIT_TYPE_ENEMY) continue;

		unit_display((unit_data_t*)&enemy[i], layer);
	}
}
