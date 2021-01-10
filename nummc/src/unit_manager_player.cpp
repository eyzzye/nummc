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
#include "inventory_manager.h"
#include "quest_log_manager.h"

unit_player_data_t g_player;
unit_player_data_t g_player_backup;
static char player_backup_path[MAX_PATH];

#define PLAYER_BASE_LIST_SIZE (16)
static unit_player_data_t player_base[PLAYER_BASE_LIST_SIZE];
static int player_base_index_end;
static int player_stat_timer[UNIT_STAT_END];

// common effect
static unit_effect_stat_data_t player_base_effect[UNIT_EFFECT_ID_P_END];

#define UNIT_PLAYER_BOOST_TIMER    4000
#define UNIT_PLAYER_FIRE_UP_TIMER  4000
#define UNIT_PLAYER_FREEZE_TIMER   1000
#define UNIT_PLAYER_SHIELD_TIMER   4000

#define UNIT_PLAYER_FIRE_UP_COUNTER        1
#define UNIT_PLAYER_FIRE_UP_DELTA_TIME  2000
#define UNIT_PLAYER_FIRE_UP_DAMAGE         5

const unit_effect_stat_data_t player_effect_default[UNIT_EFFECT_ID_P_END] = {
	// id, timer,                     counter,                     delta_time,                     damage
	{   0, 0,                         0,                           0,                              0                         },
	{   0, UNIT_PLAYER_FIRE_UP_TIMER, UNIT_PLAYER_FIRE_UP_COUNTER, UNIT_PLAYER_FIRE_UP_DELTA_TIME, UNIT_PLAYER_FIRE_UP_DAMAGE},
	{   0, UNIT_PLAYER_FREEZE_TIMER,  0,                           0,                              0                         },
	{   0, UNIT_PLAYER_BOOST_TIMER,   0,                           0,                              0                         },
	{   0, UNIT_PLAYER_SHIELD_TIMER,  0,                           0,                              0                         },
};

// default values
#define UNIT_PLAYER_ATTACK_WAIT_TIMER       600
#define UNIT_PLAYER_ATTACK_WAIT_TIMER_MIN   100
#define UNIT_PLAYER_ATTACK_WAIT_TIMER_MAX  1000

#define UNIT_PLAYER_BULLET_LIFE_TIMER_MIN   200
#define UNIT_PLAYER_BULLET_LIFE_TIMER_MAX  1200

// rank tables
#define UNIT_PLAYER_BULLET_CURVING_RANK_MIN    3
#define UNIT_PLAYER_BULLET_CURVING_RANK_MAX   10
#define UNIT_PLAYER_BULLET_CURVING_RANK_SIZE  11
static float player_bullet_curving_rank[UNIT_PLAYER_BULLET_CURVING_RANK_SIZE] = {
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
#define UNIT_PLAYER_SPEED_RANK_MIN    1
#define UNIT_PLAYER_SPEED_RANK_MAX   10
#define UNIT_PLAYER_SPEED_RANK_SIZE  11
static float player_speed_rank[UNIT_PLAYER_SPEED_RANK_SIZE] = {
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

#define UNIT_PLAYER_STRENGTH_RANK_MIN    4
#define UNIT_PLAYER_STRENGTH_RANK_MAX    8
#define UNIT_PLAYER_STRENGTH_RANK_SIZE  11
static int player_strength_rank[UNIT_PLAYER_STRENGTH_RANK_SIZE] = {
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

static void load_unit(std::string& line);

//
// player
//
int unit_manager_init_player()
{
	memset(&player_base, 0, sizeof(player_base));
	memset(&g_player, 0, sizeof(g_player));
	player_base_index_end = 0;

	return 0;
}

void unit_manager_unload_player()
{
	for (int i = 0; i < PLAYER_BASE_LIST_SIZE; i++) {
		if (player_base[i].obj) {
			delete[] player_base[i].obj;
			player_base[i].obj = NULL;
		}
		if (player_base[i].next_level) {
			delete[] player_base[i].next_level;
			player_base[i].next_level = NULL;
		}
	}
}

void unit_manager_backup_player()
{
	memcpy(&g_player_backup, &g_player, sizeof(g_player_backup));
	if (g_player.obj) {
		strcpy_s(player_backup_path, MAX_PATH, (char*)g_player.obj);
		g_player_backup.obj = (void*)player_backup_path;
	}
}

void unit_manager_restore_player()
{
	g_player.hp = g_player_backup.hp;
	g_player.exp = g_player_backup.exp;
	g_player.attack_wait_timer = g_player_backup.attack_wait_timer;
	g_player.bullet_life_timer = g_player_backup.bullet_life_timer;
	g_player.bullet_curving = g_player_backup.bullet_curving;

	g_player.speed = g_player_backup.speed;
	g_player.strength = g_player_backup.strength;
	g_player.weapon = g_player_backup.weapon;
	g_player.armor = g_player_backup.armor;

	g_player.hp_max = g_player_backup.hp_max;
	g_player.exp_max = g_player_backup.exp_max;
	g_player.level = g_player_backup.level;
}

void unit_manager_clear_player_backup()
{
	memset(&g_player_backup, 0, sizeof(g_player_backup));
	g_player_backup.obj = (void*)player_backup_path;
}

int unit_manager_load_player_effects()
{
	std::string effect_path[UNIT_EFFECT_ID_P_END] = {
		"",
		"units/effect/fire_up/fire_up.unit",
		"units/effect/freeze_up/freeze_up.unit",
		"units/effect/boost/boost.unit",
		"units/effect/shield/shield.unit",
	};

	for (int i = UNIT_EFFECT_ID_P_FIRE_UP; i < UNIT_EFFECT_ID_P_END; i++) {
		memcpy(&player_base_effect[i], &player_effect_default[i], sizeof(unit_effect_stat_data_t));
		player_base_effect[i].id = unit_manager_create_effect(0, 0, unit_manager_search_effect(effect_path[i]));
		unit_manager_effect_set_anim_stat(player_base_effect[i].id, ANIM_STAT_FLAG_HIDE);
	}

	return 0;
}

int unit_manager_load_player(std::string path)
{
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
				player_base[player_base_index_end].obj = (void*)path_c_str;
				player_base[player_base_index_end].type = UNIT_TYPE_PLAYER;
				player_base[player_base_index_end].id = player_base_index_end;
				player_base[player_base_index_end].effect_stat = UNIT_EFFECT_FLAG_P_NONE;
				player_base[player_base_index_end].effect_param = player_base_effect;
				continue;
			}
			if (line == "[/unit]") { read_flg[UNIT_TAG_UNIT] = false; continue; }
			if (line == "[collision]") { read_flg[UNIT_TAG_COLLISION] = true;  continue; }
			if (line == "[/collision]") { read_flg[UNIT_TAG_COLLISION] = false; continue; }
			if (line == "[anim]") {
				read_flg[UNIT_TAG_ANIM] = true;
				player_base[player_base_index_end].anim = animation_manager_new_anim_data();
				animation_manager_new_anim_stat_base_data(player_base[player_base_index_end].anim);
				continue;
			}
			if (line == "[/anim]") { read_flg[UNIT_TAG_ANIM] = false; continue; }

			if (read_flg[UNIT_TAG_UNIT]) {
				load_unit(line);
			}
			if (read_flg[UNIT_TAG_COLLISION]) {
				load_collision(line, &player_base[player_base_index_end].col_shape);
			}
			if (read_flg[UNIT_TAG_ANIM]) {
				load_anim(line, player_base[player_base_index_end].anim);
			}
		}
		inFile.close();
	}
	else {
		LOG_ERROR("unit_manager_load_player %s error\n", path.c_str());
		return 1;
	}

	// load anim files
	if (player_base[player_base_index_end].anim) {
		for (int i = 0; i < ANIM_STAT_END; i++) {
			char* cstr_anim_path = (char*)player_base[player_base_index_end].anim->anim_stat_base_list[i]->obj;
			if (cstr_anim_path) {
				std::string anim_path = cstr_anim_path;
				animation_manager_load_file(anim_path, player_base[player_base_index_end].anim, i);
			}
		}
	}

	player_base_index_end++;
	return 0;
}

static void load_unit(std::string& line)
{
	std::string key, value;
	game_utils_split_key_value(line, key, value);

	if (value == "") value = "0";
	if (key == "hp") player_base[player_base_index_end].hp = atoi(value.c_str());
	if (key == "attack_wait_timer") player_base[player_base_index_end].attack_wait_timer = atoi(value.c_str());
	if (key == "bullet_life_timer") player_base[player_base_index_end].bullet_life_timer = atoi(value.c_str());
	if (key == "bullet_curving") player_base[player_base_index_end].bullet_curving = atoi(value.c_str());
	if (key == "speed") player_base[player_base_index_end].speed = atoi(value.c_str());
	if (key == "strength") player_base[player_base_index_end].strength = atoi(value.c_str());
	if (key == "weapon") player_base[player_base_index_end].weapon = atoi(value.c_str());
	if (key == "hp_max") player_base[player_base_index_end].hp_max = atoi(value.c_str());
	if (key == "exp_max") player_base[player_base_index_end].exp_max = atoi(value.c_str());
	if (key == "next_level") {
		char* next_level_c_str = new char[value.size() + 1];
		memcpy(next_level_c_str, value.c_str(), value.size());
		next_level_c_str[value.size()] = '\0';
		player_base[player_base_index_end].next_level = next_level_c_str;
	}

	if (key == "resistance") {
		std::vector<std::string> stat_list;
		game_utils_split_conmma(value, stat_list);
		int resistance_val = UNIT_EFFECT_FLAG_P_NONE;
		for (int i = 0; i < stat_list.size(); i++) {
			if (stat_list[i] == "FIRE_UP") {
				resistance_val |= UNIT_EFFECT_FLAG_P_FIRE_UP;
			}
			else if (stat_list[i] == "FREEZE_UP") {
				resistance_val |= UNIT_EFFECT_FLAG_P_FREEZE_UP;
			}
		}
		player_base[player_base_index_end].resistance_stat = resistance_val;
	}
}

void unit_manager_create_player(int x, int y)
{
	// set unit data
	memcpy(&g_player, &player_base[player_base_index_end - 1], sizeof(unit_player_data_t));
	g_player.base = &player_base[player_base_index_end - 1];
	g_player.id = 0;
	g_player.exp = 0;
	g_player.type = UNIT_TYPE_PLAYER;
	g_player.stat_timer = player_stat_timer;
	for (int i = 0; i < UNIT_STAT_END; i++) player_stat_timer[i] = 0;

	// collision
	g_player.col_shape =
		collision_manager_create_dynamic_shape(player_base[player_base_index_end - 1].col_shape,
			(void*)&g_player, player_base[player_base_index_end - 1].anim->base_w, player_base[player_base_index_end - 1].anim->base_h,
			&x, &y);

	// anim
	g_player.anim = animation_manager_new_anim_data();
	g_player.anim->stat = ANIM_STAT_FLAG_IDLE;
	g_player.anim->type = g_player.base->anim->type;
	g_player.anim->obj = g_player.base->anim->obj;
	for (int i = 0; i < ANIM_STAT_END; i++) {
		g_player.anim->anim_stat_base_list[i] = g_player.base->anim->anim_stat_base_list[i];
	}
}

void unit_manager_player_set_stat(int stat)
{
	if (!(g_player.stat & stat)) {
		g_player.stat |= stat;
	}
}

void unit_manager_player_set_anim_stat(int stat)
{
	unit_manager_unit_set_anim_stat((unit_data_t*)&g_player, stat);

	// DIE
	if ((stat == ANIM_STAT_FLAG_DIE) && g_player.col_shape->b2body) {
		// disable collision body
		g_stage_world->DestroyBody(g_player.col_shape->b2body);
		g_player.col_shape->b2body = NULL;
	}
}

void unit_manager_player_set_effect_stat(int stat, bool off_on)
{
	if (g_player.resistance_stat != UNIT_EFFECT_FLAG_P_NONE) {
		stat &= (~g_player.resistance_stat);
	}
	if (stat == 0) return;

	if (g_player.effect_stat & stat) { // on
		if (off_on == false) {
			g_player.effect_stat &= (~stat);

			int i = 0; int flg = 0x00000001;
			while (stat != flg) { i++; flg <<= 1; }
			unit_manager_effect_set_anim_stat(g_player.base->effect_param[i].id, ANIM_STAT_FLAG_HIDE);
			g_player.effect_param[i].counter = 0;
		}
		else {
			int i = 0; int flg = 0x00000001;
			while (stat != flg) { i++; flg <<= 1; }

			// update timer
			g_player.effect_param[i].timer = player_effect_default[i].timer;
			g_player.effect_param[i].counter = player_effect_default[i].counter;
		}
	}
	else { // off
		if (off_on == true) {
			g_player.effect_stat |= stat;

			int i = 0; int flg = 0x00000001;
			while (stat != flg) { i++; flg <<= 1; }

			unit_manager_effect_set_b2position(g_player.base->effect_param[i].id, PIX2MET(g_player.col_shape->x), PIX2MET(g_player.col_shape->y));
			unit_manager_effect_set_anim_stat(g_player.base->effect_param[i].id, ANIM_STAT_FLAG_IDLE);
			g_player.effect_param[i].timer   = player_effect_default[i].timer;
			g_player.effect_param[i].counter = player_effect_default[i].counter;
		}
	}
}

void unit_manager_player_get_face_velocity(float* vec_x, float* vec_y, int face, float abs_velocity, int bullet_track_type, int bullet_num)
{
	unit_manager_get_face_velocity(vec_x, vec_y, face, abs_velocity, bullet_track_type, bullet_num);

	// curving velocity
	float curving_coef = unit_manager_player_get_bullet_curving();
	if ((face == UNIT_FACE_N) || (face == UNIT_FACE_S)) {
		for (int i = 0; i < bullet_num; i++) {
			*(vec_x + i) = *(vec_x + i) + curving_coef * g_player.col_shape->vec_x;
		}
	}
	else if ((face == UNIT_FACE_E) || (face == UNIT_FACE_W)) {
		for (int i = 0; i < bullet_num; i++) {
			*(vec_y + i) = *(vec_y + i) + curving_coef * g_player.col_shape->vec_y;
		}
	}

	// freeze bullet speed (= 1/2)
	if (g_player.effect_stat & UNIT_EFFECT_FLAG_P_FREEZE_UP) {
		for (int i = 0; i < bullet_num; i++) {
			*(vec_x + i) = *(vec_x + i) * 0.5f;
			*(vec_y + i) = *(vec_y + i) * 0.5f;
		}
	}
}

int unit_manager_player_get_bullet_strength()
{
	int rank;
	if (g_player.effect_stat & UNIT_EFFECT_FLAG_P_BOOST) {
		rank = MIN(UNIT_PLAYER_STRENGTH_RANK_MAX, (g_player.strength + 1));
	}
	else {
		rank = g_player.strength;
	}

	return player_strength_rank[rank];
}

int unit_manager_player_get_bullet_life_timer()
{
	int bullet_life_timer;
	if (g_player.effect_stat & UNIT_EFFECT_FLAG_P_BOOST) {
		bullet_life_timer = MIN(UNIT_PLAYER_BULLET_LIFE_TIMER_MAX, (g_player.bullet_life_timer + 200));
	}
	else {
		bullet_life_timer = g_player.bullet_life_timer;
	}

	return bullet_life_timer;
}

float unit_manager_player_get_bullet_curving()
{
	int rank;
	if (g_player.effect_stat & UNIT_EFFECT_FLAG_P_BOOST) {
		rank = MIN(UNIT_PLAYER_BULLET_CURVING_RANK_MAX, (g_player.bullet_curving + 1));
	}
	else {
		rank = g_player.bullet_curving;
	}

	return player_bullet_curving_rank[rank];
}

int unit_manager_player_get_damage_force(int hp)
{
	g_player.hp += hp;
	sound_manager_play(resource_manager_getChunkFromPath("sounds/sfx_error1.ogg"), SOUND_MANAGER_CH_MAIN2);

	char buff[32] = { '\0' };
	sprintf_s(buff, "player damaged(force): %d", hp);
	quest_log_manager_set_new_message((char*)buff, (int)strlen(buff));

	// set game over
	if (g_player.hp <= 0) {
		stage_manager_set_result(STAGE_RESULT_LOSE);
		g_player.stat = UNIT_STAT_FLAG_NONE;
		unit_manager_player_set_anim_stat(ANIM_STAT_FLAG_DIE);
		return 1;
	}
	return 0;
}

int unit_manager_player_get_damage(int hp)
{
	if ((g_player.stat & UNIT_STAT_FLAG_INVINCIBLE) || (g_player.effect_stat & UNIT_EFFECT_FLAG_P_SHIELD)) return 1;

	//player.stat |= UNIT_PLAYER_STAT_FLAG_DAMAGE;
	g_player.hp += hp;
	sound_manager_play(resource_manager_getChunkFromPath("sounds/sfx_error1.ogg"), SOUND_MANAGER_CH_MAIN2);

	char buff[32] = { '\0' };
	sprintf_s(buff, "player damaged: %d", hp);
	quest_log_manager_set_new_message((char*)buff, (int)strlen(buff));

	// set game over
	if (g_player.hp <= 0) {
		stage_manager_set_result(STAGE_RESULT_LOSE);
		g_player.stat = UNIT_STAT_FLAG_NONE;
		unit_manager_player_set_anim_stat(ANIM_STAT_FLAG_DIE);
		return 1;
	}

	g_player.stat |= UNIT_STAT_FLAG_INVINCIBLE;
	g_player.stat_timer[UNIT_STAT_INVINCIBLE] = UNIT_PLAYER_STAT_INVINCIBLE_TIMER;
	return 0;
}

int unit_manager_player_get_damage_with_bullet(unit_enemy_bullet_data_t* enemy_bullet_data)
{
	int ret = 0;
	if (enemy_bullet_data->special == UNIT_BULLET_TYPE_NONE) {
		ret = unit_manager_player_get_damage(-enemy_bullet_data->hp);
	}
	else if (enemy_bullet_data->special == UNIT_BULLET_TYPE_FIRE) {
		ret = unit_manager_player_get_damage(-enemy_bullet_data->hp);
		if (ret == 0) unit_manager_player_set_effect_stat(UNIT_EFFECT_FLAG_P_FIRE_UP, true);
	}
	else if (enemy_bullet_data->special == UNIT_BULLET_TYPE_ICE) {
		ret = unit_manager_player_get_damage(-enemy_bullet_data->hp);
		if (ret == 0) unit_manager_player_set_effect_stat(UNIT_EFFECT_FLAG_P_FREEZE_UP, true);
	}
	return ret;
}

int unit_manager_player_recovery(int hp)
{
	if (g_player.hp >= g_player.hp_max) {
		return 1;
	}

	g_player.hp += hp;
	if (g_player.hp > g_player.hp_max) {
		g_player.hp = g_player.hp_max;
	}

	char buff[32] = { '\0' };
	sprintf_s(buff, "player recovered: %d", hp);
	quest_log_manager_set_new_message((char*)buff, (int)strlen(buff));

	return 0;
}

int unit_manager_player_set_hp_max(int hp_max)
{
	if (g_player.hp_max >= UNIT_PLAYER_HP_MAX_VAL_MAX) {
		return 1;
	}

	int delta_hp;
	if ((g_player.hp_max + hp_max) > UNIT_PLAYER_HP_MAX_VAL_MAX) {
		delta_hp = UNIT_PLAYER_HP_MAX_VAL_MAX - g_player.hp_max;
	}
	else if ((g_player.hp_max + hp_max) < UNIT_PLAYER_HP_MAX_VAL_MIN) {
		delta_hp = UNIT_PLAYER_HP_MAX_VAL_MIN - g_player.hp_max;
	}
	else {
		delta_hp = hp_max;
	}

	g_player.hp_max += delta_hp;

	char buff[32] = { '\0' };
	sprintf_s(buff, "player health up: %d", delta_hp);
	quest_log_manager_set_new_message((char*)buff, (int)strlen(buff));

	unit_manager_player_recovery(delta_hp);

	return 0;
}

int unit_manager_player_get_exp(int exp)
{
	g_player.exp += exp;

	// charge val
	unit_manager_player_charge_val(exp);

	if (g_player.exp > g_player.exp_max) {
		// level up
	}

	return 0;
}

int unit_manager_player_charge_val(int exp)
{
	return inventory_manager_charge_val(exp);
}

void unit_manager_player_change_bullet_curving(int bullet_curving)
{
	g_player.bullet_curving += bullet_curving;
	g_player.bullet_curving = MAX(UNIT_PLAYER_BULLET_CURVING_RANK_MIN, MIN(UNIT_PLAYER_BULLET_CURVING_RANK_MAX, g_player.bullet_curving));

	char buff[32] = { '\0' };
	sprintf_s(buff, "player curving Lv:%d", g_player.bullet_curving);
	quest_log_manager_set_new_message((char*)buff, (int)strlen(buff));
}

void unit_manager_player_change_speed(int speed)
{
	g_player.speed += speed;
	g_player.speed = MAX(UNIT_PLAYER_SPEED_RANK_MIN, MIN(UNIT_PLAYER_SPEED_RANK_MAX, g_player.speed));

	char buff[32] = { '\0' };
	sprintf_s(buff, "player speed Lv:%d", g_player.speed);
	quest_log_manager_set_new_message((char*)buff, (int)strlen(buff));
}

void unit_manager_player_change_strength(int strength)
{
	g_player.strength += strength;
	g_player.strength = MAX(UNIT_PLAYER_STRENGTH_RANK_MIN, MIN(UNIT_PLAYER_STRENGTH_RANK_MAX, g_player.strength));

	char buff[32] = { '\0' };
	sprintf_s(buff, "player strength Lv:%d", g_player.strength);
	quest_log_manager_set_new_message((char*)buff, (int)strlen(buff));
}

int unit_manager_player_change_bullet(int weapon)
{
	g_player.weapon = weapon;

	char buff[32] = { '\0' };
	sprintf_s(buff, "player weapon: %d", g_player.weapon);
	quest_log_manager_set_new_message((char*)buff, (int)strlen(buff));

	return 0;
}

void unit_manager_player_stock_item(unit_items_data_t* item_data)
{
	inventory_manager_register_item(item_data);
}

void unit_manager_player_open_tbox(unit_items_data_t* item_data)
{
	if (item_data->item_id == UNIT_TBOX_ID_STATIC) {
		// spawn items
		int count = item_data->val1;
		int x[5], y[5];
		unit_manager_get_spawn_items_pos((unit_data_t*)item_data, (unit_data_t*)&g_player, count, x, y);
		for (int i = 2; i < (2 + count); i++) {
			int val = unit_manager_items_get_val(item_data->id, i);
			unit_manager_create_items_by_sid(val, x[i - 2], y[i - 2]);
		}
	}
	else if (item_data->item_id == UNIT_TBOX_ID_RANDOM) {
		bool spawn_flag[5] = { false }; // val2 ... 6
		int count = item_data->val1;
		int gen_count = 0;

		int x[5], y[5];
		unit_manager_get_spawn_items_pos((unit_data_t*)item_data, (unit_data_t*)&g_player, count, x, y);

		bool spawn_all = false;
		for (int spawn_loop = 0; spawn_loop < (count * 2); spawn_loop++) {
			if (gen_count >= count) {
				spawn_all = true;
				break;
			}

			int rand_index = game_utils_random_gen(6, 2); // val6 ... val2
			if (spawn_flag[rand_index - 2]) {
				continue; // retry
			}
			else {
				spawn_flag[rand_index - 2] = true;
			}

			int val = unit_manager_items_get_val(item_data->id, rand_index);
			if (unit_manager_create_items_by_sid(val, x[gen_count], y[gen_count]) != -1) {
				gen_count++;
			}
		}

		// spawn in ascending order
		if (!spawn_all) {
			for (int i = 2; i <= 6; i++) {
				if (gen_count >= count) break;

				if (!spawn_flag[i - 2]) {
					spawn_flag[i - 2] = true;
					int val = unit_manager_items_get_val(item_data->id, i);
					if (unit_manager_create_items_by_sid(val, x[gen_count], y[gen_count]) != -1) {
						gen_count++;
					}
				}
			}
		}
	}
}

int unit_manager_player_get_special_item(int item_id)
{
	if (item_id == UNIT_SPECIAL_ID_NONE) {
		// do nothing
	}
	else if (item_id == UNIT_SPECIAL_ID_BOOSTER) {
		if (g_player.effect_stat & UNIT_EFFECT_FLAG_P_BOOST) {
			// already boost_on
			return 1;
		}
		else {
			//player_boost_on = true;
			unit_manager_player_set_effect_stat(UNIT_EFFECT_FLAG_P_BOOST, true);

			char buff[32] = { '\0' };
			sprintf_s(buff, "player boost");
			quest_log_manager_set_new_message((char*)buff, (int)strlen(buff));

			std::string star_path = "units/effect/star/star.unit";
			int effect_id = unit_manager_create_effect(g_player.col_shape->x, g_player.col_shape->y, unit_manager_search_effect(star_path));
			unit_manager_effect_set_trace_unit(effect_id, (unit_data_t*)&g_player);
		}
	}
	else if (item_id == UNIT_SPECIAL_ID_BULLET_CURVING_UP) {
		if (g_player.bullet_curving >= UNIT_PLAYER_BULLET_CURVING_RANK_MAX) {
			return 1;
		}
		unit_manager_player_change_bullet_curving(1);

		char buff[32] = { '\0' };
		sprintf_s(buff, "player bullet_curving %d", g_player.bullet_curving);
		quest_log_manager_set_new_message((char*)buff, (int)strlen(buff));
	}
	else if (item_id == UNIT_SPECIAL_ID_BULLET_RANGE_UP) {
		if (g_player.bullet_life_timer >= UNIT_PLAYER_BULLET_LIFE_TIMER_MAX) {
			return 1;
		}
		else {
			g_player.bullet_life_timer += 200;

			char buff[32] = { '\0' };
			sprintf_s(buff, "player bullet_range %d", g_player.bullet_life_timer);
			quest_log_manager_set_new_message((char*)buff, (int)strlen(buff));
		}
	}
	else if (item_id == UNIT_SPECIAL_ID_BULLET_RATE_UP) {
		if (g_player.attack_wait_timer <= UNIT_PLAYER_ATTACK_WAIT_TIMER_MIN) {
			return 1;
		}
		else {
			g_player.attack_wait_timer -= 100;

			char buff[32] = { '\0' };
			sprintf_s(buff, "player bullet_rate %d", (UNIT_PLAYER_ATTACK_WAIT_TIMER_MAX + UNIT_PLAYER_ATTACK_WAIT_TIMER_MIN) / 100 - (g_player.attack_wait_timer / 100));
			quest_log_manager_set_new_message((char*)buff, (int)strlen(buff));
		}
	}
	else if (item_id == UNIT_SPECIAL_ID_SHIELD) {
		if (g_player.effect_stat & UNIT_EFFECT_FLAG_P_SHIELD) {
			// already shield_on
			return 1;
		}
		else {
			//player_shield_on = true;
			unit_manager_player_set_effect_stat(UNIT_EFFECT_FLAG_P_SHIELD, true);

			char buff[32] = { '\0' };
			sprintf_s(buff, "player shield");
			quest_log_manager_set_new_message((char*)buff, (int)strlen(buff));

			std::string star_path = "units/effect/star/star.unit";
			int effect_id = unit_manager_create_effect(g_player.col_shape->x, g_player.col_shape->y, unit_manager_search_effect(star_path));
			unit_manager_effect_set_trace_unit(effect_id, (unit_data_t*)&g_player);
		}
	}
	else if (item_id == UNIT_SPECIAL_ID_SPEED_UP) {
		if (g_player.speed >= UNIT_PLAYER_SPEED_RANK_MAX) {
			return 1;
		}
		unit_manager_player_change_speed(1);
	}
	else if (item_id == UNIT_SPECIAL_ID_STRENGTH_UP) {
		if (g_player.strength >= UNIT_PLAYER_STRENGTH_RANK_MAX) {
			return 1;
		}
		unit_manager_player_change_strength(1);
	}
	else if (item_id == UNIT_SPECIAL_ID_HEART) {
		std::string heart_path = "units/items/recovery/heart/heart.unit";
		unit_items_data_t* item_data = unit_manager_get_items_base(unit_manager_search_items(heart_path));
		if (unit_manager_player_recovery(item_data->hp) != 0) {
			return 1;
		}
	}

	return 0;
}

void unit_manager_player_get_item(unit_items_data_t* item_data)
{
	if (item_data->group == UNIT_ITEM_GROUP_NONE) {
		// do nothing
	}
	else if (item_data->group == UNIT_ITEM_GROUP_BOM) {
		// do nothing
	}
	else if (item_data->group == UNIT_ITEM_GROUP_DAMAGE) {
		if (unit_manager_player_get_damage(-item_data->base->hp) == 0) {
			std::string damage_path = "units/effect/damage/damage.unit";
			int effect_id = unit_manager_create_effect(g_player.col_shape->x, g_player.col_shape->y, unit_manager_search_effect(damage_path));
			unit_manager_effect_set_trace_unit(effect_id, (unit_data_t*)&g_player);
		}
		unit_manager_items_set_anim_stat(item_data->id, ANIM_STAT_FLAG_DIE);
	}
	else if (item_data->group == UNIT_ITEM_GROUP_RECOVERY) {
		if (item_data->item_id == UNIT_RECOVERY_ID_HEART) {
			if (unit_manager_player_recovery(item_data->hp) == 0) {
				unit_manager_items_set_anim_stat(item_data->id, ANIM_STAT_FLAG_DIE);
			}
		}
		else if (item_data->item_id == UNIT_RECOVERY_ID_HEART_UP) {
			if (unit_manager_player_set_hp_max(item_data->hp) == 0) {
				unit_manager_items_set_anim_stat(item_data->id, ANIM_STAT_FLAG_DIE);
			}
		}
	}
	else if (item_data->group == UNIT_ITEM_GROUP_SPECIAL) {
		if (item_data->item_id == UNIT_SPECIAL_ID_UNKNOWN) {
			int special_item_index = game_utils_random_gen(UNIT_SPECIAL_ID_STRENGTH_UP, UNIT_SPECIAL_ID_BOOSTER);
			std::string random_item_path = unit_manager_get_special_item(special_item_index);

			int x, y;
			unit_manager_get_spawn_items_pos((unit_data_t*)item_data, (unit_data_t*)&g_player, 1, &x, &y);
			unit_manager_create_items(x, y, unit_manager_search_items(random_item_path));
			unit_manager_items_set_anim_stat(item_data->id, ANIM_STAT_FLAG_DIE);
		}
		else if (unit_manager_player_get_special_item(item_data->item_id) == 0) {
			unit_manager_items_set_anim_stat(item_data->id, ANIM_STAT_FLAG_DIE);
		}
	}
	else if (item_data->group == UNIT_ITEM_GROUP_INVINCIBLE) {
		unit_manager_player_set_stat(UNIT_STAT_FLAG_INVINCIBLE);
		unit_manager_items_set_anim_stat(item_data->id, ANIM_STAT_FLAG_DIE);
	}
	else if (item_data->group == UNIT_ITEM_GROUP_BULLET) {
		unit_manager_player_change_bullet(item_data->val1);
		unit_manager_items_set_anim_stat(item_data->id, ANIM_STAT_FLAG_DIE);
	}
	else if (item_data->group == UNIT_ITEM_GROUP_ARMOR) {
		//
		// work in progress
		//
	}
	else if (item_data->group == UNIT_ITEM_GROUP_STOCK) {
		unit_manager_player_stock_item(item_data);
		unit_manager_items_set_anim_stat(item_data->id, ANIM_STAT_FLAG_DIE);
	}
	else if (item_data->group == UNIT_ITEM_GROUP_TBOX) {
		unit_manager_player_open_tbox(item_data);
		unit_manager_items_set_anim_stat(item_data->id, ANIM_STAT_FLAG_DIE);
	}
}

static void use_bom_item()
{
	int x, y;
	std::string bom_path = "units/items/bom/simple/bom.unit";
	unit_manager_get_spawn_items_pos_under_foot((unit_data_t*)&g_player, 1, &x, &y);
	int id = unit_manager_create_items(x, y, unit_manager_search_items(bom_path));
	unit_manager_items_set_anim_stat(id, ANIM_STAT_FLAG_ATTACK);

	char buff[32] = { '\0' };
	sprintf_s(buff, "player drop bom");
	quest_log_manager_set_new_message((char*)buff, (int)strlen(buff));
}

void unit_manager_player_use_weapon_item()
{
	int item_use_id = inventory_manager_get_weapon_item_ref_id();
	if (item_use_id >= 0) {
		int weapon_count = inventory_manager_get_weapon_count();
		if (weapon_count > 0) {
			if (item_use_id == UNIT_WEAPON_ID_BOM) {
				use_bom_item();
			}

			if (inventory_manager_set_weapon_count(-1) == 0) {
				sound_manager_play(resource_manager_getChunkFromPath("sounds/sfx_warning1.ogg"));
			}
		}
	}
}

void unit_manager_player_use_charge_item()
{
	int item_use_id = inventory_manager_get_charge_item_ref_id();
	if (item_use_id >= 0) {
		bool use_item = false;
		if (inventory_manager_get_charge_val() >= INVENTORY_CHARGE_VAL_MAX) {
			if (item_use_id == UNIT_CHARGE_ID_NONE) {
				// do nothing
			}
			else if (item_use_id == UNIT_CHARGE_ID_BOOSTER) {
				if (unit_manager_player_get_special_item(UNIT_SPECIAL_ID_BOOSTER) == 0) {
					use_item = true;
				}
			}
			else if (item_use_id == UNIT_CHARGE_ID_SHIELD) {
				if (unit_manager_player_get_special_item(UNIT_SPECIAL_ID_SHIELD) == 0) {
					use_item = true;
				}
			}
			else if (item_use_id == UNIT_CHARGE_ID_HEART) {
				if (unit_manager_player_get_special_item(UNIT_SPECIAL_ID_HEART) == 0) {
					use_item = true;
				}
			}
			else if (item_use_id == UNIT_CHARGE_ID_BOM) {
				use_bom_item();
				use_item = true;
			}
		}

		if (use_item) {
			inventory_manager_charge_val(- INVENTORY_CHARGE_EXP_UNIT_VAL * INVENTORY_CHARGE_VAL_MAX);
			sound_manager_play(resource_manager_getChunkFromPath("sounds/sfx_warning1.ogg"));
		}
		else {
			sound_manager_play(resource_manager_getChunkFromPath("sounds/sfx_error1.ogg"));
		}
	}
}

void unit_manager_player_use_special_item()
{
	int item_use_id = inventory_manager_get_special_item_ref_id();
	if (item_use_id >= 0) {
		if (unit_manager_player_get_special_item(item_use_id) == 0) {
			inventory_manager_clear_special();
			sound_manager_play(resource_manager_getChunkFromPath("sounds/sfx_warning1.ogg"));
		}
		else {
			sound_manager_play(resource_manager_getChunkFromPath("sounds/sfx_error1.ogg"));
		}
	}
}

void unit_manager_player_trap(unit_trap_data_t* trap_data)
{
	if (trap_data->group == UNIT_TRAP_GROUP_NONE) {
		// do nothing
	}
	else if (trap_data->group == UNIT_TRAP_GROUP_RECOVERY) {
		unit_manager_player_recovery(trap_data->hp);
	}
	else if (trap_data->group == UNIT_TRAP_GROUP_GATE) {
		scene_play_next_stage();
	}
	else if (trap_data->group == UNIT_TRAP_GROUP_DAMAGE) {
		if (unit_manager_player_get_damage(-trap_data->base->hp) == 0) {
			std::string damage_path = "units/effect/damage/damage.unit";
			int effect_id = unit_manager_create_effect(g_player.col_shape->x, g_player.col_shape->y, unit_manager_search_effect(damage_path));
			unit_manager_effect_set_trace_unit(effect_id, (unit_data_t*)&g_player);
		}
	}
	else if ((trap_data->group == UNIT_TRAP_GROUP_FIRE) && !(g_player.resistance_stat & UNIT_EFFECT_FLAG_P_FIRE_UP)) {
		if (unit_manager_player_get_damage(-trap_data->base->hp) == 0) {
			unit_manager_player_set_effect_stat(UNIT_EFFECT_FLAG_P_FIRE_UP, true);
		}
	}
	else if ((trap_data->group == UNIT_TRAP_GROUP_ICE) && !(g_player.resistance_stat & UNIT_EFFECT_FLAG_P_FREEZE_UP)) {
		if (unit_manager_player_get_damage(-trap_data->base->hp) == 0) {
			unit_manager_player_set_effect_stat(UNIT_EFFECT_FLAG_P_FREEZE_UP, true);
		}
	}
}

void unit_manager_player_gameover()
{
	// update animation
	int stat = ANIM_STAT_DIE;
	if ((g_stage_data->stat == STAGE_STAT_ACTIVE) && (g_player.anim->anim_stat_base_list[stat]->type == ANIM_TYPE_DYNAMIC)) {
		// set current_time
		g_player.anim->anim_stat_list[stat]->current_time += g_delta_time;

		int new_time = g_player.anim->anim_stat_list[stat]->current_time;
		int total_time = g_player.anim->anim_stat_base_list[stat]->total_time;
		if (new_time > total_time) {
			new_time = new_time % total_time;
			g_player.anim->anim_stat_list[stat]->current_time = new_time;
			// end frame event
			if (g_player.anim->stat == ANIM_STAT_FLAG_DIE) {
				stage_manager_set_stat(STAGE_STAT_TERMINATE);

				// display gameover dialog
				scene_play_stage_set_lose();
			}
		}

		// set current_frame
		int sum_frame_time = 0;
		int frame_size = g_player.anim->anim_stat_base_list[stat]->frame_size;
		for (int i = 0; i < frame_size; i++) {
			sum_frame_time += g_player.anim->anim_stat_base_list[stat]->frame_list[i]->frame_time;
			if (new_time < sum_frame_time) {
				if (g_player.anim->anim_stat_list[stat]->current_frame != i) {
					// send command
					if (g_player.anim->anim_stat_base_list[stat]->frame_list[i]->command == ANIM_FRAME_COMMAND_ON) {
						game_event_t msg = { (EVENT_MSG_UNIT_PLAYER | (0x00000001 << stat)), NULL };
						game_event_push(&msg);
						g_player.anim->anim_stat_list[stat]->command_frame = i;
					}
					// set frame
					g_player.anim->anim_stat_list[stat]->current_frame = i;
				}
				break;
			}
		}
	}
}

void unit_manager_player_move(float vec_x, float vec_y)
{
	int rank;
	if (g_player.effect_stat & UNIT_EFFECT_FLAG_P_BOOST) {
		rank = MIN(UNIT_PLAYER_SPEED_RANK_MAX, g_player.speed + 1);
	}
	else {
		rank = g_player.speed;
	}

	if (g_player.effect_stat & UNIT_EFFECT_FLAG_P_FREEZE_UP) {
		rank = MAX(UNIT_PLAYER_SPEED_RANK_MIN, g_player.speed - 3);
	}

	unit_manager_unit_move((unit_data_t*)&g_player, vec_x, vec_y, player_speed_rank[rank]);
}

void unit_manager_player_update()
{
	// player delta time
	int player_delta_time = g_delta_time;
	if (g_player.effect_stat & UNIT_EFFECT_FLAG_P_FREEZE_UP) {
		player_delta_time >>= 1;  // div 2
	}

#ifdef _COLLISION_ENABLE_BOX_2D_
	g_player.col_shape->x = (int)MET2PIX(g_player.col_shape->b2body->GetPosition().x);
	g_player.col_shape->y = (int)MET2PIX(g_player.col_shape->b2body->GetPosition().y);
	unit_manager_update_unit_friction((unit_data_t*)&g_player);
#endif

	// update *base* effect position
	for (int ei = UNIT_EFFECT_ID_P_FIRE_UP; ei < UNIT_EFFECT_ID_P_END; ei++) {
		int effect_flg = 0x00000001 << ei;
		if (g_player.effect_stat & effect_flg) {
			unit_manager_effect_set_b2position(g_player.base->effect_param[ei].id, PIX2MET(g_player.col_shape->x), PIX2MET(g_player.col_shape->y));

			g_player.effect_param[ei].timer -= player_delta_time;
			if (g_player.effect_param[ei].timer < 0) {
				unit_manager_player_set_effect_stat(effect_flg, false);
			}

			// timer damage (fire up)
			else if (g_player.effect_param[ei].counter > 0) {
				int damage_count = g_player.effect_param[ei].counter;
				if (g_player.effect_param[ei].timer < g_player.effect_param[ei].delta_time * damage_count) {
					// damage 5 hp
					if (unit_manager_player_get_damage_force(-g_player.effect_param[ei].damage) == 0) {
						std::string damage_path = "units/effect/damage/damage.unit";
						int effect_id = unit_manager_create_effect(g_player.col_shape->x, g_player.col_shape->y, unit_manager_search_effect(damage_path));
						unit_manager_effect_set_trace_unit(effect_id, (unit_data_t*)&g_player);
					}

					g_player.effect_param[ei].counter -= 1;
				}
			}
		}
	}

	// update stat timer
	for (int i = 0; i < UNIT_STAT_END; i++) {
		if (g_player.stat_timer[i] > 0) {
			g_player.stat_timer[i] -= player_delta_time;
			if (g_player.stat_timer[i] <= 0) {
				g_player.stat &= ~(0x00000001 << i);
			}
		}
	}

	// update animation
	int stat = unit_manager_unit_get_anim_stat((unit_data_t*)&g_player);
	if ((stat != -1) && (g_player.anim->anim_stat_base_list[stat]->type == ANIM_TYPE_DYNAMIC)) {
		// set current_time
		g_player.anim->anim_stat_list[stat]->current_time += player_delta_time;

		int new_time = g_player.anim->anim_stat_list[stat]->current_time;
		int total_time = g_player.anim->anim_stat_base_list[stat]->total_time;
		if (new_time > total_time) {
			new_time = new_time % total_time;
			g_player.anim->anim_stat_list[stat]->current_time = new_time;
			//
			// end frame event
			//
			if ((g_player.anim->stat == ANIM_STAT_FLAG_ATTACK1) || (g_player.anim->stat == ANIM_STAT_FLAG_ATTACK2)) {
				unit_manager_player_set_anim_stat(ANIM_STAT_FLAG_IDLE);
			}
		}

		// set current_frame
		int sum_frame_time = 0;
		int frame_size = g_player.anim->anim_stat_base_list[stat]->frame_size;
		for (int i = 0; i < frame_size; i++) {
			sum_frame_time += g_player.anim->anim_stat_base_list[stat]->frame_list[i]->frame_time;
			if (new_time < sum_frame_time) {
				if (g_player.anim->anim_stat_list[stat]->current_frame != i) {
					// send command
					if (g_player.anim->anim_stat_base_list[stat]->frame_list[i]->command == ANIM_FRAME_COMMAND_ON) {
						game_event_t msg = { (EVENT_MSG_UNIT_PLAYER | (0x00000001 << stat)), NULL };
						game_event_push(&msg);
						g_player.anim->anim_stat_list[stat]->command_frame = i;
					}
					// set frame
					g_player.anim->anim_stat_list[stat]->current_frame = i;
				}
				break;
			}
		}
	}
}

void unit_manager_player_display(int layer)
{
	unit_display((unit_data_t*)&g_player, layer);
}
