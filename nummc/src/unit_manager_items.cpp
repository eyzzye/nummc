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

static unit_items_data_t items_base[UNIT_ITEMS_BASE_LIST_SIZE];
static unit_items_data_t items[UNIT_ITEMS_LIST_SIZE];
static int items_base_index_end;
static int items_index_end;

static const char* special_items_path[] = {
	"",
	"units/items/stock/booster/booster.unit",
	"units/items/special/bullet_curving_up/curve.unit",
	"units/items/special/bullet_range_up/range.unit",
	"units/items/special/bullet_rate_up/rate.unit",
	"units/items/stock/shield/shield.unit",
	"units/items/special/speed_up/speed_up.unit",
	"units/items/special/strength_up/strength_up.unit",
};

static void load_items_unit(std::string& line);

// fire bom callback
class BomCallback : public b2QueryCallback
{
public:
	bool ReportFixture(b2Fixture* fixture)
	{
		b2Body* body = fixture->GetBody();
		b2Shape* shape = fixture->GetShape();
		unit_data_t* unit_data = (unit_data_t*)body->GetUserData();

		if (unit_data) {
			std::string effect_path = "units/effect/smoke/smoke.unit";
			if (unit_data->type == UNIT_TYPE_PLAYER) {
				bool overlap = b2TestOverlap(shape, 0, &m_circle, 0, body->GetTransform(), m_transform);
				if (overlap) {
					if (unit_manager_player_get_damage(-m_item_data->hp) == 0) {
						unit_manager_create_effect(unit_data->col_shape->x, unit_data->col_shape->y, unit_manager_search_effect(effect_path));
					}
				}
			}
			else if ((unit_data->type == UNIT_TYPE_ENEMY) && (unit_data->col_shape->stat == COLLISION_STAT_ENABLE)) {
				bool overlap = b2TestOverlap(shape, 0, &m_circle, 0, body->GetTransform(), m_transform);
				if (overlap) {
					unit_manager_enemy_get_damage((unit_enemy_data_t*)unit_data, -m_item_data->hp);
					unit_manager_create_effect(unit_data->col_shape->x, unit_data->col_shape->y, unit_manager_search_effect(effect_path));
				}
			}
			else if ((unit_data->type == UNIT_TYPE_ITEMS) && (unit_data->col_shape->stat == COLLISION_STAT_ENABLE)) {
				bool overlap = false;
				if (((unit_items_data_t*)unit_data)->group == UNIT_ITEM_GROUP_TBOX) {
					overlap = false;
				}
				else {
					overlap = b2TestOverlap(shape, 0, &m_circle, 0, body->GetTransform(), m_transform);
				}
				if (overlap) {
					unit_manager_items_set_anim_stat(unit_data->id, ANIM_STAT_FLAG_DIE);
					unit_manager_create_effect(unit_data->col_shape->x, unit_data->col_shape->y, unit_manager_search_effect(effect_path));
				}
			}
		}

		return true;
	}

	b2CircleShape m_circle;
	b2Transform m_transform;
	unit_items_data_t* m_item_data;
};

//
// items
//
int unit_manager_init_items()
{
	memset(&items_base, 0, sizeof(items_base));
	memset(&items, 0, sizeof(items));
	items_base_index_end = 0;
	items_index_end = 0;

	return 0;
}

void unit_manager_unload_items()
{
	for (int i = 0; i < UNIT_ITEMS_BASE_LIST_SIZE; i++) {
		if (items_base[i].obj) {
			delete[] items_base[i].obj;
			items_base[i].obj = NULL;
		}
	}
}

int unit_manager_search_items(std::string& path)
{
	int ii = 0;
	bool items_found = false;
	while (items_base[ii].type == UNIT_TYPE_ITEMS) {
		std::string regist_path = (char*)items_base[ii].obj;
		if (regist_path == path) {
			items_found = true;
			break;
		}
		ii++;
	}

	if (items_found) return ii;  // regist already
	else return (-1);            // not found
}

unit_items_data_t* unit_manager_get_items_base(int index)
{
	return &items_base[index];
}

unit_items_data_t* unit_manager_get_items(int index)
{
	return &items[index];
}

const char* unit_manager_get_special_item(int index)
{
	return special_items_path[index];
}

void unit_manager_items_set_anim_stat(int unit_id, int stat)
{
	unit_manager_unit_set_anim_stat((unit_data_t*)&items[unit_id], stat);

	// DIE
	if ((items[unit_id].anim->stat == ANIM_STAT_FLAG_DIE) && items[unit_id].col_shape->b2body) {
		if (items[unit_id].group != UNIT_ITEM_GROUP_TBOX) { // exclude TBOX
			g_stage_world->DestroyBody(items[unit_id].col_shape->b2body);
			items[unit_id].col_shape->b2body = NULL;
		}
		
	}
}

int unit_manager_items_get_val(int unit_id, int index)
{
	int ret = -1;
	switch (index) {
	case 1:
		ret = items[unit_id].val1;
		break;
	case 2:
		ret = items[unit_id].val2;
		break;
	case 3:
		ret = items[unit_id].val3;
		break;
	case 4:
		ret = items[unit_id].val4;
		break;
	case 5:
		ret = items[unit_id].val5;
		break;
	case 6:
		ret = items[unit_id].val6;
		break;
	default:
		break;
	}
	return ret;
}

void unit_manager_items_set_val(int unit_id, int val, int index)
{
	switch (index) {
	case 1:
		items[unit_id].val1 = val;
		break;
	case 2:
		items[unit_id].val2 = val;
		break;
	case 3:
		items[unit_id].val3 = val;
		break;
	case 4:
		items[unit_id].val4 = val;
		break;
	case 5:
		items[unit_id].val5 = val;
		break;
	case 6:
		items[unit_id].val6 = val;
		break;
	default:
		break;
	}
}

void unit_manager_items_fire_bom(unit_items_data_t* item_data)
{
	float center_x = PIX2MET(item_data->col_shape->x) + PIX2MET(g_tile_width / 2);
	float center_y = PIX2MET(item_data->col_shape->y) + PIX2MET(g_tile_height / 2);

	// query player/enemy/items -> BomCallback::ReportFixture()
	BomCallback callback;
	callback.m_circle.m_radius = PIX2MET(g_tile_width * 2 + g_tile_width / 2);
	callback.m_circle.m_p.Set(center_x, center_y);
	callback.m_transform.SetIdentity();
	callback.m_item_data = item_data;

	b2AABB aabb;
	callback.m_circle.ComputeAABB(&aabb, callback.m_transform, 0);
	g_stage_world->QueryAABB(&callback, aabb);

	// break block
	int w = g_tile_width * 5, h = g_tile_height * 5;
	int r = ((shape_round_data*)item_data->col_shape)->r;
	map_manager_break_block(item_data->col_shape->x, item_data->col_shape->y, 2/* block */, 2/* block */);
}

int unit_manager_load_items_def(std::string path)
{
	bool read_flg = false;
	std::ifstream inFile(g_base_path + "data/" + path);
	if (inFile.is_open()) {
		std::string line;
		while (std::getline(inFile, line)) {
			if (line == "") continue;
			if (line[0] == '#') continue;

			if (line == "[items]") { read_flg = true;  continue; }
			if (line == "[/items]") { read_flg = false; continue; }

			if (read_flg) {
				unit_manager_load_items(line);
			}
		}
		inFile.close();
	}
	else {
		LOG_ERROR("unit_manager_load_items_def %s error\n", path.c_str());
		return 1;
	}
	return 0;
}

int unit_manager_load_items(std::string path)
{
	if (unit_manager_search_items(path) >= 0) return 0;

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
				items_base[items_base_index_end].obj = (void*)path_c_str;
				items_base[items_base_index_end].type = UNIT_TYPE_ITEMS;
				items_base[items_base_index_end].id = items_base_index_end;
				continue;
			}
			if (line == "[/unit]") { read_flg[UNIT_TAG_UNIT] = false; continue; }
			if (line == "[collision]") { read_flg[UNIT_TAG_COLLISION] = true;  continue; }
			if (line == "[/collision]") { read_flg[UNIT_TAG_COLLISION] = false; continue; }
			if (line == "[anim]") {
				read_flg[UNIT_TAG_ANIM] = true;
				items_base[items_base_index_end].anim = animation_manager_new_anim_data();
				animation_manager_new_anim_stat_base_data(items_base[items_base_index_end].anim);
				continue;
			}
			if (line == "[/anim]") { read_flg[UNIT_TAG_ANIM] = false; continue; }

			if (read_flg[UNIT_TAG_UNIT]) {
				load_items_unit(line);
			}
			if (read_flg[UNIT_TAG_COLLISION]) {
				load_collision(line, &items_base[items_base_index_end].col_shape);
			}
			if (read_flg[UNIT_TAG_ANIM]) {
				load_anim(line, items_base[items_base_index_end].anim);
			}
		}
		inFile.close();
	}
	else {
		LOG_ERROR("unit_manager_load_items %s error\n", path.c_str());
		return 1;
	}

	// load anim files
	if (items_base[items_base_index_end].anim) {
		for (int i = 0; i < ANIM_STAT_END; i++) {
			char* cstr_anim_path = (char*)items_base[items_base_index_end].anim->anim_stat_base_list[i]->obj;
			if (cstr_anim_path) {
				std::string anim_path = cstr_anim_path;
				animation_manager_load_file(anim_path, items_base[items_base_index_end].anim, i);
			}
		}
	}

	items_base_index_end++;
	return 0;
}

static void load_items_unit(std::string& line)
{
	std::string key, value;
	game_utils_split_key_value(line, key, value);

	if (value == "") value = "0";
	if (key == "group") {
		if (value == "NONE") {
			items_base[items_base_index_end].group = UNIT_ITEM_GROUP_NONE;
		}
		else if (value == "DAMAGE") {
			items_base[items_base_index_end].group = UNIT_ITEM_GROUP_DAMAGE;
		}
		else if (value == "RECOVERY") {
			items_base[items_base_index_end].group = UNIT_ITEM_GROUP_RECOVERY;
		}
		else if (value == "SPECIAL") {
			items_base[items_base_index_end].group = UNIT_ITEM_GROUP_SPECIAL;
		}
		else if (value == "INVINCIBLE") {
			items_base[items_base_index_end].group = UNIT_ITEM_GROUP_INVINCIBLE;
		}
		else if (value == "BULLET") {
			items_base[items_base_index_end].group = UNIT_ITEM_GROUP_BULLET;
		}
		else if (value == "ARMOR") {
			items_base[items_base_index_end].group = UNIT_ITEM_GROUP_ARMOR;
		}
		else if (value == "STOCK") {
			items_base[items_base_index_end].group = UNIT_ITEM_GROUP_STOCK;
		}
		else if (value == "TBOX") {
			items_base[items_base_index_end].group = UNIT_ITEM_GROUP_TBOX;
		}
		else if (value == "BOM") {
			items_base[items_base_index_end].group = UNIT_ITEM_GROUP_BOM;
		}
	}
	if (key == "hp") items_base[items_base_index_end].hp = atoi(value.c_str());
	if (key == "time") items_base[items_base_index_end].time = atoi(value.c_str());
	if (key == "item_id") items_base[items_base_index_end].item_id = atoi(value.c_str());
	if (key == "sub_id") items_base[items_base_index_end].sub_id = atoi(value.c_str());

	if (key == "val1") items_base[items_base_index_end].val1 = atoi(value.c_str());
	if (key == "val2") items_base[items_base_index_end].val2 = atoi(value.c_str());
	if (key == "val3") items_base[items_base_index_end].val3 = atoi(value.c_str());
	if (key == "val4") items_base[items_base_index_end].val4 = atoi(value.c_str());
	if (key == "val5") items_base[items_base_index_end].val5 = atoi(value.c_str());
	if (key == "val6") items_base[items_base_index_end].val6 = atoi(value.c_str());
}

int unit_manager_create_items(int x, int y, int base_index)
{
	int ret = -1;

	for (int i = items_index_end; i < UNIT_ITEMS_LIST_SIZE; i++) {
		if (items[i].type != UNIT_TYPE_ITEMS) {
			ret = items_index_end = i;
			break;
		}
	}
	if ((ret == -1) && (items_index_end > 0)) {
		for (int i = 0; i < items_index_end; i++) {
			if (items[i].type != UNIT_TYPE_ITEMS) {
				ret = items_index_end = i;
				break;
			}
		}
	}
	if (ret == -1) {
		LOG_ERROR("Error: unit_manager_create_items() overflow.");
		return ret;
	}

	if (base_index == -1) base_index = items_base_index_end - 1;

	// set unit data
	memcpy(&items[items_index_end], &items_base[base_index], sizeof(unit_items_data_t));
	items[items_index_end].base = &items_base[base_index];
	items[items_index_end].id = items_index_end;
	items[items_index_end].type = UNIT_TYPE_ITEMS;
	items[items_index_end].group = items_base[base_index].group;

	// collision
	if (items_base[base_index].col_shape->type & COLLISION_ID_STATIC_SHAPE) {
		items[items_index_end].col_shape =
			collision_manager_create_static_shape(items_base[base_index].col_shape,
				(void*)&items[items_index_end], items_base[base_index].anim->base_w, items_base[base_index].anim->base_h,
				&x, &y);
	}
	else {
		items[items_index_end].col_shape =
			collision_manager_create_dynamic_shape(items_base[base_index].col_shape,
				(void*)&items[items_index_end], items_base[base_index].anim->base_w, items_base[base_index].anim->base_h,
				&x, &y);
		collision_manager_set_mass(items[items_index_end].col_shape, 0.05f);
	}

	// anim
	items[items_index_end].anim = animation_manager_new_anim_data();
	items[items_index_end].anim->stat = ANIM_STAT_FLAG_IDLE;
	items[items_index_end].anim->type = items[items_index_end].base->anim->type;
	items[items_index_end].anim->obj = items[items_index_end].base->anim->obj;
	for (int i = 0; i < ANIM_STAT_END; i++) {
		items[items_index_end].anim->anim_stat_base_list[i] = items[items_index_end].base->anim->anim_stat_base_list[i];
	}

	// set stat SPAWN
	if (items[items_index_end].anim->anim_stat_base_list[ANIM_STAT_SPAWN]->obj) {
		unit_manager_items_set_anim_stat(items_index_end, ANIM_STAT_FLAG_SPAWN);
	}

	items_index_end++;
	if (items_index_end >= UNIT_ITEMS_LIST_SIZE) {
		items_index_end = 0;
	}

	return ret;
}

int unit_manager_create_items_by_sid(int sid, int x, int y)
{
	int ret = -1;
	int group_id, sub_id, item_id;

	// set ids
	int flag = (0xFFFF0000 & sid), i = 0;
	while (flag != 0x00010000) {
		i++;
		flag >>= 1;
	}
	group_id = i;

	if (group_id == UNIT_ITEM_GROUP_STOCK) {
		switch (0xFFFFF000 & sid) {
		case UNIT_ITEM_ID_ST_WEAPON:
			sub_id = UNIT_STOCK_SUB_ID_WEAPON;
			break;
		case UNIT_ITEM_ID_ST_CHARGE:
			sub_id = UNIT_STOCK_SUB_ID_CHARGE;
			break;
		case UNIT_ITEM_ID_ST_SPECIAL:
			sub_id = UNIT_STOCK_SUB_ID_SPECIAL;
			break;
		default:
			sub_id = -1;
			break;
		}
		item_id = (0x00000FFF & sid);
	}
	else {
		sub_id = -1;
		item_id = (0x0000FFFF & sid);
	}

	// search base_index
	int base_index = 0;
	bool items_found = false;
	while (items_base[base_index].type == UNIT_TYPE_ITEMS) {
		if ((items_base[base_index].group == group_id) && (items_base[base_index].item_id == item_id)) {
			if ((sub_id != -1) && (items_base[base_index].sub_id != sub_id)) {
				base_index++;
				continue;
			}
			items_found = true;
			break;
		}
		base_index++;
	}

	// spawn item
	if (items_found) {
		ret = unit_manager_create_items(x, y, base_index);
	}

	return ret;
}

void unit_manager_clear_all_items()
{
	for (int i = 0; i < UNIT_ITEMS_LIST_SIZE; i++) {
		if (items[i].type != UNIT_TYPE_ITEMS) continue;
		unit_manager_clear_items(&items[i]);
	}
	items_index_end = 0;
}

void unit_manager_clear_items(unit_items_data_t* item)
{
	item->type = UNIT_TYPE_NONE;
	item->base = NULL;
	collision_manager_delete_shape(item->col_shape);
	item->col_shape = NULL;
	animation_manager_delete_anim_stat_data(item->anim);
	item->anim = NULL;
}

void unit_manager_items_register_item_stock()
{
	int mini_map_icon = STAGE_MINI_MAP_ICON_NONE;
	for (int i = 0; i < UNIT_ITEMS_LIST_SIZE; i++) {
		if (items[i].type != UNIT_TYPE_ITEMS) continue;
		if ((items[i].group == UNIT_ITEM_GROUP_TBOX) && (items[i].anim->stat == ANIM_STAT_FLAG_DIE)) continue;

		stage_manager_register_stock_item((void*)&items[i]);

		if ((items[i].group == UNIT_ITEM_GROUP_TBOX)
			&& ((mini_map_icon == STAGE_MINI_MAP_ICON_NONE) || (mini_map_icon > STAGE_MINI_MAP_ICON_TBOX))) {
			mini_map_icon = STAGE_MINI_MAP_ICON_TBOX;
		}
		else if ((items[i].group == UNIT_ITEM_GROUP_SPECIAL) && (items[i].item_id == UNIT_SPECIAL_ID_UNKNOWN)
			&& ((mini_map_icon == STAGE_MINI_MAP_ICON_NONE) || (mini_map_icon > STAGE_MINI_MAP_ICON_UNKNOWN))) {
			mini_map_icon = STAGE_MINI_MAP_ICON_UNKNOWN;
		}
		else if ((items[i].group == UNIT_ITEM_GROUP_STOCK) && (items[i].sub_id == UNIT_STOCK_SUB_ID_CHARGE)
			&& ((mini_map_icon == STAGE_MINI_MAP_ICON_NONE) || (mini_map_icon > STAGE_MINI_MAP_ICON_CHARGE))) {
			mini_map_icon = STAGE_MINI_MAP_ICON_CHARGE;
		}
		else if ((items[i].group == UNIT_ITEM_GROUP_STOCK) && (items[i].sub_id == UNIT_STOCK_SUB_ID_SPECIAL)
			&& ((mini_map_icon == STAGE_MINI_MAP_ICON_NONE) || (mini_map_icon > STAGE_MINI_MAP_ICON_STOCK))) {
			mini_map_icon = STAGE_MINI_MAP_ICON_STOCK;
		}
		else if ((items[i].group == UNIT_ITEM_GROUP_RECOVERY)
			&& ((mini_map_icon == STAGE_MINI_MAP_ICON_NONE) || (mini_map_icon > STAGE_MINI_MAP_ICON_HEART))) {
			mini_map_icon = STAGE_MINI_MAP_ICON_HEART;
		}
		else if ((items[i].group == UNIT_ITEM_GROUP_STOCK) && (items[i].sub_id == UNIT_STOCK_SUB_ID_WEAPON) && (items[i].item_id == UNIT_WEAPON_ID_BOM)
			&& ((mini_map_icon == STAGE_MINI_MAP_ICON_NONE) || (mini_map_icon > STAGE_MINI_MAP_ICON_BOM))) {
			mini_map_icon = STAGE_MINI_MAP_ICON_BOM;
		}
		else if (mini_map_icon == STAGE_MINI_MAP_ICON_NONE) {
			mini_map_icon = STAGE_MINI_MAP_ICON_ITEM;
		}
	}

	// set icon stat
	g_stage_data->stage_map[g_stage_data->current_stage_map_index].mini_map_icon = mini_map_icon;
}

void unit_manager_items_update()
{
	for (int i = 0; i < UNIT_ITEMS_LIST_SIZE; i++) {
		if (items[i].type != UNIT_TYPE_ITEMS) continue;

		if ((items[i].col_shape->stat == COLLISION_STAT_ENABLE)
			|| ((items[i].group == UNIT_ITEM_GROUP_TBOX) && (items[i].anim->stat == ANIM_STAT_FLAG_DIE)) ) {
#ifdef _COLLISION_ENABLE_BOX_2D_
			items[i].col_shape->x = (int)MET2PIX(items[i].col_shape->b2body->GetPosition().x);
			items[i].col_shape->y = (int)MET2PIX(items[i].col_shape->b2body->GetPosition().y);
			unit_manager_update_unit_friction((unit_data_t*)&items[i]);
#endif
		}

		// anim update
		int stat = unit_manager_unit_get_anim_stat((unit_data_t*)&items[i]);
		if ((stat != -1) && (items[i].anim->anim_stat_base_list[stat]->type == ANIM_TYPE_STATIC)) {
			if (items[i].anim->stat == ANIM_STAT_FLAG_DIE) {
				unit_manager_clear_items(&items[i]);
				continue;
			}
		}

		if ((stat != -1) && (items[i].anim->anim_stat_base_list[stat]->type == ANIM_TYPE_DYNAMIC)) {
			// set current_time
			items[i].anim->anim_stat_list[stat]->current_time += g_delta_time;

			int new_time = items[i].anim->anim_stat_list[stat]->current_time;
			int total_time = items[i].anim->anim_stat_base_list[stat]->total_time;
			if (new_time > total_time) {
				new_time = new_time % total_time;
				items[i].anim->anim_stat_list[stat]->current_time = new_time;

				// end frame event
				if (items[i].anim->stat == ANIM_STAT_FLAG_DIE) {
					if (items[i].group != UNIT_ITEM_GROUP_TBOX) { // exclude TBOX
						unit_manager_clear_items(&items[i]);
					}
					continue;
				}
				if (items[i].anim->stat == ANIM_STAT_FLAG_ATTACK) {
					if (items[i].group == UNIT_ITEM_GROUP_BOM) {
						unit_manager_items_set_anim_stat(i, ANIM_STAT_FLAG_DIE);
						continue;
					}
				}
				if (items[i].anim->stat == ANIM_STAT_FLAG_SPAWN) {
					unit_manager_items_set_anim_stat(i, ANIM_STAT_FLAG_IDLE);
					continue;
				}
			}

			// set current_frame
			int sum_frame_time = 0;
			int frame_size = items[i].anim->anim_stat_base_list[stat]->frame_size;
			for (int fi = 0; fi < frame_size; fi++) {
				sum_frame_time += items[i].anim->anim_stat_base_list[stat]->frame_list[fi]->frame_time;
				if (new_time < sum_frame_time) {
					if (items[i].anim->anim_stat_list[stat]->current_frame != fi) {
						// send command
						if (items[i].anim->anim_stat_base_list[stat]->frame_list[fi]->command == ANIM_FRAME_COMMAND_ON) {
							game_event_unit_t* msg_param = new game_event_unit_t;
							msg_param->obj1 = (unit_data_t*)(&items[i]);
							game_event_t msg = { (EVENT_MSG_UNIT_ITEMS | (0x00000001 << stat)), (void*)msg_param };
							game_event_push(&msg);
							items[i].anim->anim_stat_list[stat]->command_frame = fi;
						}
						// set frame
						items[i].anim->anim_stat_list[stat]->current_frame = fi;
					}
					break;
				}
			}
		}
	}
}

void unit_manager_items_display(int layer)
{
	for (int i = 0; i < UNIT_ITEMS_LIST_SIZE; i++) {
		if (items[i].type != UNIT_TYPE_ITEMS) continue;

		unit_display((unit_data_t*)&items[i], layer);
	}
}
