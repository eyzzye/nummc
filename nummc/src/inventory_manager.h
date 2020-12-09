#pragma once
#include "game_common.h"

#include "unit_manager.h"

#define INVENTORY_TYPE_WEAPON   0
#define INVENTORY_TYPE_CHARGE   1
#define INVENTORY_TYPE_SPECIAL  2
#define INVENTORY_TYPE_END      3

#define INVENTORY_ITEM_NONE   (-1)

#define INVENTORY_WEAPON_CNT_MIN       0
#define INVENTORY_WEAPON_CNT_MAX      10

#define INVENTORY_CHARGE_EXP_UNIT_VAL  5
#define INVENTORY_CHARGE_VAL_MIN       0
#define INVENTORY_CHARGE_VAL_MAX      10

typedef struct _inventory_item_data_t inventory_item_data_t;
typedef struct _inventory_weapon_item_data_t inventory_weapon_item_data_t;
typedef struct _inventory_charge_item_data_t inventory_charge_item_data_t;
typedef struct _inventory_special_item_data_t inventory_special_item_data_t;
typedef struct _inventory_stocker_data_t inventory_stocker_data_t;

struct _inventory_item_data_t {
	int type;        // NONE:0
	int id;          // inventory_id_end
	void* obj;       // object address
	int reserve0;

	inventory_item_data_t* prev;
	inventory_item_data_t* next;
	int reserve1;
	int reserve2;

	Uint32 data_field[16];
};

struct _inventory_weapon_item_data_t {
	int type;        // NONE:0
	int id;          // inventory_id_end
	void* obj;       // object address
	int reserve0;

	inventory_item_data_t* prev;
	inventory_item_data_t* next;
	int item_list_idx;
	int item_count;
};

struct _inventory_charge_item_data_t {
	int type;        // NONE:0
	int id;          // inventory_id_end
	void* obj;       // object address
	int reserve0;

	inventory_item_data_t* prev;
	inventory_item_data_t* next;
	int item_list_idx;
	int item_count;

	int charge_val;
	int charge_timer;
};

struct _inventory_special_item_data_t {
	int type;        // NONE:0
	int id;          // inventory_id_end
	void* obj;       // object address
	int reserve0;

	inventory_item_data_t* prev;
	inventory_item_data_t* next;
	int item_list_idx;
	int item_count;
};

struct _inventory_stocker_data_t {
	int type;        // NONE:0, PLAYER:1
	int id;          // unit_id_end
	void* obj;       // object address
	int version;

	inventory_stocker_data_t* prev;
	inventory_stocker_data_t* next;
	int reserve1;
	int reserve2;

	inventory_weapon_item_data_t* weapon_item;
	inventory_charge_item_data_t* charge_item;
	inventory_special_item_data_t* special_item;
};

extern inventory_stocker_data_t g_stocker;
extern inventory_stocker_data_t g_stocker_backup;

extern int inventory_manager_init();
extern void inventory_manager_unload();
extern void inventory_manager_backup_stocker();
extern void inventory_manager_restore_stocker();
extern void inventory_manager_clear_stocker_backup();
extern void inventory_manager_clear_charge();
extern void inventory_manager_clear_special();
extern int inventory_manager_register_item(unit_items_data_t* item_data);
extern unit_items_data_t* inventory_manager_get_charge_item();
extern unit_items_data_t* inventory_manager_get_special_item();
extern int inventory_manager_get_weapon_item_ref_id();
extern int inventory_manager_get_charge_item_ref_id();
extern int inventory_manager_get_special_item_ref_id();
extern int inventory_manager_get_weapon_count();
extern int inventory_manager_set_weapon_count(int count);
extern int inventory_manager_get_charge_val();
extern int inventory_manager_charge_val(int exp);
