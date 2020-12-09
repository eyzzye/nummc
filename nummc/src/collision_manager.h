#pragma once
#include "game_common.h"

//#define COLLISION_DEBUG

#ifdef _COLLISION_ENABLE_BOX_2D_
#include "Box2D/Box2d.h"

#define PIX2MET(x)  ((x)/20.0f)
#define MET2PIX(x)  ((x)*20.0f)
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

#define COLLISION_GROUP_NONE           0
#define COLLISION_GROUP_PLAYER         1
#define COLLISION_GROUP_ENEMY          2
#define COLLISION_GROUP_ITEMS          3
#define COLLISION_GROUP_PLAYER_BULLET  4
#define COLLISION_GROUP_ENEMY_BULLET   5
#define COLLISION_GROUP_MAP            6
#define COLLISION_GROUP_TRAP           7
#define COLLISION_GROUP_END            8

#define COLLISION_GROUP_MASK_PLAYER         (0x00000001 << COLLISION_GROUP_PLAYER)
#define COLLISION_GROUP_MASK_ENEMY          (0x00000001 << COLLISION_GROUP_ENEMY)
#define COLLISION_GROUP_MASK_ITEMS          (0x00000001 << COLLISION_GROUP_ITEMS)
#define COLLISION_GROUP_MASK_PLAYER_BULLET  (0x00000001 << COLLISION_GROUP_PLAYER_BULLET)
#define COLLISION_GROUP_MASK_ENEMY_BULLET   (0x00000001 << COLLISION_GROUP_ENEMY_BULLET)
#define COLLISION_GROUP_MASK_MAP            (0x00000001 << COLLISION_GROUP_MAP)
#define COLLISION_GROUP_MASK_TRAP           (0x00000001 << COLLISION_GROUP_TRAP)

#define COLLISION_B2GROUP_DEFAULT        (0)
#define COLLISION_B2GROUP_NEVER_COLLIDE  (-1)

#define COLLISION_JOINT_TYPE_NONE            0
#define COLLISION_JOINT_TYPE_PIN             1
#define COLLISION_JOINT_TYPE_PIN_ROUND       2
#define COLLISION_JOINT_TYPE_END             3

#define COLLISION_STAT_DISABLE         0
#define COLLISION_STAT_ENABLE          1

#define COLLISION_STATIC_WALL_TOP     (0)
#define COLLISION_STATIC_WALL_LEFT    (1)
#define COLLISION_STATIC_WALL_RIGHT   (2)
#define COLLISION_STATIC_WALL_BOTTOM  (3)
#define COLLISION_STATIC_WALL_NUM     (4)

typedef struct _shape_data shape_data;
typedef struct _shape_box_data shape_box_data;
typedef struct _shape_round_data shape_round_data;

struct _shape_data {
	int type;        // NONE:0, BOX:1, ROUND:2
	int id;          // COLLISION_ID_xxx_SHAPE | xxx_shape_id_end
	int group;       // COLLISION_GROUP_xxx
	void* obj;       // object address

	shape_data* prev;
	shape_data* next;
	int stat;        // DISABLE:0, ENABLE:1
#ifdef _COLLISION_ENABLE_BOX_2D_
	b2Body* b2body;
#endif

	// common param
	int x;
	int y;
	int old_x;
	int old_y;

	int offset_x;
	int offset_y;
	int face_type;
	int face;        // N:1, E:2, W:3, S:4

	float vec_x;
	float vec_y;
	float float_x;
	float float_y;

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
	int group;
	void* obj;       // object address

	shape_data* prev;
	shape_data* next;
	int stat;        // DISABLE:0, ENABLE:1
#ifdef _COLLISION_ENABLE_BOX_2D_
	b2Body* b2body;
#endif

	// common param
	int x;
	int y;
	int old_x;
	int old_y;

	int offset_x;
	int offset_y;
	int face_type;
	int face;        // N:1, E:2, W:3, S:4

	float vec_x;
	float vec_y;
	float float_x;
	float float_y;

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
	int group;
	void* obj;       // object address

	shape_data* prev;
	shape_data* next;
	int stat;        // DISABLE:0, ENABLE:1
#ifdef _COLLISION_ENABLE_BOX_2D_
	b2Body* b2body;
#endif

	// common param
	int x;
	int y;
	int old_x;
	int old_y;

	int offset_x;
	int offset_y;
	int face_type;
	int face;        // N:1, E:2, W:3, S:4

	float vec_x;
	float vec_y;
	float float_x;
	float float_y;

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
extern void collision_manager_display();
extern void collision_manager_delete_shape(shape_data* delete_data);
extern shape_box_data* collision_manager_new_shape_box(int static_or_dynamic = COLLISION_ID_DYNAMIC_SHAPE);
extern shape_round_data* collision_manager_new_shape_round(int static_or_dynamic = COLLISION_ID_DYNAMIC_SHAPE);
extern shape_data* collision_manager_create_static_shape(shape_data* base_shape, void* unit_data, int img_w, int img_h,
	int* x = NULL, int* y = NULL, float* vec_x = NULL, float* vec_y = NULL, int* face = NULL);
extern shape_data* collision_manager_create_dynamic_shape(shape_data* base_shape, void* unit_data, int img_w, int img_h,
	int* x = NULL, int* y = NULL, float* vec_x = NULL, float* vec_y = NULL, int* face = NULL);
extern void collision_manager_create_static_wall(int wall_type, void* unit_data, b2Body* top_left, b2Body* right_bottom);

extern int collision_manager_set_group(shape_data* shape, std::string& group);
extern void collision_manager_set_face(shape_data* shape, shape_data* base_shape, int img_w, int img_h, int new_face);
extern int collision_manager_set_mass(shape_data* shape, float weight);
extern int collision_manager_set_moter_speed(shape_data* shape, float speed);
