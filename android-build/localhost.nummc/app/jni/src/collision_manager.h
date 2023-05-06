#pragma once
#include "game_common.h"

//#define COLLISION_DEBUG

#ifdef _COLLISION_ENABLE_BOX_2D_
#include "Box2D/Box2D.h"

#define PIX2MET(x)  ((x)/20.0f)
#define MET2PIX(x)  ((x)*20.0f + 0.5f)
#endif

#define COLLISION_ID_STATIC_SHAPE      (0x01000000)
#define COLLISION_ID_DYNAMIC_SHAPE     (0x02000000)

#define COLLISION_TYPE_NONE            0
#define COLLISION_TYPE_BOX             (0x00000001)
#define COLLISION_TYPE_ROUND           (0x00000002)

#define COLLISION_TYPE_BOX_S           (COLLISION_ID_STATIC_SHAPE  | COLLISION_TYPE_BOX)
#define COLLISION_TYPE_ROUND_S         (COLLISION_ID_STATIC_SHAPE  | COLLISION_TYPE_ROUND)
#define COLLISION_TYPE_BOX_D           (COLLISION_ID_DYNAMIC_SHAPE | COLLISION_TYPE_BOX)
#define COLLISION_TYPE_ROUND_D         (COLLISION_ID_DYNAMIC_SHAPE | COLLISION_TYPE_ROUND)

// GROUP LOWER_BIT
#define COLLISION_GROUP_NONE           0
#define COLLISION_GROUP_PLAYER         1
#define COLLISION_GROUP_ENEMY          2
#define COLLISION_GROUP_ITEMS          3
#define COLLISION_GROUP_PLAYER_BULLET  4
#define COLLISION_GROUP_ENEMY_BULLET   5
#define COLLISION_GROUP_MAP            6
#define COLLISION_GROUP_TRAP           7
#define COLLISION_GROUP_END            8

// GROUP UPPER_BIT
#define COLLISION_GROUP_U_NONE              (0x00000000)
#define COLLISION_GROUP_U_THROUGH_MAP       (0x00010000)

// GROUP MASK (box2d)
#define COLLISION_GROUP_MASK_NONE           (0x00000001 << COLLISION_GROUP_NONE)
#define COLLISION_GROUP_MASK_PLAYER         (0x00000001 << COLLISION_GROUP_PLAYER)
#define COLLISION_GROUP_MASK_ENEMY          (0x00000001 << COLLISION_GROUP_ENEMY)
#define COLLISION_GROUP_MASK_ITEMS          (0x00000001 << COLLISION_GROUP_ITEMS)
#define COLLISION_GROUP_MASK_PLAYER_BULLET  (0x00000001 << COLLISION_GROUP_PLAYER_BULLET)
#define COLLISION_GROUP_MASK_ENEMY_BULLET   (0x00000001 << COLLISION_GROUP_ENEMY_BULLET)
#define COLLISION_GROUP_MASK_MAP            (0x00000001 << COLLISION_GROUP_MAP)
#define COLLISION_GROUP_MASK_TRAP           (0x00000001 << COLLISION_GROUP_TRAP)

#define COLLISION_B2CATEGORY_IGNORE         (0xFFFF8000)  // unsigned
#define COLLISION_B2MASK_IGNORE             (0xFFFF8000)  // unsigned
#define COLLISION_B2GROUP_IGNORE            (0xFFFF8000)  // signed

#define COLLISION_JOINT_TYPE_NONE            0
#define COLLISION_JOINT_TYPE_PIN             1
#define COLLISION_JOINT_TYPE_PIN_ROUND       2
#define COLLISION_JOINT_TYPE_END             3

#define COLLISION_STAT_DISABLE         0
#define COLLISION_STAT_ENABLE          1

// WALL
#define COLLISION_STATIC_WALL_TOP_L         0
#define COLLISION_STATIC_WALL_TOP_R         1
#define COLLISION_STATIC_WALL_BOTTOM_L      2
#define COLLISION_STATIC_WALL_BOTTOM_R      3
#define COLLISION_STATIC_WALL_LEFT_U        4
#define COLLISION_STATIC_WALL_LEFT_D        5
#define COLLISION_STATIC_WALL_RIGHT_U       6
#define COLLISION_STATIC_WALL_RIGHT_D       7
// DOOR
#define COLLISION_STATIC_WALL_TOP_DOOR      8
#define COLLISION_STATIC_WALL_BOTTOM_DOOR   9
#define COLLISION_STATIC_WALL_LEFT_DOOR    10
#define COLLISION_STATIC_WALL_RIGHT_DOOR   11
#define COLLISION_STATIC_WALL_NUM          12

typedef struct _shape_data shape_data;
typedef struct _shape_box_data shape_box_data;
typedef struct _shape_round_data shape_round_data;

struct _shape_data {
	int type;        // NONE:0, BOX:1, ROUND:2
	int id;          // COLLISION_ID_xxx_SHAPE | xxx_shape_id_end
	shape_data* prev;
	shape_data* next;

	void* obj;       // object address
	int group;       // COLLISION_GROUP_xxx
	int stat;        // DISABLE:0, ENABLE:1
#ifdef _COLLISION_ENABLE_BOX_2D_
	b2Body* b2body;
#endif

	// common param
	int x;
	int y;
	float float_x;
	float float_y;

	int offset_x;
	int offset_y;
	int face_type;
	int face;        // N:1, E:2, W:3, S:4

	float vec_x;
	float vec_y;
	float old_vec_x;
	float old_vec_y;

	float vec_x_max;
	float vec_y_max;
	float vec_x_delta;
	float vec_y_delta;

	// joint param
	int joint_type;
	int joint_x;
	int joint_y;
	int joint_val1;

	Uint32 data_field[4];
};

struct _shape_box_data {
	int type;        // NONE:0, BOX:1, ROUND:2
	int id;
	shape_data* prev;
	shape_data* next;

	void* obj;       // object address
	int group;
	int stat;        // DISABLE:0, ENABLE:1
#ifdef _COLLISION_ENABLE_BOX_2D_
	b2Body* b2body;
#endif

	// common param
	int x;
	int y;
	float float_x;
	float float_y;

	int offset_x;
	int offset_y;
	int face_type;
	int face;        // N:1, E:2, W:3, S:4

	float vec_x;
	float vec_y;
	float old_vec_x;
	float old_vec_y;

	float vec_x_max;
	float vec_y_max;
	float vec_x_delta;
	float vec_y_delta;

	// joint param
	int joint_type;
	int joint_x;
	int joint_y;
	int joint_val1;

	// box param
	int w;
	int h;
	int old_w;
	int old_h;
};

struct _shape_round_data {
	int type;        // NONE:0, BOX:1, ROUND:2
	int id;
	shape_data* prev;
	shape_data* next;

	void* obj;       // object address
	int group;
	int stat;        // DISABLE:0, ENABLE:1
#ifdef _COLLISION_ENABLE_BOX_2D_
	b2Body* b2body;
#endif

	// common param
	int x;
	int y;
	float float_x;
	float float_y;

	int offset_x;
	int offset_y;
	int face_type;
	int face;        // N:1, E:2, W:3, S:4

	float vec_x;
	float vec_y;
	float old_vec_x;
	float old_vec_y;

	float vec_x_max;
	float vec_y_max;
	float vec_x_delta;
	float vec_y_delta;

	// joint param
	int joint_type;
	int joint_x;
	int joint_y;
	int joint_val1;

	// round param
	int r;
	int padding0;
	int old_r;
	int old_padding0;
};

#ifdef _COLLISION_ENABLE_BOX_2D_
extern b2World* g_stage_world;
#endif

extern int collision_manager_init();
extern void collision_manager_unload();
extern int collision_manager_dynamic_update();
extern void collision_manager_display_circle(shape_data* col_shape);
extern void collision_manager_display();
extern void collision_manager_delete_shape(shape_data* delete_data);
extern shape_box_data* collision_manager_new_shape_box(int static_or_dynamic = COLLISION_ID_DYNAMIC_SHAPE);
extern shape_round_data* collision_manager_new_shape_round(int static_or_dynamic = COLLISION_ID_DYNAMIC_SHAPE);
extern shape_data* collision_manager_create_static_shape(shape_data* base_shape, void* unit_data, int img_w, int img_h,
	int* x = NULL, int* y = NULL, float* vec_x = NULL, float* vec_y = NULL, int* face = NULL);
extern shape_data* collision_manager_create_dynamic_shape(shape_data* base_shape, void* unit_data, int img_w, int img_h,
	int* x = NULL, int* y = NULL, float* vec_x = NULL, float* vec_y = NULL, int* face = NULL);
extern shape_data* collision_manager_create_static_wall(int wall_type, void* unit_data, int x, int y, int w, int h);

extern int collision_manager_set_group(shape_data* shape, char* group);
extern int collision_manager_set_group_option(shape_data* shape, char* group_option);
extern void collision_manager_set_face(shape_data* shape, shape_data* base_shape, int img_w, int img_h, int new_face);
extern int collision_manager_set_mass(shape_data* shape, float weight);
extern void collision_manager_set_angle(shape_data* shape, float angle /* rad */);
extern const void* collision_manager_get_filter(shape_data* shape);
extern void collision_manager_set_filter(shape_data* shape, const b2Filter& filter);
extern void collision_manager_set_filter(shape_data* shape, int maskBits = COLLISION_B2MASK_IGNORE, int categoryBits = COLLISION_B2CATEGORY_IGNORE, int groupIndex = COLLISION_B2GROUP_IGNORE);
extern int collision_manager_set_moter_speed(shape_data* shape, float speed);
extern int collision_manager_set_joint(void* unit_data);
extern int collision_manager_delete_joint(shape_data* shape);
extern int collision_manager_set_force(shape_data* shape, float strength_x, float strength_y);
