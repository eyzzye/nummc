#include <math.h>
#include "game_common.h"
#include "collision_manager.h"

#ifdef _COLLISION_ENABLE_BOX_2D_
#include "Box2D/Box2d.h"
#endif

#include "game_utils.h"
#include "game_log.h"
#include "game_window.h"
#include "game_timer.h"
#include "game_event.h"
#include "map_manager.h"
#include "unit_manager.h"

// static body
#define STATIC_SHAPE_LIST_SIZE ((((MAP_WIDTH_NUM_MAX*MAP_HEIGHT_NUM_MAX*MAP_TYPE_END + 32 /* trap, item, effect */) + 64) / 64) * 64)
static shape_data static_shape_data_list[STATIC_SHAPE_LIST_SIZE];
static shape_data* static_shape_list_box_start;
static shape_data* static_shape_list_box_end;
static shape_data* static_shape_list_rnd_start;
static shape_data* static_shape_list_rnd_end;
static Uint32 static_shape_id_end;

// dynamic body
#define DYNAMIC_SHAPE_LIST_SIZE ((((1 + UNIT_ENEMY_LIST_SIZE + UNIT_ITEMS_LIST_SIZE + UNIT_TRAP_LIST_SIZE + UNIT_PLAYER_BULLET_LIST_SIZE + UNIT_ENEMY_BULLET_LIST_SIZE) + 64) / 64) * 64)
static shape_data dynamic_shape_data_list[DYNAMIC_SHAPE_LIST_SIZE];
static shape_data* dynamic_shape_list_box_start;
static shape_data* dynamic_shape_list_box_end;
static shape_data* dynamic_shape_list_rnd_start;
static shape_data* dynamic_shape_list_rnd_end;
static Uint32 dynamic_shape_id_end;

// static wall
static b2Body* static_wall[COLLISION_STATIC_WALL_NUM];

#ifdef _COLLISION_ENABLE_BOX_2D_
b2World* g_stage_world;

class CollisionListener : public b2ContactListener
{
public:
	CollisionListener() {};
	virtual ~CollisionListener() {};

	virtual void BeginContact(b2Contact* contact) { B2_NOT_USED(contact); }
	virtual void EndContact(b2Contact* contact) { B2_NOT_USED(contact); }
	virtual void PreSolve(b2Contact* contact, const b2Manifold* oldManifold);
	virtual void PostSolve(b2Contact* contact, const b2ContactImpulse* impulse)	{ B2_NOT_USED(contact); B2_NOT_USED(impulse); }
};
static CollisionListener* col_listener;

static void update_contact(b2Contact* contact)
{
	const b2Manifold* manifold = contact->GetManifold();
	if (manifold->pointCount == 0)
	{
		return;
	}

	b2Fixture* fixtureA = contact->GetFixtureA();
	b2Fixture* fixtureB = contact->GetFixtureB();
	b2Body* bodyA = fixtureA->GetBody();
	b2Body* bodyB = fixtureB->GetBody();
	unit_data_t* unitA = (unit_data_t*)bodyA->GetUserData();
	unit_data_t* unitB = (unit_data_t*)bodyB->GetUserData();

	if ((unitA == NULL) || (unitB == NULL)) {
		return;
	}

	unit_player_data_t* unit_player = NULL;
	unit_enemy_data_t* unit_enemy = NULL;
	unit_items_data_t* unit_item = NULL;
	unit_player_bullet_data_t* unit_p_bullet = NULL;
	unit_enemy_bullet_data_t* unit_e_bullet = NULL;
	tile_instance_data_t* unit_tile = NULL;

	bool unitA_found = false;
	if (unitA->type == UNIT_TYPE_PLAYER) {
		unit_player = (unit_player_data_t*)unitA;
		unitA_found = true;
	}
	else if (unitA->type == UNIT_TYPE_ENEMY) {
		unit_enemy = (unit_enemy_data_t*)unitA;
		unitA_found = true;
	}
	else if (unitA->type == UNIT_TYPE_ITEMS) {
		unit_item = (unit_items_data_t*)unitA;
		unitA_found = true;
	}
	else if (unitA->type == UNIT_TYPE_PLAYER_BULLET) {
		unit_p_bullet = (unit_player_bullet_data_t*)unitA;
		unitA_found = true;
	}
	else if (unitA->type == UNIT_TYPE_ENEMY_BULLET) {
		unit_e_bullet = (unit_enemy_bullet_data_t*)unitA;
		unitA_found = true;
	}
	else if (unitA->type == UNIT_TYPE_TILE) {
		unit_tile = (tile_instance_data_t*)unitA;
		unitA_found = true;
	}

	bool unitB_found = false;
	if (unitB->type == UNIT_TYPE_PLAYER) {
		unit_player = (unit_player_data_t*)unitB;
		unitB_found = true;
	}
	else if (unitB->type == UNIT_TYPE_ENEMY) {
		unit_enemy = (unit_enemy_data_t*)unitB;
		unitB_found = true;
	}
	else if (unitB->type == UNIT_TYPE_ITEMS) {
		unit_item = (unit_items_data_t*)unitB;
		unitB_found = true;
	}
	else if (unitB->type == UNIT_TYPE_PLAYER_BULLET) {
		unit_p_bullet = (unit_player_bullet_data_t*)unitB;
		unitB_found = true;
	}
	else if (unitB->type == UNIT_TYPE_ENEMY_BULLET) {
		unit_e_bullet = (unit_enemy_bullet_data_t*)unitB;
		unitB_found = true;
	}
	else if (unitB->type == UNIT_TYPE_TILE) {
		unit_tile = (tile_instance_data_t*)unitB;
		unitB_found = true;
	}

	if (!unitA_found || !unitB_found) {
		return;
	}

	// RECOVERY, EFFECT
	// item(event tile)
	if ((unit_player) && (unit_item)) {
		if (unit_item->group == UNIT_ITEM_GROUP_DAMAGE) {
			if (unit_player->stat & UNIT_STAT_FLAG_INVINCIBLE) {
				return;
			}
		}

		// send event
		game_event_collision_dynamic_t* msg_param = new game_event_collision_dynamic_t;
		msg_param->obj1 = unit_player->col_shape;
		msg_param->obj2 = unit_item->col_shape;
		game_event_t msg = { EVENT_MSG_COLLISION_DYNAMIC_PvI, (void*)msg_param };
		game_event_push(&msg);
		return;
	}


	// ATTACK
	// p_bullet vs enemy
	if ((unit_p_bullet) && (unit_enemy)) {
		if (unit_p_bullet->col_shape->stat == COLLISION_STAT_ENABLE) {
			// send event
			game_event_collision_dynamic_t* msg_param = new game_event_collision_dynamic_t;
			msg_param->obj1 = unit_p_bullet->col_shape;
			msg_param->obj2 = unit_enemy->col_shape;
			game_event_t msg = { EVENT_MSG_COLLISION_DYNAMIC_PBvE, (void*)msg_param };
			game_event_push(&msg);
			return;
		}
	}

	// bullet vs map(tile)
	if ((unit_p_bullet) && (unit_tile)) {
		if (unit_p_bullet->col_shape->stat == COLLISION_STAT_ENABLE) {
			// send event
			game_event_collision_dynamic_t* msg_param = new game_event_collision_dynamic_t;
			msg_param->obj1 = unit_p_bullet->col_shape;
			msg_param->obj2 = unit_tile->col_shape;
			game_event_t msg = { EVENT_MSG_COLLISION_DYNAMIC_PBvM, (void*)msg_param };
			game_event_push(&msg);
			return;
		}
	}
	if ((unit_e_bullet) && (unit_tile)) {
		if (unit_e_bullet->col_shape->stat == COLLISION_STAT_ENABLE) {
			// send event
			game_event_collision_dynamic_t* msg_param = new game_event_collision_dynamic_t;
			msg_param->obj1 = unit_e_bullet->col_shape;
			msg_param->obj2 = unit_tile->col_shape;
			game_event_t msg = { EVENT_MSG_COLLISION_DYNAMIC_EBvM, (void*)msg_param };
			game_event_push(&msg);
			return;
		}
	}

	// DAMAGE
	// player vs enemy
	if ((unit_player) && (unit_enemy)) {
		if (unit_player->stat & UNIT_STAT_FLAG_INVINCIBLE) {
			return;
		}

		// send event
		game_event_collision_dynamic_t* msg_param = new game_event_collision_dynamic_t;
		msg_param->obj1 = unit_player->col_shape;
		msg_param->obj2 = unit_enemy->col_shape;
		game_event_t msg = { EVENT_MSG_COLLISION_DYNAMIC_PvE, (void*)msg_param };
		game_event_push(&msg);
		return;
	}

	// player vs e_bullet
	if ((unit_e_bullet) && (unit_player)) {
		if (unit_e_bullet->col_shape->stat == COLLISION_STAT_ENABLE) {
			// send event
			game_event_collision_dynamic_t* msg_param = new game_event_collision_dynamic_t;
			msg_param->obj1 = unit_e_bullet->col_shape;
			msg_param->obj2 = unit_player->col_shape;
			game_event_t msg = { EVENT_MSG_COLLISION_DYNAMIC_EBvP, (void*)msg_param };
			game_event_push(&msg);
			return;
		}
	}
}

void CollisionListener::PreSolve(b2Contact* contact, const b2Manifold* oldManifold)
{
	// do nothing
}
#endif

int collision_manager_init() {

#ifdef _COLLISION_ENABLE_BOX_2D_
	b2Vec2 gravity(0.0f, 0.0f);
	g_stage_world = new b2World(gravity);
	col_listener = new CollisionListener();
	g_stage_world->SetContactListener(col_listener);

	static_wall[COLLISION_STATIC_WALL_TOP]    = NULL;
	static_wall[COLLISION_STATIC_WALL_LEFT]   = NULL;
	static_wall[COLLISION_STATIC_WALL_RIGHT]  = NULL;
	static_wall[COLLISION_STATIC_WALL_BOTTOM] = NULL;
#endif

	static_shape_id_end = 0;
	memset(static_shape_data_list, 0, sizeof(static_shape_data_list));
	static_shape_list_box_start = NULL;
	static_shape_list_box_end = NULL;
	static_shape_list_rnd_start = NULL;
	static_shape_list_rnd_end = NULL;

	dynamic_shape_id_end = 0;
	memset(dynamic_shape_data_list, 0, sizeof(dynamic_shape_data_list));
	dynamic_shape_list_box_start = NULL;
	dynamic_shape_list_box_end = NULL;
	dynamic_shape_list_rnd_start = NULL;
	dynamic_shape_list_rnd_end = NULL;

	return 0;
}

void collision_manager_unload() {
#ifdef _COLLISION_ENABLE_BOX_2D_
	if (g_stage_world != NULL) {
		delete g_stage_world;
		g_stage_world = NULL;
	}
	if (col_listener != NULL) {
		delete col_listener;
		col_listener = NULL;
	}
#endif
}

static void update_trap_collision(b2Body* body1)
{
	unit_trap_data_t* unit_trap_data1 = NULL;
	unit_data_t* unit_data1 = (unit_data_t*)body1->GetUserData();
	if (unit_data1 == NULL) return;

	if (unit_data1->type == UNIT_TYPE_TRAP) {
		unit_trap_data1 = (unit_trap_data_t*)unit_data1;
	}
	else {
		return;
	}

	b2Fixture* fixture1 = body1->GetFixtureList();
	if (fixture1 == NULL) return;
	b2Shape* shape1 = fixture1->GetShape();
	if (shape1 == NULL) return;

	// vs player, enemy
	for (b2Body* body2 = g_stage_world->GetBodyList(); body2; body2 = body2->GetNext()) {
		unit_data_t* unit_data2 = (unit_data_t*)body2->GetUserData();

		if (unit_data2) {
			int event_id = 0;
			if (unit_data2->type == UNIT_TYPE_PLAYER) {
				event_id = EVENT_MSG_COLLISION_DYNAMIC_PvT;
			}
			else if (unit_data2->type == UNIT_TYPE_ENEMY) {
				if (((unit_enemy_data_t*)unit_data2)->resistance_stat & UNIT_EFFECT_FLAG_E_NO_TRAP_DAMAGE) {
					continue;
				}
				event_id = EVENT_MSG_COLLISION_DYNAMIC_EvT;
			}
			else {
				continue;
			}

			b2Fixture* fixture2 = body2->GetFixtureList();
			if (fixture2 == NULL) continue;
			b2Shape* shape2 = fixture2->GetShape();
			if (shape2 == NULL) continue;

			if (unit_data2->stat & UNIT_STAT_FLAG_INVINCIBLE) {
				if (unit_trap_data1->group == UNIT_TRAP_GROUP_DAMAGE) {
					continue;
				}
			}

			bool overlap = b2TestOverlap(shape1, 0, shape2, 0, body1->GetTransform(), body2->GetTransform());
			if (overlap) {
				// RECOVERY, TRAP send event
				game_event_collision_dynamic_t* msg_param = new game_event_collision_dynamic_t;
				msg_param->obj1 = unit_data2->col_shape;
				msg_param->obj2 = unit_data1->col_shape;
				game_event_t msg = { event_id, (void*)msg_param };
				game_event_push(&msg);
			}
		}
	}
}

int collision_manager_dynamic_update() {
#ifdef _COLLISION_ENABLE_BOX_2D_
	// trap collision
	for (b2Body* body1 = g_stage_world->GetBodyList(); body1; body1 = body1->GetNext()) {
		update_trap_collision(body1);
	}

	// all dynamic collision
	for (b2Contact* contact = g_stage_world->GetContactList(); contact; contact = contact->GetNext()) {
		update_contact(contact);
	}
	return 0;
#endif
}

#ifdef _COLLISION_ENABLE_BOX_2D_
static void debug_draw_circle(SDL_Point* round_points, int center_x, int center_y, float r)
{
	//SDL_Point round_points[9];
	int root2_up = 7071;
	int root2_down = 10000;
	int tmp_l = (int)(r * root2_up + 5000) / root2_down;
	int x = center_x;
	int y = center_y;

	round_points[0].x = (int)VIEW_STAGE_X(x + r);
	round_points[0].y = VIEW_STAGE_Y(y);
	round_points[1].x = VIEW_STAGE_X(x + tmp_l);
	round_points[1].y = VIEW_STAGE_Y(y - tmp_l);

	round_points[2].x = VIEW_STAGE_X(x);
	round_points[2].y = (int)VIEW_STAGE_Y(y - r);
	round_points[3].x = VIEW_STAGE_X(x - tmp_l);
	round_points[3].y = VIEW_STAGE_Y(y - tmp_l);

	round_points[4].x = (int)VIEW_STAGE_X(x - r);
	round_points[4].y = VIEW_STAGE_Y(y);
	round_points[5].x = VIEW_STAGE_X(x - tmp_l);
	round_points[5].y = VIEW_STAGE_Y(y + tmp_l);

	round_points[6].x = VIEW_STAGE_X(x);
	round_points[6].y = (int)VIEW_STAGE_Y(y + r);
	round_points[7].x = VIEW_STAGE_X(x + tmp_l);
	round_points[7].y = VIEW_STAGE_Y(y + tmp_l);

	round_points[8].x = round_points[0].x;
	round_points[8].y = round_points[0].y;
}
#endif

#define CIRCLE_POINT_NUM  33
void collision_manager_display_circle(shape_data* col_shape) {
	SDL_Point round_points[CIRCLE_POINT_NUM];
	int center_x = col_shape->x + col_shape->offset_x;
	int center_y = col_shape->y + col_shape->offset_y;
	int circle_r = ((shape_round_data*)col_shape)->r;
	float delta_angle = 2.0f * b2_pi / (CIRCLE_POINT_NUM - 1);
	float angle = 0.0f;

	for (int i = 0; i < CIRCLE_POINT_NUM - 1; i++) {
		float sin_val = game_utils_sin(angle);
		float cos_val = game_utils_cos(angle);
		round_points[i].x = (int)VIEW_STAGE_X(center_x + circle_r * sin_val);
		round_points[i].y = (int)VIEW_STAGE_Y(center_y + circle_r * cos_val);
		angle += delta_angle;
	}
	round_points[CIRCLE_POINT_NUM - 1].x = round_points[0].x;
	round_points[CIRCLE_POINT_NUM - 1].y = round_points[0].y;

	SDL_RenderDrawLines(g_ren, round_points, CIRCLE_POINT_NUM);
}

void collision_manager_display() {
#ifdef _COLLISION_ENABLE_BOX_2D_
#ifdef COLLISION_DEBUG
	for (b2Body* b = g_stage_world->GetBodyList(); b; b = b->GetNext())
	{
		b2Fixture* filxture_list = b->GetFixtureList();
		if (filxture_list) {
			b2Shape* tmp_shape = filxture_list->GetShape();
			if (tmp_shape) {
				if (tmp_shape->m_type == b2Shape::e_polygon) {
					b2PolygonShape* polygon = (b2PolygonShape*)tmp_shape;
					if (polygon->m_count != 4) {
						LOG_ERROR("Error: collision_manager_display polygon has 5 over points.");
						continue;
					}

					float tmp_x = b->GetPosition().x;
					float tmp_y = b->GetPosition().y;
					float angle = b->GetAngle();
					SDL_Point box_points[5];
					for (int i = 0; i < 4; i++) {
						float sin_val = game_utils_sin(angle);
						float cos_val = game_utils_cos(angle);
						float round_x = cos_val * polygon->m_vertices[i].x - sin_val * polygon->m_vertices[i].y;
						float round_y = sin_val * polygon->m_vertices[i].x + cos_val * polygon->m_vertices[i].y;

						box_points[i].x = (int)VIEW_STAGE_X(MET2PIX(round_x + tmp_x));
						box_points[i].y = (int)VIEW_STAGE_Y(MET2PIX(round_y + tmp_y));
					}
					box_points[4].x = box_points[0].x;
					box_points[4].y = box_points[0].y;

					SDL_SetRenderDrawColor(g_ren, 0, 255, 128, 255);
					SDL_RenderDrawLines(g_ren, box_points, 5);
				}
				else if (tmp_shape->m_type == b2Shape::e_circle) {
					SDL_Point round_points[9];

					float angle = b->GetAngle();
					float sin_val = game_utils_sin(angle);
					float cos_val = game_utils_cos(angle);
					float b2x = b->GetPosition().x;
					float b2y = b->GetPosition().y;
					float offset_x = ((b2CircleShape*)b->GetFixtureList()[0].GetShape())->m_p.x;
					float offset_y = ((b2CircleShape*)b->GetFixtureList()[0].GetShape())->m_p.y;
					float round_offset_x = cos_val * offset_x - sin_val * offset_y;
					float round_offset_y = sin_val * offset_x + cos_val * offset_y;

					int sdl_x = (int)MET2PIX(b2x + round_offset_x);
					int sdl_y = (int)MET2PIX(b2y + round_offset_y);
					float b2r = MET2PIX(filxture_list->GetShape()->m_radius);
					debug_draw_circle(round_points, sdl_x, sdl_y, b2r);

					SDL_SetRenderDrawColor(g_ren, 0, 255, 128, 255);
					SDL_RenderDrawLines(g_ren, round_points, 9);
				}
			}

			// draw AABB
			int aabb_x = (int)MET2PIX(filxture_list->GetAABB(0).lowerBound.x);
			int aabb_y = (int)MET2PIX(filxture_list->GetAABB(0).lowerBound.y);
			int aabb_w = (int)(MET2PIX(filxture_list->GetAABB(0).upperBound.x - filxture_list->GetAABB(0).lowerBound.x));
			int aabb_h = (int)(MET2PIX(filxture_list->GetAABB(0).upperBound.y - filxture_list->GetAABB(0).lowerBound.y));
			SDL_Rect rect = VIEW_STAGE_RECT(aabb_x, aabb_y, aabb_w, aabb_h);
			SDL_SetRenderDrawColor(g_ren, 0, 128, 255, 255);
			SDL_RenderDrawRect(g_ren, &rect);
		}
	}
#endif
#endif
}

//
// controll shape object
//
void collision_manager_delete_shape(shape_data* delete_data)
{
	shape_data* tmp1 = delete_data->prev;
	shape_data* tmp2 = delete_data->next;
	if (tmp1) tmp1->next = tmp2;
	if (tmp2) tmp2->prev = tmp1;

	if (delete_data->type == COLLISION_TYPE_BOX_S) {
		if (delete_data == static_shape_list_box_start) static_shape_list_box_start = tmp2;
		if (delete_data == static_shape_list_box_end) static_shape_list_box_end = tmp1;
	}
	else if (delete_data->type == COLLISION_TYPE_ROUND_S) {
		if (delete_data == static_shape_list_rnd_start) static_shape_list_rnd_start = tmp2;
		if (delete_data == static_shape_list_rnd_end) static_shape_list_rnd_end = tmp1;
	}
	else if (delete_data->type == COLLISION_TYPE_BOX_D) {
		if (delete_data == dynamic_shape_list_box_start) dynamic_shape_list_box_start = tmp2;
		if (delete_data == dynamic_shape_list_box_end) dynamic_shape_list_box_end = tmp1;
	}
	else if (delete_data->type == COLLISION_TYPE_ROUND_D) {
		if (delete_data == dynamic_shape_list_rnd_start) dynamic_shape_list_rnd_start = tmp2;
		if (delete_data == dynamic_shape_list_rnd_end) dynamic_shape_list_rnd_end = tmp1;
	}

	if (delete_data->b2body) {
		g_stage_world->DestroyBody(delete_data->b2body);
		delete_data->b2body = NULL;
	}
	memset(delete_data, 0, sizeof(shape_data));
}

shape_box_data* collision_manager_new_shape_box(int static_or_dynamic)
{
	int list_size;
	int box_id;
	shape_data* data_list = NULL;
	int start_index;
	Uint32* shape_id_end = NULL;
	shape_data** shape_list_box_end = NULL;
	shape_data** shape_list_box_start = NULL;

	if (static_or_dynamic == COLLISION_ID_STATIC_SHAPE) {
		list_size = STATIC_SHAPE_LIST_SIZE;
		box_id = COLLISION_TYPE_BOX_S;
		data_list = static_shape_data_list;
		start_index = static_shape_id_end % STATIC_SHAPE_LIST_SIZE;
		shape_id_end = &static_shape_id_end;
		shape_list_box_start = &static_shape_list_box_start;
		shape_list_box_end = &static_shape_list_box_end;
	}
    else if (static_or_dynamic == COLLISION_ID_DYNAMIC_SHAPE) {
		list_size = DYNAMIC_SHAPE_LIST_SIZE;
		box_id = COLLISION_TYPE_BOX_D;
		data_list = dynamic_shape_data_list;
		start_index = dynamic_shape_id_end % DYNAMIC_SHAPE_LIST_SIZE;
		shape_id_end = &dynamic_shape_id_end;
		shape_list_box_start = &dynamic_shape_list_box_start;
		shape_list_box_end = &dynamic_shape_list_box_end;
	}
	else {
		LOG_ERROR("collision_manager_new_shape_box error\n");
		return NULL;
	}

	// search empty node
	int new_index = -1;
	for (int i = 0; i < list_size; i++) {
		int index = start_index + i;
		if (index >= list_size) index -= list_size;
		if (data_list[index].type == COLLISION_TYPE_NONE) {
			new_index = index;
			(*shape_id_end) += i;
			break;
		}
	}
	if (new_index == -1) {
		LOG_ERROR("collision_manager_new_shape_box error\n");
		return NULL;
	}

	// set new node
	shape_box_data* tmp = (shape_box_data*)&data_list[new_index];
	tmp->id = static_or_dynamic | *shape_id_end;
	tmp->type = box_id;
	tmp->obj = NULL;
#ifdef _COLLISION_ENABLE_BOX_2D_
	tmp->b2body = NULL;
#endif

	// set prev, next
	tmp->prev = *shape_list_box_end;
	tmp->next = NULL;
	if (*shape_list_box_end) (*shape_list_box_end)->next = (shape_data*)tmp;
	*shape_list_box_end = (shape_data*)tmp;

	// only first node
	if (*shape_list_box_start == NULL) {
		tmp->prev = NULL;
		*shape_list_box_start = (shape_data*)tmp;
	}

	(*shape_id_end) += 1;
	return tmp;
}

shape_round_data* collision_manager_new_shape_round(int static_or_dynamic)
{
	int list_size;
	int round_id;
	shape_data* data_list = NULL;
	int start_index;
	Uint32* shape_id_end = NULL;
	shape_data** shape_list_rnd_end = NULL;
	shape_data** shape_list_rnd_start = NULL;

	if (static_or_dynamic == COLLISION_ID_STATIC_SHAPE) {
		list_size = STATIC_SHAPE_LIST_SIZE;
		round_id = COLLISION_TYPE_ROUND_S;
		data_list = static_shape_data_list;
		start_index = static_shape_id_end % STATIC_SHAPE_LIST_SIZE;
		shape_id_end = &static_shape_id_end;
		shape_list_rnd_start = &static_shape_list_rnd_start;
		shape_list_rnd_end = &static_shape_list_rnd_end;
	}
	else if (static_or_dynamic == COLLISION_ID_DYNAMIC_SHAPE) {
		list_size = DYNAMIC_SHAPE_LIST_SIZE;
		round_id = COLLISION_TYPE_ROUND_D;
		data_list = dynamic_shape_data_list;
		start_index = dynamic_shape_id_end % DYNAMIC_SHAPE_LIST_SIZE;
		shape_id_end = &dynamic_shape_id_end;
		shape_list_rnd_start = &dynamic_shape_list_rnd_start;
		shape_list_rnd_end = &dynamic_shape_list_rnd_end;
	}
	else {
		LOG_ERROR("collision_manager_new_shape_round error\n");
		return NULL;
	}

	// search empty node
	int new_index = -1;
	for (int i = 0; i < list_size; i++) {
		int index = start_index + i;
		if (index >= list_size) index -= list_size;
		if (data_list[index].type == COLLISION_TYPE_NONE) {
			new_index = index;
			(*shape_id_end) += i;
			break;
		}
	}
	if (new_index == -1) {
		LOG_ERROR("collision_manager_new_shape_round error\n");
		return NULL;
	}

	// set new node
	shape_round_data* tmp = (shape_round_data*)&data_list[new_index];
	tmp->id = static_or_dynamic | *shape_id_end;
	tmp->type = round_id;
	tmp->obj = NULL;
#ifdef _COLLISION_ENABLE_BOX_2D_
	tmp->b2body = NULL;
#endif

	// set prev, next
	tmp->prev = *shape_list_rnd_end;
	tmp->next = NULL;
	if (*shape_list_rnd_end) (*shape_list_rnd_end)->next = (shape_data*)tmp;
	*shape_list_rnd_end = (shape_data*)tmp;

	// only first node
	if (*shape_list_rnd_start == NULL) {
		tmp->prev = NULL;
		*shape_list_rnd_start = (shape_data*)tmp;
	}

	(*shape_id_end) += 1;
	return tmp;
}

int collision_manager_set_group(shape_data* shape, std::string& group) {
	if (group == "NONE") {
		shape->group = COLLISION_GROUP_NONE;
	}
	else if (group == "PLAYER") {
		shape->group = COLLISION_GROUP_PLAYER;
	}
	else if (group == "ENEMY") {
		shape->group = COLLISION_GROUP_ENEMY;
	}
	else if (group == "ITEMS") {
		shape->group = COLLISION_GROUP_ITEMS;
	}
	else if (group == "PLAYER_BULLET") {
		shape->group = COLLISION_GROUP_PLAYER_BULLET;
	}
	else if (group == "ENEMY_BULLET") {
		shape->group = COLLISION_GROUP_ENEMY_BULLET;
	}
	else if (group == "MAP") {
		shape->group = COLLISION_GROUP_MAP;
	}
	else if (group == "TRAP") {
		shape->group = COLLISION_GROUP_TRAP;
	}
	else {
		LOG_ERROR("collision_manager_set_group unsupport type error\n");
		return 1;
	}
	return 0;
}

int collision_manager_set_group_option(shape_data* shape, std::string& group_option) {
	if (group_option == "THROUGH_MAP") {
		shape->group |= COLLISION_GROUP_U_THROUGH_MAP;
	}
	return 0;
}

#ifdef _COLLISION_ENABLE_BOX_2D_
static void set_filter(int group, b2Filter& filter)
{
	int group_low = LOWER_BIT(group);
	int group_high = UPPER_BIT(group);
	if (group_low == COLLISION_GROUP_NONE) {
		filter.groupIndex = COLLISION_B2GROUP_NEVER_COLLIDE;
	}
	else if (group_low == COLLISION_GROUP_PLAYER) {
		filter.categoryBits = COLLISION_GROUP_MASK_PLAYER;
		filter.maskBits = COLLISION_GROUP_MASK_ENEMY | COLLISION_GROUP_MASK_ITEMS | COLLISION_GROUP_MASK_ENEMY_BULLET | COLLISION_GROUP_MASK_MAP;
	}
	else if (group_low == COLLISION_GROUP_ENEMY) {
		filter.categoryBits = COLLISION_GROUP_MASK_ENEMY;
		if (group_high & COLLISION_GROUP_U_THROUGH_MAP) filter.maskBits = COLLISION_GROUP_MASK_PLAYER | COLLISION_GROUP_MASK_ITEMS | COLLISION_GROUP_MASK_PLAYER_BULLET;
		else filter.maskBits = COLLISION_GROUP_MASK_PLAYER | COLLISION_GROUP_MASK_ITEMS | COLLISION_GROUP_MASK_PLAYER_BULLET | COLLISION_GROUP_MASK_MAP;
	}
	else if (group_low == COLLISION_GROUP_ITEMS) {
		filter.categoryBits = COLLISION_GROUP_MASK_ITEMS;
		filter.maskBits = COLLISION_GROUP_MASK_PLAYER | COLLISION_GROUP_MASK_ENEMY | COLLISION_GROUP_MASK_MAP;
	}
	else if (group_low == COLLISION_GROUP_PLAYER_BULLET) {
		filter.categoryBits = COLLISION_GROUP_MASK_PLAYER_BULLET;
		if (group_high & COLLISION_GROUP_U_THROUGH_MAP) filter.maskBits = COLLISION_GROUP_MASK_ENEMY;
		else filter.maskBits = COLLISION_GROUP_MASK_ENEMY | COLLISION_GROUP_MASK_MAP;
	}
	else if (group_low == COLLISION_GROUP_ENEMY_BULLET) {
		filter.categoryBits = COLLISION_GROUP_MASK_ENEMY_BULLET;
		if (group_high & COLLISION_GROUP_U_THROUGH_MAP) filter.maskBits = COLLISION_GROUP_MASK_PLAYER;
		else filter.maskBits = COLLISION_GROUP_MASK_PLAYER | COLLISION_GROUP_MASK_MAP;
	}
	else if (group_low == COLLISION_GROUP_MAP) {
		filter.categoryBits = COLLISION_GROUP_MASK_MAP;
		filter.maskBits = COLLISION_GROUP_MASK_PLAYER | COLLISION_GROUP_MASK_ENEMY | COLLISION_GROUP_MASK_ITEMS | COLLISION_GROUP_MASK_PLAYER_BULLET | COLLISION_GROUP_MASK_ENEMY_BULLET;
	}
	else if (group_low == COLLISION_GROUP_TRAP) {
		filter.categoryBits = COLLISION_GROUP_MASK_TRAP;
		filter.maskBits = COLLISION_GROUP_MASK_MAP;
	}
}
#endif

shape_data* collision_manager_create_static_shape(
	shape_data* base_shape,
	void* unit_data, int img_w, int img_h,
	int* x, int* y, float* vec_x, float* vec_y,
	int* face)
{
	if (base_shape->type == COLLISION_TYPE_BOX_S) {
		shape_box_data* new_shape = collision_manager_new_shape_box(COLLISION_ID_STATIC_SHAPE);
		new_shape->group = base_shape->group;
		new_shape->stat = COLLISION_STAT_ENABLE;
		new_shape->obj = unit_data;
		new_shape->offset_x = ((shape_box_data*)base_shape)->offset_x;
		new_shape->offset_y = ((shape_box_data*)base_shape)->offset_y;
		new_shape->x = *x;
		new_shape->y = *y;
		new_shape->w = ((shape_box_data*)base_shape)->w;
		new_shape->h = ((shape_box_data*)base_shape)->h;

#ifdef _COLLISION_ENABLE_BOX_2D_
		float center_x = PIX2MET(new_shape->offset_x + new_shape->w / 2.0f);
		float center_y = PIX2MET(new_shape->offset_y + new_shape->h / 2.0f);

		b2BodyDef bodyDef;
		//bodyDef.type = b2_staticBody;
		bodyDef.userData = unit_data;
		bodyDef.position.Set(PIX2MET(new_shape->x), PIX2MET(new_shape->y));
		new_shape->b2body = g_stage_world->CreateBody(&bodyDef);

		b2PolygonShape staticBox;
		b2Vec2 center_pos(center_x, center_y);
		staticBox.SetAsBox(PIX2MET(new_shape->w / 2.0f), PIX2MET(new_shape->h / 2.0f), center_pos, 0.0f);

		b2FixtureDef fixtureDef;
		fixtureDef.shape = &staticBox;
		fixtureDef.density = 1.0f;
		fixtureDef.friction = 0.3f;
		set_filter(new_shape->group, fixtureDef.filter);
		new_shape->b2body->CreateFixture(&fixtureDef);
#endif

		return (shape_data*)new_shape;
	}
	else if (base_shape->type == COLLISION_TYPE_ROUND_S) {
		shape_round_data* new_shape = collision_manager_new_shape_round(COLLISION_ID_STATIC_SHAPE);
		new_shape->group = base_shape->group;
		new_shape->stat = COLLISION_STAT_ENABLE;
		new_shape->obj = unit_data;
		new_shape->offset_x = ((shape_round_data*)base_shape)->offset_x;
		new_shape->offset_y = ((shape_round_data*)base_shape)->offset_y;
		new_shape->x = *x;
		new_shape->y = *y;
		new_shape->r = ((shape_round_data*)base_shape)->r;

#ifdef _COLLISION_ENABLE_BOX_2D_
		b2BodyDef bodyDef;
		//bodyDef.type = b2_staticBody;
		bodyDef.userData = unit_data;
		bodyDef.position.Set(PIX2MET(new_shape->x), PIX2MET(new_shape->y));
		new_shape->b2body = g_stage_world->CreateBody(&bodyDef);

		b2CircleShape circle;
		circle.m_radius = PIX2MET(new_shape->r);
		b2Vec2 center_pos(PIX2MET(new_shape->offset_x), PIX2MET(new_shape->offset_y));
		circle.m_p = center_pos;

		b2FixtureDef fixtureDef;
		fixtureDef.shape = &circle;
		fixtureDef.density = 1.0f;
		fixtureDef.friction = 0.3f;
		set_filter(new_shape->group, fixtureDef.filter);
		new_shape->b2body->CreateFixture(&fixtureDef);
#endif

		return (shape_data*)new_shape;
	}
	return NULL;
}

shape_data* collision_manager_create_dynamic_shape(
	shape_data* base_shape,
	void* unit_data, int img_w, int img_h,
	int* x, int* y,	float* vec_x, float* vec_y,
	int* face)
{
	if (base_shape->type == COLLISION_TYPE_BOX_D) {
		shape_box_data* col_box_shape = collision_manager_new_shape_box();
		shape_box_data* tmp = (shape_box_data*)base_shape;
		col_box_shape->group = tmp->group;
		col_box_shape->stat = COLLISION_STAT_ENABLE;
		col_box_shape->obj = unit_data;

		col_box_shape->x = (x == NULL) ? tmp->x : *x;
		col_box_shape->y = (y == NULL) ? tmp->y : *y;
		col_box_shape->vec_x = (vec_x == NULL) ? tmp->vec_x : *vec_x;
		col_box_shape->vec_y = (vec_y == NULL) ? tmp->vec_y : *vec_y;
		col_box_shape->old_vec_x = col_box_shape->vec_x;
		col_box_shape->old_vec_y = col_box_shape->vec_y;
		col_box_shape->float_x = (float)col_box_shape->x;
		col_box_shape->float_y = (float)col_box_shape->y;
		col_box_shape->vec_x_max = tmp->vec_x_max;
		col_box_shape->vec_y_max = tmp->vec_y_max;
		col_box_shape->vec_x_delta = tmp->vec_x_delta;
		col_box_shape->vec_y_delta = tmp->vec_y_delta;

		// set face offset
		col_box_shape->face_type = tmp->face_type;
		int new_face = (face == NULL) ? tmp->face : *face;
		collision_manager_set_face((shape_data*)col_box_shape, base_shape, img_w, img_h, new_face);

#ifdef _COLLISION_ENABLE_BOX_2D_
		unit_data_t* unit_data_base = ((unit_data_t*)unit_data)->base;

		float center_x = PIX2MET(col_box_shape->offset_x + col_box_shape->w / 2.0f);
		float center_y = PIX2MET(col_box_shape->offset_y + col_box_shape->h / 2.0f);

		b2BodyDef bodyDef;
		bodyDef.type = b2_dynamicBody;
		bodyDef.userData = unit_data;
		if (unit_data_base->col_shape->joint_type == COLLISION_JOINT_TYPE_PIN) {
			col_box_shape->joint_type = unit_data_base->col_shape->joint_type;
			col_box_shape->joint_x = unit_data_base->col_shape->joint_x;
			col_box_shape->joint_y = unit_data_base->col_shape->joint_y;
		}
		else if (unit_data_base->col_shape->joint_type == COLLISION_JOINT_TYPE_PIN_ROUND) {
			bodyDef.fixedRotation = false;
			col_box_shape->joint_type = unit_data_base->col_shape->joint_type;
			col_box_shape->joint_x = unit_data_base->col_shape->joint_x;
			col_box_shape->joint_y = unit_data_base->col_shape->joint_y;
			col_box_shape->joint_val1 = unit_data_base->col_shape->joint_val1;
		}
		else {
			bodyDef.fixedRotation = true;
		}
		bodyDef.position.Set(PIX2MET(col_box_shape->x), PIX2MET(col_box_shape->y));
		col_box_shape->b2body = g_stage_world->CreateBody(&bodyDef);

		b2PolygonShape dynamicBox;
		b2Vec2 center_pos(center_x, center_y);
		dynamicBox.SetAsBox(PIX2MET(col_box_shape->w / 2.0f), PIX2MET(col_box_shape->h / 2.0f), center_pos, 0.0f);

		b2FixtureDef fixtureDef;
		fixtureDef.shape = &dynamicBox;
		fixtureDef.density = 1.0f;
		fixtureDef.friction = 0.3f;
		set_filter(col_box_shape->group, fixtureDef.filter);
		col_box_shape->b2body->CreateFixture(&fixtureDef);

		if ((vec_x != NULL) || (vec_y != NULL)) {
			b2Vec2 new_vec(col_box_shape->vec_x, col_box_shape->vec_y);
			col_box_shape->b2body->SetLinearVelocity(new_vec);
		}

		if (unit_data_base->col_shape->joint_type == COLLISION_JOINT_TYPE_PIN) {
			if (static_wall[COLLISION_STATIC_WALL_TOP]) {
				b2RevoluteJointDef rjd;
				b2Vec2 rjd_center(PIX2MET(col_box_shape->x + unit_data_base->col_shape->joint_x), PIX2MET(col_box_shape->y + +unit_data_base->col_shape->joint_y));
				rjd.Initialize(static_wall[COLLISION_STATIC_WALL_TOP], col_box_shape->b2body, rjd_center);
				rjd.lowerAngle = 0.0f;
				rjd.upperAngle = 0.0f;
				rjd.enableLimit = true;
				rjd.enableMotor = false;
				rjd.collideConnected = false;

				g_stage_world->CreateJoint(&rjd);
			}
		}
		else if (unit_data_base->col_shape->joint_type == COLLISION_JOINT_TYPE_PIN_ROUND) {
			if (static_wall[COLLISION_STATIC_WALL_TOP]) {
				b2RevoluteJointDef rjd;
				b2Vec2 rjd_center(PIX2MET(col_box_shape->x + unit_data_base->col_shape->joint_x), PIX2MET(col_box_shape->y + +unit_data_base->col_shape->joint_y));
				rjd.Initialize(static_wall[COLLISION_STATIC_WALL_TOP], col_box_shape->b2body, rjd_center);
				rjd.motorSpeed = ((float)unit_data_base->col_shape->joint_val1 /1000.0f) * b2_pi;
				rjd.maxMotorTorque = 10000.0f;
				rjd.enableMotor = true;
				rjd.collideConnected = false;

				g_stage_world->CreateJoint(&rjd);
			}
		}
#endif

		return (shape_data*)col_box_shape;
	}
	else if (base_shape->type == COLLISION_TYPE_ROUND_D) {
		shape_round_data* col_round_shape = collision_manager_new_shape_round();
		shape_round_data* tmp = (shape_round_data*)base_shape;
		col_round_shape->group = tmp->group;
		col_round_shape->stat = COLLISION_STAT_ENABLE;
		col_round_shape->obj = unit_data;

		col_round_shape->x = (x == NULL) ? tmp->x : *x;
		col_round_shape->y = (y == NULL) ? tmp->y : *y;
		col_round_shape->r = tmp->r;
		col_round_shape->vec_x = (vec_x == NULL) ? tmp->vec_x : *vec_x;
		col_round_shape->vec_y = (vec_y == NULL) ? tmp->vec_y : *vec_y;
		col_round_shape->old_vec_x = col_round_shape->vec_x;
		col_round_shape->old_vec_y = col_round_shape->vec_y;
		col_round_shape->float_x = (float)col_round_shape->x;
		col_round_shape->float_y = (float)col_round_shape->y;
		col_round_shape->vec_x_max = tmp->vec_x_max;
		col_round_shape->vec_y_max = tmp->vec_y_max;
		col_round_shape->vec_x_delta = tmp->vec_x_delta;
		col_round_shape->vec_y_delta = tmp->vec_y_delta;

		// set face offset
		col_round_shape->face_type = tmp->face_type;
		int new_face = (face == NULL) ? tmp->face : *face;
		collision_manager_set_face((shape_data*)col_round_shape, base_shape, img_w, img_h, new_face);

#ifdef _COLLISION_ENABLE_BOX_2D_
		unit_data_t* unit_data_base = ((unit_data_t*)unit_data)->base;

		b2BodyDef bodyDef;
		bodyDef.type = b2_dynamicBody;
		bodyDef.userData = unit_data;
		if (unit_data_base->col_shape->joint_type == COLLISION_JOINT_TYPE_PIN) {
			col_round_shape->joint_type = unit_data_base->col_shape->joint_type;
			col_round_shape->joint_x = unit_data_base->col_shape->joint_x;
			col_round_shape->joint_y = unit_data_base->col_shape->joint_y;
		}
		else if (unit_data_base->col_shape->joint_type == COLLISION_JOINT_TYPE_PIN_ROUND) {
			bodyDef.fixedRotation = false;
			col_round_shape->joint_type = unit_data_base->col_shape->joint_type;
			col_round_shape->joint_x = unit_data_base->col_shape->joint_x;
			col_round_shape->joint_y = unit_data_base->col_shape->joint_y;
			col_round_shape->joint_val1 = unit_data_base->col_shape->joint_val1;
		}
		else {
			bodyDef.fixedRotation = true;
		}

		bodyDef.position.Set(PIX2MET(col_round_shape->x), PIX2MET(col_round_shape->y));
		col_round_shape->b2body = g_stage_world->CreateBody(&bodyDef);

		b2CircleShape circle;
		circle.m_radius = PIX2MET(col_round_shape->r);
		b2Vec2 center_pos(PIX2MET(col_round_shape->offset_x), PIX2MET(col_round_shape->offset_y));
		circle.m_p = center_pos;

		b2FixtureDef fixtureDef;
		fixtureDef.shape = &circle;
		fixtureDef.density = 1.0f;
		fixtureDef.friction = 0.3f;
		set_filter(col_round_shape->group, fixtureDef.filter);
		col_round_shape->b2body->CreateFixture(&fixtureDef);

		if ((vec_x != NULL) || (vec_y != NULL)) {
			b2Vec2 new_vec(col_round_shape->vec_x, col_round_shape->vec_y);
			col_round_shape->b2body->SetLinearVelocity(new_vec);
		}

		if (unit_data_base->col_shape->joint_type == COLLISION_JOINT_TYPE_PIN) {
			if (static_wall[COLLISION_STATIC_WALL_TOP]) {
				b2RevoluteJointDef rjd;
				b2Vec2 rjd_center(PIX2MET(col_round_shape->x + unit_data_base->col_shape->joint_x), PIX2MET(col_round_shape->y + +unit_data_base->col_shape->joint_y));
				rjd.Initialize(static_wall[COLLISION_STATIC_WALL_TOP], col_round_shape->b2body, rjd_center);
				rjd.lowerAngle = 0.0f;
				rjd.upperAngle = 0.0f;
				rjd.enableLimit = true;
				rjd.enableMotor = false;
				rjd.collideConnected = false;

				g_stage_world->CreateJoint(&rjd);
			}
		}
		else if (unit_data_base->col_shape->joint_type == COLLISION_JOINT_TYPE_PIN_ROUND) {
			if (static_wall[COLLISION_STATIC_WALL_TOP]) {
				b2RevoluteJointDef rjd;
				b2Vec2 rjd_center(PIX2MET(col_round_shape->x + unit_data_base->col_shape->joint_x), PIX2MET(col_round_shape->y + +unit_data_base->col_shape->joint_y));
				rjd.Initialize(static_wall[COLLISION_STATIC_WALL_TOP], col_round_shape->b2body, rjd_center);
				rjd.motorSpeed = ((float)unit_data_base->col_shape->joint_val1 / 1000.0f) * b2_pi;
				rjd.maxMotorTorque = 10000.0f;
				rjd.enableMotor = true;
				rjd.collideConnected = true;

				g_stage_world->CreateJoint(&rjd);
			}
		}
#endif

		return (shape_data*)col_round_shape;
	}

	return NULL;
}

void collision_manager_create_static_wall(int wall_type, void* unit_data, b2Body* top_left, b2Body* right_bottom)
{
#ifdef _COLLISION_ENABLE_BOX_2D_
	float x = 0.0f, y = 0.0f;
	b2Fixture* tmp_list = top_left->GetFixtureList();
	if (tmp_list) {
		b2Shape* tmp_shape = tmp_list->GetShape();
		if (tmp_shape) {
			if (tmp_shape->m_type == b2Shape::e_polygon) {
				b2PolygonShape* polygon = (b2PolygonShape*)tmp_shape;
				x = polygon->m_vertices[0].x;
				y = polygon->m_vertices[0].y;
				for (int i = 1; i < polygon->m_count; i++) {
					if (polygon->m_vertices[i].x < x) x = polygon->m_vertices[i].x;
					if (polygon->m_vertices[i].y < y) y = polygon->m_vertices[i].y;
				}
			}
		}
		x = top_left->GetPosition().x + x;
		y = top_left->GetPosition().y + y;
	}

	float end_x = 0.0f, end_y = 0.0f;
	tmp_list = right_bottom->GetFixtureList();
	if (tmp_list) {
		b2Shape* tmp_shape = tmp_list->GetShape();
		if (tmp_shape) {
			if (tmp_shape->m_type == b2Shape::e_polygon) {
				b2PolygonShape* polygon = (b2PolygonShape*)tmp_shape;
				end_x = polygon->m_vertices[0].x;
				end_y = polygon->m_vertices[0].y;
				for (int i = 1; i < polygon->m_count; i++) {
					if (end_x < polygon->m_vertices[i].x) end_x = polygon->m_vertices[i].x;
					if (end_y < polygon->m_vertices[i].y) end_y = polygon->m_vertices[i].y;
				}
			}
		}
		end_x = right_bottom->GetPosition().x + end_x;
		end_y = right_bottom->GetPosition().y + end_y;
	}
	float w = end_x - x;
	float h = end_y - y;

	b2BodyDef bodyDef;
	bodyDef.userData = unit_data;
	bodyDef.position.Set(x + w / 2.0f, y + h / 2.0f);
	static_wall[wall_type] = g_stage_world->CreateBody(&bodyDef);

	b2PolygonShape staticBox;
	staticBox.SetAsBox(w / 2.0f, h / 2.0f);

	b2FixtureDef fixtureDef;
	fixtureDef.shape = &staticBox;
	fixtureDef.density = 1.0f;
	fixtureDef.friction = 0.3f;
	set_filter(COLLISION_GROUP_MAP, fixtureDef.filter);
	static_wall[wall_type]->CreateFixture(&fixtureDef);
#endif
}

void collision_manager_set_face(shape_data* shape, shape_data* base_shape, int img_w, int img_h, int new_face)
{
	if ((shape->face) && (shape->face == new_face)) return;

	if (shape->type == COLLISION_TYPE_BOX_D) {
		shape_box_data* col_box_shape = (shape_box_data*)shape;
		shape_box_data* tmp = (shape_box_data*)base_shape;
		bool setup_flg = false;
		if (new_face == UNIT_FACE_N) {
			if ((col_box_shape->face_type == UNIT_FACE_TYPE_UD) || (col_box_shape->face_type == UNIT_FACE_TYPE_ALL)) {
				col_box_shape->offset_x = img_w - (tmp->offset_y + tmp->h);
				col_box_shape->offset_y = tmp->offset_x;
				col_box_shape->w = tmp->h;
				col_box_shape->h = tmp->w;
				shape->face = new_face;
				setup_flg = true;
			}
		}
		else if (new_face == UNIT_FACE_S) {
			if ((col_box_shape->face_type == UNIT_FACE_TYPE_UD) || (col_box_shape->face_type == UNIT_FACE_TYPE_ALL)) {
				col_box_shape->offset_x = tmp->offset_y;
				col_box_shape->offset_y = img_h - (tmp->offset_x + tmp->w);
				col_box_shape->w = tmp->h;
				col_box_shape->h = tmp->w;
				shape->face = new_face;
				setup_flg = true;
			}
		}
		else if (new_face == UNIT_FACE_W) {
			if ((col_box_shape->face_type == UNIT_FACE_TYPE_LR) || (col_box_shape->face_type == UNIT_FACE_TYPE_ALL)) {
				col_box_shape->offset_x = tmp->offset_x;
				col_box_shape->offset_y = tmp->offset_y;
				col_box_shape->w = tmp->w;
				col_box_shape->h = tmp->h;
				shape->face = new_face;
				setup_flg = true;
			}
		}
		else if (new_face == UNIT_FACE_E) {
			if ((col_box_shape->face_type == UNIT_FACE_TYPE_LR) || (col_box_shape->face_type == UNIT_FACE_TYPE_ALL)) {
				col_box_shape->offset_x = img_w - (tmp->offset_x + tmp->w);
				col_box_shape->offset_y = tmp->offset_y;
				col_box_shape->w = tmp->w;
				col_box_shape->h = tmp->h;
				shape->face = new_face;
				setup_flg = true;
			}
		}
		if (!setup_flg)
		{
			col_box_shape->offset_x = tmp->offset_x;
			col_box_shape->offset_y = tmp->offset_y;
			col_box_shape->w = tmp->w;
			col_box_shape->h = tmp->h;
			shape->face = UNIT_FACE_W;
		}
	}
	else if (shape->type == COLLISION_TYPE_ROUND_D) {
		shape_round_data* col_round_shape = (shape_round_data*)shape;
		shape_round_data* tmp = (shape_round_data*)base_shape;
		bool setup_flg = false;
		if (new_face == UNIT_FACE_N) {
			if ((col_round_shape->face_type == UNIT_FACE_TYPE_UD) || (col_round_shape->face_type == UNIT_FACE_TYPE_ALL)) {
				col_round_shape->offset_x = img_w - tmp->offset_y;
				col_round_shape->offset_y = tmp->offset_x;
				shape->face = new_face;
				setup_flg = true;
			}
		}
		else if (new_face == UNIT_FACE_S) {
			if ((col_round_shape->face_type == UNIT_FACE_TYPE_UD) || (col_round_shape->face_type == UNIT_FACE_TYPE_ALL)) {
				col_round_shape->offset_x = tmp->offset_y;
				col_round_shape->offset_y = img_h - tmp->offset_x;
				shape->face = new_face;
				setup_flg = true;
			}
		}
		else if (new_face == UNIT_FACE_W) {
			if ((col_round_shape->face_type == UNIT_FACE_TYPE_LR) || (col_round_shape->face_type == UNIT_FACE_TYPE_ALL)) {
				col_round_shape->offset_x = tmp->offset_x;
				col_round_shape->offset_y = tmp->offset_y;
				shape->face = new_face;
				setup_flg = true;
			}
		}
		else if (new_face == UNIT_FACE_E) {
			if ((col_round_shape->face_type == UNIT_FACE_TYPE_LR) || (col_round_shape->face_type == UNIT_FACE_TYPE_ALL)) {
				col_round_shape->offset_x = img_w - tmp->offset_x;
				col_round_shape->offset_y = tmp->offset_y;
				shape->face = new_face;
				setup_flg = true;
			}
		}
		if (!setup_flg)
		{
			col_round_shape->offset_x = tmp->offset_x;
			col_round_shape->offset_y = tmp->offset_y;
			shape->face = UNIT_FACE_W;
		}
	}
}

int collision_manager_set_mass(shape_data* shape, float weight) {
	int ret = -1;
	if (shape->b2body) {
		b2MassData mass_data;
		shape->b2body->GetMassData(&mass_data);
		mass_data.mass = weight;
		shape->b2body->SetMassData(&mass_data);
		ret = 0;
	}
	return ret;
}

void collision_manager_set_angle(shape_data* shape, float angle /* rad */) {
	b2Transform old_xf = shape->b2body->GetTransform();
	b2Vec2 old_center = shape->b2body->GetWorldCenter();

	float delta_angle = angle - old_xf.q.GetAngle();
	if (delta_angle == 0.0f) return;

	float sin_val = game_utils_sin(delta_angle);
	float cos_val = game_utils_cos(delta_angle);
	float vec_x = old_xf.p.x - old_center.x;
	float vec_y = old_xf.p.y - old_center.y;

	b2Vec2 new_pos;
	new_pos.x = old_center.x + cos_val * vec_x - sin_val * vec_y;
	new_pos.y = old_center.y + sin_val * vec_x + cos_val * vec_y;

	shape->b2body->SetTransform(new_pos, angle);
	shape->x = (int)MET2PIX(shape->b2body->GetPosition().x);
	shape->y = (int)MET2PIX(shape->b2body->GetPosition().y);
}

const void* collision_manager_get_filter(shape_data* shape) {
	if (shape->b2body) {
		b2Fixture* tmp_list = shape->b2body->GetFixtureList();
		if (tmp_list) {
			const b2Filter& filter = tmp_list->GetFilterData();
			return (const void*)&filter;
		}
	}
	return NULL;
}

void collision_manager_set_filter(shape_data* shape, const b2Filter& filter) {
	if (shape->b2body) {
		b2Fixture* tmp_list = shape->b2body->GetFixtureList();
		if (tmp_list) {
			tmp_list->SetFilterData(filter);
			return;
		}
	}
}

int collision_manager_set_moter_speed(shape_data* shape, float speed) {
	int ret = -1;

	if (shape->b2body) {
		b2JointEdge* joint_itr = shape->b2body->GetJointList();
		while (joint_itr != NULL) {
			b2Joint* joint = joint_itr->joint;
			if (joint->GetType() == b2JointType::e_revoluteJoint) {
				((b2RevoluteJoint*)joint)->SetMotorSpeed(speed);
				ret = 0;
			}
			joint_itr = joint_itr->next;
		}
	}

	return ret;
}

int collision_manager_set_joint(void* unit_data) {
	int ret = -1;
	shape_data* shape = ((unit_data_t*)unit_data)->col_shape;
	unit_data_t* unit_data_base = ((unit_data_t*)unit_data)->base;

	if (shape->joint_type == COLLISION_JOINT_TYPE_PIN) {
		if (static_wall[COLLISION_STATIC_WALL_TOP]) {
			b2RevoluteJointDef rjd;
			b2Vec2 rjd_center(PIX2MET(shape->x + unit_data_base->col_shape->joint_x), PIX2MET(shape->y + +unit_data_base->col_shape->joint_y));
			rjd.Initialize(static_wall[COLLISION_STATIC_WALL_TOP], shape->b2body, rjd_center);
			rjd.lowerAngle = 0.0f;
			rjd.upperAngle = 0.0f;
			rjd.enableLimit = true;
			rjd.enableMotor = false;
			rjd.collideConnected = false;

			g_stage_world->CreateJoint(&rjd);
			ret = 0;
		}
	}

	return ret;
}

int collision_manager_delete_joint(shape_data* shape) {
	int ret = -1;

	if (shape->b2body) {
		b2JointEdge* joint_itr = shape->b2body->GetJointList();
		while (joint_itr != NULL) {
			b2Joint* joint = joint_itr->joint;
			joint_itr = joint_itr->next;

			if (joint->GetType() == b2JointType::e_revoluteJoint) {
				g_stage_world->DestroyJoint(joint);
				ret = 0;
			}
		}
	}

	return ret;
}

int collision_manager_set_force(shape_data* shape, float strength_x, float strength_y) {
	int ret = -1;
	if (shape->b2body) {
		b2Vec2 force = { strength_x, strength_y };
		shape->b2body->ApplyForceToCenter(force, true);
		ret = 0;
	}
	return ret;
}
