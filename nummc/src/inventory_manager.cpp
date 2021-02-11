#include "game_common.h"
#include "inventory_manager.h"

#include "unit_manager.h"
#include "map_manager.h"

static inventory_item_data_t item_data[INVENTORY_TYPE_END];
inventory_stocker_data_t g_stocker;

static inventory_item_data_t item_data_backup[INVENTORY_TYPE_END];
inventory_stocker_data_t g_stocker_backup;

//
// item list
//
typedef struct _inventory_item_ref_t inventory_item_ref_t;
struct _inventory_item_ref_t {
	const char* path;
	const int item_use_id;
};

inventory_item_ref_t weapon_list[] = {
	{"", UNIT_WEAPON_ID_NONE},
	{"units/items/stock/simple_bom/simple_bom.unit", UNIT_WEAPON_ID_BOM},
};

inventory_item_ref_t charge_list[] = {
	{"", UNIT_CHARGE_ID_NONE},
	{"units/items/stock/charge/booster/booster.unit",           UNIT_CHARGE_ID_BOOSTER},
	{"units/items/stock/charge/shield/shield.unit",             UNIT_CHARGE_ID_SHIELD},
	{"units/items/stock/charge/heart_bottle/heart_bottle.unit", UNIT_CHARGE_ID_HEART},
	{"units/items/stock/charge/bom_bottle/bom_bottle.unit",     UNIT_CHARGE_ID_BOM},
	{"units/items/stock/charge/slowed/slowed.unit",             UNIT_CHARGE_ID_SLOWED},
	{"units/items/stock/charge/rampage/rampage.unit",           UNIT_CHARGE_ID_RAMPAGE},
};

inventory_item_ref_t special_list[] = {
	{"", UNIT_SPECIAL_ID_NONE},
	{"units/items/stock/booster/booster.unit",         UNIT_SPECIAL_ID_BOOSTER},
	{"units/items/stock/bullet_curving_up/curve.unit", UNIT_SPECIAL_ID_BULLET_CURVING_UP},
	{"units/items/stock/bullet_range_up/range.unit",   UNIT_SPECIAL_ID_BULLET_RANGE_UP},
	{"units/items/stock/bullet_rate_up/rate.unit",     UNIT_SPECIAL_ID_BULLET_RATE_UP},
	{"units/items/stock/shield/shield.unit",           UNIT_SPECIAL_ID_SHIELD},
	{"units/items/stock/speed_up/speed_up.unit",       UNIT_SPECIAL_ID_SPEED_UP},
	{"units/items/stock/strength_up/strength_up.unit", UNIT_SPECIAL_ID_BULLET_STRENGTH_UP},
	{"units/items/stock/heart/heart.unit",             UNIT_SPECIAL_ID_HEART},
	{"units/items/stock/scope/scope.unit",             UNIT_SPECIAL_ID_SCOPE},
	{"units/items/stock/slowed/slowed.unit",           UNIT_SPECIAL_ID_SLOWED},
};

static int search_weapon(unit_items_data_t* item_data);
static int search_charge(unit_items_data_t* item_data);
static int search_special(unit_items_data_t* item_data);
static int inventory_manager_register_weapon(unit_items_data_t* item_data, int list_index);
static int inventory_manager_register_charge(unit_items_data_t* item_data, int list_index);
static int inventory_manager_register_special(unit_items_data_t* item_data, int list_index);


int inventory_manager_init()
{
	memset(item_data, 0, sizeof(item_data));
	memset(&g_stocker, 0, sizeof(g_stocker));

	g_stocker.weapon_item = (inventory_weapon_item_data_t*)&item_data[INVENTORY_TYPE_WEAPON];
	g_stocker.charge_item = (inventory_charge_item_data_t*)&item_data[INVENTORY_TYPE_CHARGE];
	g_stocker.special_item = (inventory_special_item_data_t*)&item_data[INVENTORY_TYPE_SPECIAL];

	g_stocker.weapon_item->item_list_idx = 1;  // fix
	g_stocker.charge_item->item_list_idx = INVENTORY_ITEM_NONE;
	g_stocker.special_item->item_list_idx = INVENTORY_ITEM_NONE;

	return 0;
}

void inventory_manager_unload()
{

}

void inventory_manager_backup_stocker()
{
	memcpy(item_data_backup, item_data, sizeof(item_data_backup));
	memcpy(&g_stocker_backup, &g_stocker, sizeof(g_stocker_backup));

	g_stocker_backup.weapon_item = (inventory_weapon_item_data_t*)&item_data_backup[INVENTORY_TYPE_WEAPON];
	g_stocker_backup.charge_item = (inventory_charge_item_data_t*)&item_data_backup[INVENTORY_TYPE_CHARGE];
	g_stocker_backup.special_item = (inventory_special_item_data_t*)&item_data_backup[INVENTORY_TYPE_SPECIAL];
}

void inventory_manager_restore_stocker()
{
	g_stocker.weapon_item->item_list_idx  = g_stocker_backup.weapon_item->item_list_idx;
	g_stocker.weapon_item->item_count     = g_stocker_backup.weapon_item->item_count;

	g_stocker.charge_item->item_list_idx  = g_stocker_backup.charge_item->item_list_idx;
	g_stocker.charge_item->item_count     = g_stocker_backup.charge_item->item_count;
	g_stocker.charge_item->charge_val     = g_stocker_backup.charge_item->charge_val;
	g_stocker.charge_item->charge_timer   = g_stocker_backup.charge_item->charge_timer;

	g_stocker.special_item->item_list_idx = g_stocker_backup.special_item->item_list_idx;
	g_stocker.special_item->item_count    = g_stocker_backup.special_item->item_count;

	g_stocker.version += 1;
}

void inventory_manager_clear_stocker_backup()
{
	memset(item_data_backup, 0, sizeof(item_data_backup));
	memset(&g_stocker_backup, 0, sizeof(g_stocker_backup));

	g_stocker_backup.weapon_item = (inventory_weapon_item_data_t*)&item_data_backup[INVENTORY_TYPE_WEAPON];
	g_stocker_backup.charge_item = (inventory_charge_item_data_t*)&item_data_backup[INVENTORY_TYPE_CHARGE];
	g_stocker_backup.special_item = (inventory_special_item_data_t*)&item_data_backup[INVENTORY_TYPE_SPECIAL];
}

static int search_weapon(unit_items_data_t* item_data)
{
	char* item_path = (char *)item_data->obj;
	int list_num = sizeof(weapon_list) / sizeof(weapon_list[0]);
	for (int i = 0; i < list_num; i++) {
		if (strcmp(weapon_list[i].path, item_path) == 0) {
			return i;
		}
	}
	return (-1);
}

static int search_charge(unit_items_data_t* item_data)
{
	char* item_path = (char*)item_data->obj;
	int list_num = sizeof(charge_list) / sizeof(charge_list[0]);
	for (int i = 0; i < list_num; i++) {
		if (strcmp(charge_list[i].path, item_path) == 0) {
			return i;
		}
	}
	return (-1);
}

static int search_special(unit_items_data_t* item_data)
{
	char* item_path = (char*)item_data->obj;
	int list_num = sizeof(special_list) / sizeof(special_list[0]);
	for (int i = 0; i < list_num; i++) {
		if (strcmp(special_list[i].path, item_path) == 0) {
			return i;
		}
	}
	return (-1);
}

static int inventory_manager_register_weapon(unit_items_data_t* item_data, int list_index)
{
	return inventory_manager_set_weapon_count(1);
}

static int inventory_manager_register_charge(unit_items_data_t* item_data, int list_index)
{
	// drop item
	int drop_item = g_stocker.charge_item->item_list_idx;
	if (drop_item >= 0)
	{
		int x, y;
		std::string drop_item_path = charge_list[drop_item].path;
		unit_manager_get_spawn_items_pos((unit_data_t*)&g_player, (unit_data_t*)&g_player, 1, &x, &y);
		int id = unit_manager_create_items(x, y, unit_manager_search_items(drop_item_path));
		unit_manager_items_set_val(id, g_stocker.charge_item->charge_val, 1);
		unit_manager_items_set_val(id, g_stocker.charge_item->charge_timer, 2);

		// init info
		g_stocker.charge_item->item_list_idx = INVENTORY_ITEM_NONE;
		g_stocker.charge_item->item_count = 0;
		g_stocker.charge_item->charge_val = 0;
		g_stocker.charge_item->charge_timer = 0;
	}

	// stock new item
	g_stocker.charge_item->item_list_idx = list_index;
	g_stocker.charge_item->item_count = 1;
	g_stocker.charge_item->charge_val = item_data->val1;
	g_stocker.charge_item->charge_timer = item_data->val2;

	g_stocker.version += 1;
	return 0;
}

static int inventory_manager_register_special(unit_items_data_t* item_data, int list_index)
{
	// drop item
	int drop_item = g_stocker.special_item->item_list_idx;
	if (drop_item >= 0)
	{
		int x, y;
		std::string drop_item_path = special_list[drop_item].path;
		unit_manager_get_spawn_items_pos((unit_data_t*)&g_player, (unit_data_t*)&g_player, 1, &x, &y);
		unit_manager_create_items(x, y, unit_manager_search_items(drop_item_path));

		// init info
		g_stocker.special_item->item_list_idx = INVENTORY_ITEM_NONE;
		g_stocker.special_item->item_count = 0;
	}

	// stock new item
	g_stocker.special_item->item_list_idx = list_index;
	g_stocker.special_item->item_count = 1;

	g_stocker.version += 1;
	return 0;
}

// clear stocker
void inventory_manager_clear_charge()
{
	int item_idx = g_stocker.charge_item->item_list_idx;
	if (item_idx >= 0)
	{
		// init info
		g_stocker.charge_item->item_list_idx = INVENTORY_ITEM_NONE;
		g_stocker.charge_item->item_count = 0;
		g_stocker.charge_item->charge_val = 0;
		g_stocker.charge_item->charge_timer = 0;

		g_stocker.version += 1;
	}
}

void inventory_manager_clear_special()
{
	int item_idx = g_stocker.special_item->item_list_idx;
	if (item_idx >= 0)
	{
		// init info
		g_stocker.special_item->item_list_idx = INVENTORY_ITEM_NONE;
		g_stocker.special_item->item_count = 0;

		g_stocker.version += 1;
	}
}

// register items
int inventory_manager_register_item(unit_items_data_t* item_data)
{
	int ret = 1;
	int list_index = search_weapon(item_data);
	if (list_index >= 0) {
		ret = inventory_manager_register_weapon(item_data, list_index);
		return ret;
	}

	list_index = search_charge(item_data);
	if (list_index >= 0) {
		ret = inventory_manager_register_charge(item_data, list_index);
		return ret;
	}

	list_index = search_special(item_data);
	if (list_index >= 0) {
		ret = inventory_manager_register_special(item_data, list_index);
		return ret;
	}

	return ret;
}

// get item
unit_items_data_t* inventory_manager_get_charge_item()
{
	int item_list_idx = g_stocker.charge_item->item_list_idx;
	if (item_list_idx >= 0) {
		std::string item_path = charge_list[item_list_idx].path;
		int item_base_index = unit_manager_search_items(item_path);
		if (item_base_index >= 0) {
			return unit_manager_get_items_base(item_base_index);
		}
	}

	return NULL;
}

unit_items_data_t* inventory_manager_get_special_item()
{
	int item_list_idx = g_stocker.special_item->item_list_idx;
	if (item_list_idx >= 0) {
		std::string item_path = special_list[item_list_idx].path;
		int item_base_index = unit_manager_search_items(item_path);
		if (item_base_index >= 0) {
			return unit_manager_get_items_base(item_base_index);
		}
	}

	return NULL;
}

// get id
int inventory_manager_get_weapon_item_ref_id()
{
	int item_list_idx = g_stocker.weapon_item->item_list_idx;
	if (item_list_idx >= 0) {
		return weapon_list[item_list_idx].item_use_id;
	}
	return INVENTORY_ITEM_NONE;
}

int inventory_manager_get_charge_item_ref_id()
{
	int item_list_idx = g_stocker.charge_item->item_list_idx;
	if (item_list_idx >= 0) {
		return charge_list[item_list_idx].item_use_id;
	}
	return INVENTORY_ITEM_NONE;
}

int inventory_manager_get_special_item_ref_id()
{
	int item_list_idx = g_stocker.special_item->item_list_idx;
	if (item_list_idx >= 0) {
		return special_list[item_list_idx].item_use_id;
	}
	return INVENTORY_ITEM_NONE;
}

// weapon count
int inventory_manager_get_weapon_count()
{
	int item_list_idx = g_stocker.weapon_item->item_list_idx;
	if (item_list_idx >= 0) { // registed item
		return g_stocker.weapon_item->item_count;
	}
	return 0;
}

int inventory_manager_set_weapon_count(int count)
{
	int item_list_idx = g_stocker.weapon_item->item_list_idx;
	if (item_list_idx >= 0) { // registed item
		int val = count;
		if ((val > 0) && (g_stocker.weapon_item->item_count < INVENTORY_WEAPON_CNT_MAX)) {
			g_stocker.weapon_item->item_count += count;
			g_stocker.weapon_item->item_count = MIN(INVENTORY_WEAPON_CNT_MAX, g_stocker.weapon_item->item_count);
			g_stocker.version += 1;
			return 0;
		}
		else if ((val < 0) && (INVENTORY_WEAPON_CNT_MIN < g_stocker.weapon_item->item_count)) {
			g_stocker.weapon_item->item_count += count;
			g_stocker.weapon_item->item_count = MAX(INVENTORY_WEAPON_CNT_MIN, g_stocker.weapon_item->item_count);
			g_stocker.version += 1;
			return 0;
		}
	}
	return 1;
}

// charge val
int inventory_manager_get_charge_val()
{
	int item_list_idx = g_stocker.charge_item->item_list_idx;
	if (item_list_idx >= 0) { // registed item
		return g_stocker.charge_item->charge_val;
	}
	return 0;
}

int inventory_manager_charge_val(int exp)
{
	int item_list_idx = g_stocker.charge_item->item_list_idx;
	if (item_list_idx >= 0) { // registed item
		int val = exp / INVENTORY_CHARGE_EXP_UNIT_VAL;
		if ((val > 0) && (g_stocker.charge_item->charge_val < INVENTORY_CHARGE_VAL_MAX)) {
			g_stocker.charge_item->charge_val += val;
			g_stocker.charge_item->charge_val = MIN(g_stocker.charge_item->charge_val, INVENTORY_CHARGE_VAL_MAX);
			g_stocker.version += 1;
			return 0;
		}
		else if ((val < 0) && (INVENTORY_CHARGE_VAL_MIN < g_stocker.charge_item->charge_val)) {
			g_stocker.charge_item->charge_val += val;
			g_stocker.charge_item->charge_val = MAX(g_stocker.charge_item->charge_val, INVENTORY_CHARGE_VAL_MIN);
			g_stocker.version += 1;
			return 0;
		}
	}
	return 1;
}
