#include <time.h>
#include "game_common.h"
#include "gui_common.h"
#include "scene_manager.h"
#include "scene_play_stage.h"

#include "resource_manager.h"
#include "game_key_event.h"
#include "game_mouse_event.h"
#include "game_window.h"
#include "game_log.h"
#include "game_utils.h"
#include "game_save.h"
#include "game_timer.h"
#include "game_event.h"
#include "dialog_message.h"

#include "quest_log_manager.h"
#include "inventory_manager.h"
#include "collision_manager.h"
#include "unit_manager.h"
#include "stage_manager.h"
#include "sound_manager.h"
#include "scene_loading.h"
#include "scene_play_story.h"

#include "windows.h" // for Sleep()

#define GAME_START_WAIT_TIMER         1500
#define GAME_NEXT_STAGE_WAIT_TIMER    4000
#define KEY_SYNC_ITEM_USE_WAIT_TIMER   300
#define KEY_SYNC_WAIT_TIMER            150
#define SHOT_BULLET_WAIT_TIMER         200

static void tex_info_init();
static void set_stat_event(int stat);
static void main_event_next_load();
static void main_event_gameover();
static void dialog_message_ok();
static void section_init();
static void clear_section();
static void play_current_bgm(bool on_off);
static void drop_goal_items();
#ifdef _MAP_OFFSET_ENABLE_
static void scroll_view();
#endif
static void event_msg_handler();

// event functions
static SceneManagerFunc scene_func;
static int scene_stat;
static int return_scene_id;
static std::string stage_id;

// key event variables
static int key_sync_attack_timer;
static int key_sync_timer;
static int last_key_code;

// wait timers
static int game_start_wait_timer;
static int game_next_stage_wait_timer;

// player info
static std::string player_path;
static bool reload_player;

// goal info
static std::string goal_path = "units/trap/goal/goal.unit";
static std::string go_next_path = "units/trap/go_next/go_next.unit";
static std::string smoke_effect_path = "units/effect/smoke/smoke.unit";
static int game_next_stage_dark_alpha;

// event func
static void pre_event() {
	if (scene_stat != SCENE_STAT_ACTIVE) return;

	game_mouse_event_reset();
	map_manager_update();
}
static void key_event(SDL_Event* e) {
	if (scene_stat != SCENE_STAT_ACTIVE) return;

	game_key_event_set(e);
	game_mouse_event_set(e);
}

static void main_event_section_boss() {
	if (g_stage_data->section_stat == SECTION_STAT_ACTIVE) {
		// create trap (already have get goal)
		if (g_stage_data->stage_map[g_stage_data->current_stage_map_index].stat & STAGE_MAP_STAT_GOAL) {
			// unset SPAWN stat
			int unit_id = unit_manager_create_trap(g_stage_data->goal_x, g_stage_data->goal_y, unit_manager_search_trap(goal_path));
			unit_manager_trap_set_anim_stat(unit_id, ANIM_STAT_FLAG_IDLE);
		}
		// create trap (goal)
		else {
			// play win bgm
			play_current_bgm(false);
			sound_manager_play(resource_manager_getChunkFromPath("music/win.ogg"), SOUND_MANAGER_CH_MUSIC);

			// give bonus exp
			unit_manager_player_get_exp(g_stage_data->bonus_exp);

			drop_goal_items();
			unit_manager_create_trap(g_stage_data->goal_x, g_stage_data->goal_y, unit_manager_search_trap(goal_path));

			// create smoke effect
			unit_manager_create_effect(g_stage_data->goal_x - g_tile_width / 2, g_stage_data->goal_y - g_tile_height / 2, unit_manager_search_effect(smoke_effect_path));

			stage_manager_set_result(STAGE_RESULT_WIN);
			g_stage_data->stage_map[g_stage_data->current_stage_map_index].stat |= STAGE_MAP_STAT_GOAL;
		}

		// open door
		map_manager_open_door();

		g_stage_data->section_stat = SECTION_STAT_DOOR_SELECT_WAIT;
	}
	// wait select door
	else if (g_stage_data->section_stat == SECTION_STAT_DOOR_SELECT_WAIT) {
		// do nothing

		// select door -> scene_play_next_section() update
		//  g_stage_data->current_stage_map_index
		//  g_stage_data->current_section_index
		//  g_stage_data->current_section_data
	}
	else if (g_stage_data->section_stat == SECTION_STAT_NEXT_WAIT) {
		clear_section();
		section_init();

		// re-place player
		unit_manager_player_set_position(g_stage_data->section_start_x, g_stage_data->section_start_y);

		// stop player move
		b2Vec2 new_vec(0.0f, 0.0f);
		g_player.col_shape->b2body->SetLinearVelocity(new_vec);

		// play bgm
		play_current_bgm(true);
	}
}
static void main_event_section_normal() {
	// drop goal items
	if (g_stage_data->section_stat == SECTION_STAT_ACTIVE) {
		if (g_stage_data->stage_map[g_stage_data->current_stage_map_index].stat & STAGE_MAP_STAT_WIN) {

		}
		else {
			drop_goal_items();
			g_stage_data->stage_map[g_stage_data->current_stage_map_index].stat |= STAGE_MAP_STAT_WIN;
		}

		// open door
		map_manager_open_door();

		g_stage_data->section_stat = SECTION_STAT_DOOR_SELECT_WAIT;
	}
	// wait select door
	else if (g_stage_data->section_stat == SECTION_STAT_DOOR_SELECT_WAIT) {
		// do nothing

		// select door -> scene_play_next_section() update
		//  g_stage_data->current_stage_map_index
		//  g_stage_data->current_section_index
		//  g_stage_data->current_section_data
	}
	// load next section
	else if (g_stage_data->section_stat == SECTION_STAT_NEXT_WAIT) {
		clear_section();
		section_init();

		// re-place player
		unit_manager_player_set_position(g_stage_data->section_start_x, g_stage_data->section_start_y);

		play_current_bgm(true);
	}
}
static void main_event() {
	if (scene_stat != SCENE_STAT_ACTIVE) return;

	// next load process
	if (g_stage_data->next_load == STAGE_NEXT_LOAD_ON) {
		main_event_next_load();
		return;
	}
	// game over process
	else if (g_stage_data->result == STAGE_RESULT_LOSE) {
		main_event_gameover();
		return;
	}
	// game start process
	else if (game_start_wait_timer > 0) {
		game_start_wait_timer -= g_delta_time;
		return;
	}

	// disappear all enemies
	if (!unit_manager_enemy_exist()) {
		int section_id = g_stage_data->stage_map[g_stage_data->current_stage_map_index].section_id;

		// final section (gate open)
		if (g_stage_data->section_list[section_id]->section_type == SECTION_TYPE_BOSS) {
			main_event_section_boss();
		}
		else {
			main_event_section_normal();
		}
	}

#ifdef _COLLISION_ENABLE_BOX_2D_
	//g_stage_world->Step(1.0f/60.0f, 8, 3);
	g_stage_world->Step((g_delta_time / ONE_FRAME_TIME) * 1.0f / 60.0f, 8, 3);
#endif

	// unit update (anim timer & frame command)
	unit_manager_trap_update();
	unit_manager_items_update();
	unit_manager_enemy_update();
	unit_manager_player_bullet_update();
	unit_manager_enemy_bullet_update();
	unit_manager_player_update();
	unit_manager_effect_update();

	// dynamic collision (send collision event)
	collision_manager_dynamic_update();

	// key handler
	if (game_key_event_get(SDL_SCANCODE_ESCAPE, GUI_SELECT_WAIT_TIMER)) {
		set_stat_event(SCENE_STAT_IDLE);
		scene_manager_load(SCENE_ID_ESCAPE_MENU);
	}

	if (game_key_event_get(SDL_SCANCODE_E, KEY_SYNC_ITEM_USE_WAIT_TIMER)) {
		unit_manager_player_use_weapon_item();
	}
	if (game_key_event_get(SDL_SCANCODE_Q, KEY_SYNC_ITEM_USE_WAIT_TIMER)) {
		unit_manager_player_use_charge_item();
	}
	if (game_key_event_get(SDL_SCANCODE_SPACE, KEY_SYNC_ITEM_USE_WAIT_TIMER)) {
		unit_manager_player_use_special_item();
	}

	// attack key
	if (key_sync_attack_timer <= 0) {
		int x, y, face;
		float vec_x = 0.0f, vec_y = 0.0f;
		float abs_vec = 1.0f;

		bool attack_dirt = false;
		if (game_key_event_get(SDL_SCANCODE_UP, 0)) {
			attack_dirt = true;
			face = UNIT_FACE_N;
		}
		else if (game_key_event_get(SDL_SCANCODE_DOWN, 0)) {
			attack_dirt = true;
			face = UNIT_FACE_S;
		}
		else if (game_key_event_get(SDL_SCANCODE_LEFT, 0)) {
			attack_dirt = true;
			face = UNIT_FACE_W;
		}
		else if (game_key_event_get(SDL_SCANCODE_RIGHT, 0)) {
			attack_dirt = true;
			face = UNIT_FACE_E;
		}

		if (attack_dirt) {
			std::string bullet_path = g_player_bullet_path[g_player.weapon];
			int bullet_base_id = unit_manager_search_player_bullet(bullet_path);
			unit_manager_get_bullet_start_pos((unit_data_t*)&g_player, (unit_data_t*)unit_manager_get_player_bullet_base(bullet_base_id), UNIT_BULLET_TRACK_LINE, UNIT_BULLET_NUM_SINGLE, face, &x, &y);
			unit_manager_player_get_face_velocity(&vec_x, &vec_y, face, abs_vec);

			int unit_id = unit_manager_create_player_bullet(x, y, vec_x, vec_y, face, bullet_base_id);
			unit_manager_player_bullet_set_anim_stat(unit_id, ANIM_STAT_FLAG_ATTACK);
			unit_manager_player_bullet_set_effect_stat(unit_id, g_player.effect_stat);
			unit_manager_player_bullet_set_hp(unit_id, unit_manager_player_get_bullet_strength());
			unit_manager_player_bullet_set_bullet_life_timer(unit_id, unit_manager_player_get_bullet_life_timer());

			key_sync_attack_timer = g_player.attack_wait_timer;
			unit_manager_player_set_anim_stat(ANIM_STAT_FLAG_ATTACK);
		}
	}
	else {
		key_sync_attack_timer -= g_delta_time;
	}


	// move key
	if (key_sync_timer <= 0) {
		float vec_x = 0.0f, vec_y = 0.0f;
		int key_flg = 0;  //right:8, left:4, down:2, up:1
		bool move_dirt = false;
		if (game_key_event_get(SDL_SCANCODE_W, 0)) {
			move_dirt = true;
			vec_y = -g_player.col_shape->vec_y_delta;
			key_flg |= 0x00000001;
		}
		if (game_key_event_get(SDL_SCANCODE_S, 0)) {
			move_dirt = true;
			vec_y = g_player.col_shape->vec_y_delta;
			key_flg |= 0x00000002;
		}
		if (game_key_event_get(SDL_SCANCODE_A, 0)) {
			move_dirt = true;
			vec_x = -g_player.col_shape->vec_x_delta;
			key_flg |= 0x00000004;
		}
		if (game_key_event_get(SDL_SCANCODE_D, 0)) {
			move_dirt = true;
			vec_x = g_player.col_shape->vec_x_delta;
			key_flg |= 0x00000008;
		}

		if (move_dirt) {
			// set player velocity
			unit_manager_player_move(vec_x, vec_y);
			key_sync_timer = KEY_SYNC_WAIT_TIMER;

			// keep last key
			if ((key_flg & 0x00000001) && (last_key_code == SDL_SCANCODE_W)) ;       //last_key_code = SDL_SCANCODE_UP;
			else if ((key_flg & 0x00000002) && (last_key_code == SDL_SCANCODE_S)) ;  //last_key_code = SDL_SCANCODE_DOWN;
			else if ((key_flg & 0x00000004) && (last_key_code == SDL_SCANCODE_A)) ;  //last_key_code = SDL_SCANCODE_LEFT;
			else if ((key_flg & 0x00000008) && (last_key_code == SDL_SCANCODE_D)) ;  //last_key_code = SDL_SCANCODE_RIGHT;
			else {
				// set new key code
				if ((key_flg & 0x00000001) && (last_key_code != SDL_SCANCODE_W))      last_key_code = SDL_SCANCODE_W;
				else if ((key_flg & 0x00000002) && (last_key_code != SDL_SCANCODE_S)) last_key_code = SDL_SCANCODE_S;
				else if ((key_flg & 0x00000004) && (last_key_code != SDL_SCANCODE_A)) last_key_code = SDL_SCANCODE_A;
				else if ((key_flg & 0x00000008) && (last_key_code != SDL_SCANCODE_D)) last_key_code = SDL_SCANCODE_D;
			}
		}
	}
	else {
		key_sync_timer -= g_delta_time;
	}

#ifdef _MAP_OFFSET_ENABLE_
	// move view
	scroll_view();
#endif

	// ai control (key emulator)
	unit_manager_enemy_ai_update();

	// message event
	event_msg_handler();

	// update hud
	hud_manager_update();
}

static void pre_draw() {
	if (scene_stat != SCENE_STAT_ACTIVE) return;
}
static void draw() {
	if (scene_stat != SCENE_STAT_ACTIVE) return;

	// set background
	SDL_SetRenderDrawColor(g_ren, 255, 255, 255, 255);
	SDL_RenderFillRect(g_ren, &g_screen_size);

	for (int layer = ANIM_TEX_LAYER_MIN; layer <= ANIM_TEX_LAYER_MAX; layer++) {
		// draw map
		map_manager_display(layer);

		// draw unit
		unit_manager_trap_display(layer);
		unit_manager_effect_display(layer);
		unit_manager_items_display(layer);
		unit_manager_enemy_display(layer);
		unit_manager_player_display(layer);
		unit_manager_player_bullet_display(layer);
		unit_manager_enemy_bullet_display(layer);
	}

	// draw hud
	hud_manager_display();

	// draw quest_log
	quest_log_manager_display();

	// draw collision shapes
	collision_manager_display();

	// draw ai info
	ai_manager_display();

	// dark sceen
	if ((g_dialog_message_enable) || (game_start_wait_timer > 0))
	{
		// set background dark
		SDL_SetRenderDrawColor(g_ren, 0, 0, 0, 128);
		SDL_RenderFillRect(g_ren, &g_screen_size);
	}

	// exit gate effect
	if ((g_stage_data->next_load == STAGE_NEXT_LOAD_ON) && (g_stage_data->result == STAGE_RESULT_WIN))
	{
		// set dark effect
		SDL_SetRenderDrawColor(g_ren, 0, 0, 0, game_next_stage_dark_alpha);
		SDL_RenderFillRect(g_ren, &g_screen_size);
	}

	// dialog exit
	if (g_dialog_message_enable) {
		// draw dialog
		dialog_message_draw();
	}

	// disable region
	if (g_screen_size.y > 0) {
		SDL_SetRenderDrawColor(g_ren, 0, 0, 0, 255);
		SDL_Rect disabel_region = { 0, 0, g_screen_size.w, g_screen_size.y };
		SDL_RenderFillRect(g_ren, &disabel_region);
		disabel_region = { 0, g_screen_size.y + g_screen_size.h, g_screen_size.w, g_screen_size.y };
		SDL_RenderFillRect(g_ren, &disabel_region);
	}
	else if (g_screen_size.x > 0) {
		SDL_SetRenderDrawColor(g_ren, 0, 0, 0, 255);
		SDL_Rect disabel_region = { 0, 0, g_screen_size.x, g_screen_size.h };
		SDL_RenderFillRect(g_ren, &disabel_region);
		disabel_region = { g_screen_size.x + g_screen_size.w, 0, g_screen_size.x, g_screen_size.h };
		SDL_RenderFillRect(g_ren, &disabel_region);
	}

	SDL_RenderPresent(g_ren);
}
static void after_draw() {
	if (scene_stat != SCENE_STAT_ACTIVE) return;

	game_event_clear();
}

static void pre_load_event(void* null) {
	// load resource files
	resource_manager_load_dat("scenes/scene_play_stage.dat");

	// set texture position
	tex_info_init();

	Sleep(3000);

	// collision init
	collision_manager_init();

	// animation init
	animation_manager_init();

	// ai init
	ai_manager_init();

	// map init
	map_manager_init();

	// load stage
	stage_manager_init();
	std::string stage_path = (stage_id == "final") ? ("scenes/stages/final.dat") : ("scenes/stages/stage" + stage_id + ".dat");
	stage_manager_load(stage_path);

	// init unit manager
	unit_manager_init();

	// inventory init
	inventory_manager_init();

	// hud init
	hud_manager_init();

	// quest_log init
	quest_log_manager_init();

	// load effect
	unit_manager_load_effect("units/effect/boost/boost.unit");
	unit_manager_load_effect("units/effect/fire_up/fire_up.unit");
	unit_manager_load_effect("units/effect/fire_up/48x48/fire_up.unit");
	unit_manager_load_effect("units/effect/fire_up/64x64/fire_up.unit");
	unit_manager_load_effect("units/effect/freeze_up/freeze_up.unit");
	unit_manager_load_effect("units/effect/freeze_up/48x48/freeze_up.unit");
	unit_manager_load_effect("units/effect/freeze_up/64x64/freeze_up.unit");
	unit_manager_load_effect("units/effect/shield/shield.unit");

	// load common items
	for (int i = 0; i < g_stage_data->common_items_list.size(); i++) {
		unit_manager_load_items_def(g_stage_data->common_items_list[i]);
	}

	// load goal (trap)
	unit_manager_load_trap(goal_path);
	unit_manager_load_trap(go_next_path);

	// load player unit
	unit_manager_load_player_effects();
	unit_manager_load_player(player_path);
	unit_manager_create_player(g_stage_data->start_x, g_stage_data->start_y);
#ifdef _MAP_OFFSET_ENABLE_
	scroll_view();
#endif
	if (reload_player) {
		unit_manager_restore_player();
		inventory_manager_restore_stocker();
	}
	else { // NewGame
		int cursor_index = 0;
		game_save_get_config_default_slot(&cursor_index);

		std::string start_stage = g_stage_data->id;
		std::string start_player = game_utils_get_filename(player_path);
		if (game_save_set_config_slot(cursor_index, start_player, start_stage) == 0) {
			game_save_config_save();
		}
	}

	// first section
	g_stage_data->current_section_index = SECTION_INDEX_START;
	g_stage_data->current_section_data = g_stage_data->section_list[g_stage_data->current_section_index];
	section_init();

	scene_manager_set_pre_load_stat(true);
}
static void load_event() {
	return_scene_id = scene_manager_get_scene_id();
	if (return_scene_id != SCENE_ID_ESCAPE_MENU) game_start_wait_timer = GAME_START_WAIT_TIMER;
	game_next_stage_wait_timer = 0;
	last_key_code = SDL_SCANCODE_D;
	game_utils_random_init((unsigned int)time(NULL));

	// resize
	quest_log_manager_reset();

	// resize
	hud_manager_reset();

	// init message event
	game_event_init();

	// key event switch
	game_key_event_init();
	key_sync_attack_timer = 0;
	key_sync_timer = 0;
	g_key_event_enable.push_back(SDL_SCANCODE_ESCAPE);
	g_key_event_enable.push_back(SDL_SCANCODE_RETURN);
	g_key_event_enable.push_back(SDL_SCANCODE_W);
	g_key_event_enable.push_back(SDL_SCANCODE_A);
	g_key_event_enable.push_back(SDL_SCANCODE_S);
	g_key_event_enable.push_back(SDL_SCANCODE_D);
	g_key_event_enable.push_back(SDL_SCANCODE_UP);
	g_key_event_enable.push_back(SDL_SCANCODE_DOWN);
	g_key_event_enable.push_back(SDL_SCANCODE_LEFT);
	g_key_event_enable.push_back(SDL_SCANCODE_RIGHT);
	g_key_event_enable.push_back(SDL_SCANCODE_E);
	g_key_event_enable.push_back(SDL_SCANCODE_Q);
	g_key_event_enable.push_back(SDL_SCANCODE_SPACE);

	game_mouse_event_init(0, 400, 200, 150, 5);
}
static void unload_event() {
	quest_log_manager_unload();
	hud_manager_unload();
	inventory_manager_unload();
	unit_manager_unload();
	map_manager_unload();
	animation_manager_unload();
	ai_manager_unload();
	collision_manager_unload();
	stage_manager_unload();
}
static int get_stat_event() {
	return scene_stat;
}
static void set_stat_event(int stat) {
	if (stat == SCENE_STAT_IDLE) {
		play_current_bgm(false);
		stage_manager_set_stat(STAGE_STAT_IDLE);
	}
	else if (stat == SCENE_STAT_ACTIVE) {
		play_current_bgm(true);
		stage_manager_set_stat(STAGE_STAT_ACTIVE);
	}
	scene_stat = stat;
}

static void main_event_next_load()
{
	game_next_stage_wait_timer += g_delta_time;
	game_next_stage_dark_alpha = MAX(0, MIN( (game_next_stage_wait_timer - 1200) / 10, 255));  // A:0->255 (1200[msec]~)

	if (game_next_stage_wait_timer > GAME_NEXT_STAGE_WAIT_TIMER) {
		// NN -> load next stage
		if (g_stage_data->next_stage_id[0] != '_') {
			if (g_stage_data->next_stage_id != "final") { // exclude "final"
				// auto save
				int load_slot_index;
				game_save_get_config_default_slot(&load_slot_index);
				std::string slot_player = game_utils_get_filename(player_path);
				if (load_slot_index >= 0) {
					// save current stage
					if (game_save_set_config_slot(load_slot_index, slot_player, g_stage_data->next_stage_id) == 0) {
						game_save_config_save();
					}
				}
			}

			// load next stage
			scene_loading_set_stage(g_stage_data->next_stage_id);
			scene_play_stage_set_stage_id(g_stage_data->next_stage_id);

			// keep current player/stocker
			unit_manager_backup_player();
			inventory_manager_backup_stocker();
			reload_player = true;

			set_stat_event(SCENE_STAT_IDLE);
			scene_manager_load(SCENE_ID_PLAY_STAGE, true);
		}
		// _endNN -> end scroll
		else {
			// unlock charactor
			int unlock_stat; game_save_get_config_unlock_stat(&unlock_stat);
			int unlock_index = -1;
			for (int unlock_i = RESOURCE_MANAGER_PROFILE_LIST_SIZE; unlock_i > 0; unlock_i--) {
				if (unlock_stat & (0x00000001 << unlock_i)) {
					unlock_index = unlock_i;
					break;
				}
			}

			// current player is newest player.
			if ((unlock_index != -1) && (unlock_index < RESOURCE_MANAGER_PROFILE_LIST_SIZE)
				&& (strcmp(g_resource_manager_profile[unlock_index].unit_path, (char *)g_player.obj) == 0))
			{
				int new_unlock_stat = (unlock_stat << 1 | 0x00000002); // new charactor | infinity
				if (game_save_set_config_unlock_stat(new_unlock_stat) == 0) {
					game_save_config_save();
				}
			}

			set_stat_event(SCENE_STAT_IDLE);
			std::string story_path = "scenes/story/infinity/ending.dat";
			for (int prof_i = 0; prof_i < RESOURCE_MANAGER_PROFILE_LIST_SIZE; prof_i++) {
				if (strcmp(g_resource_manager_profile[prof_i].unit_path, (char*)g_player.obj) == 0) {
					story_path = g_resource_manager_profile[prof_i].ending_path;
					break;
				}
			}
			scene_play_story_set_story(story_path);
			scene_manager_load(SCENE_ID_PLAY_STORY);
		}
	}
}
static void main_event_gameover()
{
	if (g_dialog_message_enable) {
		dialog_message_event();
		return; // recive only dialog key
	}

	// unit update
	unit_manager_player_gameover();
}

static void dialog_message_ok()
{
	unload_event();

	sound_manager_play(resource_manager_getChunkFromPath("sounds/sfx_click1.ogg"), SOUND_MANAGER_CH_SFX2);
	scene_stat = SCENE_STAT_IDLE;
	scene_manager_load(SCENE_ID_TOP_MENU);

	dialog_message_set_enable(false);
}

static void section_init()
{
	// clear tmp region
	g_stage_data->section_timer = 0;

	// first section
	if ((g_stage_data->current_section_index == SECTION_INDEX_START) && !(g_stage_data->stage_map[g_stage_data->current_stage_map_index].stat & STAGE_MAP_STAT_WIN)) {
		g_stage_data->drop_judge_count = 0;

		// generate stage map
		map_manager_create_stage_map();

		// load enemy bullet
		unit_manager_load_enemy_bullet("units/bullet/point/enemy/point.unit");
		unit_manager_load_enemy_bullet("units/bullet/fire/enemy/fire.unit");
		unit_manager_load_enemy_bullet("units/bullet/ice/enemy/ice.unit");
		unit_manager_load_enemy_bullet("units/bullet/ice_ball/enemy/ice_ball.unit");
		unit_manager_load_enemy_bullet("units/bullet/leaser/enemy/leaser.unit");
		unit_manager_load_enemy_bullet("units/bullet/big_point/enemy/big_point.unit");

		// load player bullet
		unit_manager_load_player_bullet("units/bullet/point/point.unit");
		unit_manager_load_player_bullet("units/bullet/fire/fire.unit");
		unit_manager_load_player_bullet("units/bullet/ice/ice.unit");
		//unit_manager_load_player_bullet("units/bullet/ice_ball/ice_ball.unit");
		unit_manager_load_player_bullet("units/bullet/leaser/leaser.unit");
		unit_manager_load_player_bullet("units/bullet/big_point/big_point.unit");

		// load effect
		unit_manager_load_effect("units/effect/smoke/smoke.unit");
		unit_manager_load_effect("units/effect/trash/trash.unit");
		unit_manager_load_effect("units/effect/star/star.unit");
		unit_manager_load_effect("units/effect/damage/damage.unit");
		unit_manager_load_effect("units/effect/damage/48x48/damage.unit");
		unit_manager_load_effect("units/effect/damage/64x64/damage.unit");
		unit_manager_load_effect("units/effect/high_light_line/high_light_line.unit");
		unit_manager_load_effect("units/effect/shadow/shadow.unit");
		unit_manager_load_effect("units/effect/shadow/48x48/shadow.unit");
		unit_manager_load_effect("units/effect/shadow/64x64/shadow.unit");
		unit_manager_load_effect("units/effect/shadow/shadow_drop.unit");
		unit_manager_load_effect("units/effect/shadow/48x48/shadow_drop.unit");
		unit_manager_load_effect("units/effect/shadow/64x64/shadow_drop.unit");
	}

	// load section map
	map_manager_load_section_map();

	// set wall & door (arrange size by g_player.col_shape)
	map_manager_create_wall();
	map_manager_create_door();

	section_data_t* p_section = g_stage_data->current_section_data;

	if (g_stage_data->stage_map[g_stage_data->current_stage_map_index].stat & STAGE_MAP_STAT_WIN) {
		// restore from item_stocker (items, drop items, goal items)
		stage_manager_create_all_stock_item();

		// load trap
		for (int i = 0; i < p_section->trap_list.size(); i++) {
			unit_manager_load_trap(p_section->trap_list[i]->type);
			if ((p_section->trap_list[i]->x >= 0) && (p_section->trap_list[i]->y >= 0)) {
				unit_manager_create_trap(p_section->trap_list[i]->x, p_section->trap_list[i]->y, unit_manager_search_trap(p_section->trap_list[i]->type));
			}
		}
	}
	else if (g_stage_data->stage_map[g_stage_data->current_stage_map_index].stat & STAGE_MAP_STAT_GOAL) {
		// restore from item_stocker (items, drop items, goal items)
		stage_manager_create_all_stock_item();

		// load trap
		for (int i = 0; i < p_section->trap_list.size(); i++) {
			unit_manager_load_trap(p_section->trap_list[i]->type);
			if ((p_section->trap_list[i]->x >= 0) && (p_section->trap_list[i]->y >= 0)) {
				unit_manager_create_trap(p_section->trap_list[i]->x, p_section->trap_list[i]->y, unit_manager_search_trap(p_section->trap_list[i]->type));
			}
		}
	}
	// STAGE_MAP_STAT_NONE
	else {
		// load items
		for (int i = 0; i < p_section->items_list.size(); i++) {
			unit_manager_load_items(p_section->items_list[i]->type);
			if ((p_section->items_list[i]->x >= 0) && (p_section->items_list[i]->y >= 0)) {
				unit_manager_create_items(p_section->items_list[i]->x, p_section->items_list[i]->y, unit_manager_search_items(p_section->items_list[i]->type));
			}
		}

		// load drop items
		for (int i = 0; i < p_section->drop_items_list.size(); i++) {
			int drop_item_id = unit_manager_search_items(p_section->drop_items_list[i]);
			p_section->drop_items_id_list.push_back(drop_item_id);
		}

		// load goal items
		for (int i = 0; i < p_section->goal_items_list.size(); i++) {
			int goal_item_id = unit_manager_search_items(p_section->goal_items_list[i]);
			p_section->goal_items_id_list.push_back(goal_item_id);
		}

		// load trap
		for (int i = 0; i < p_section->trap_list.size(); i++) {
			unit_manager_load_trap(p_section->trap_list[i]->type);
			if ((p_section->trap_list[i]->x >= 0) && (p_section->trap_list[i]->y >= 0)) {
				unit_manager_create_trap(p_section->trap_list[i]->x, p_section->trap_list[i]->y, unit_manager_search_trap(p_section->trap_list[i]->type));
			}
		}

		// load enemy unit
		for (int i = 0; i < p_section->enemy_list.size(); i++) {
			unit_manager_load_enemy(p_section->enemy_list[i]->type);
			int enemy_id = unit_manager_create_enemy(p_section->enemy_list[i]->x, p_section->enemy_list[i]->y, p_section->enemy_list[i]->face, unit_manager_search_enemy(p_section->enemy_list[i]->type));
			if (p_section->enemy_list[i]->ai_step != 0) {
				unit_manager_set_ai_step(enemy_id, p_section->enemy_list[i]->ai_step);
			}

			if (g_stage_data->current_section_index != 0) {
				// create smoke effect
				unit_manager_create_effect(p_section->enemy_list[i]->x - g_tile_width / 2, p_section->enemy_list[i]->y - g_tile_height / 2, unit_manager_search_effect(smoke_effect_path));
			}
		}
	}

	char buff[32] = { '\0' };
	sprintf_s(buff, "start section: %d", g_stage_data->current_section_index);
	quest_log_manager_set_new_message((char*)buff, (int)strlen(buff));

	// set section_stat active
	g_stage_data->section_stat = SECTION_STAT_ACTIVE;
}

static void clear_section()
{
	// unit(enemy,items,trap,player_bullet,enemy_bullet)/map unload
	unit_manager_clear_all_enemy();
	unit_manager_clear_all_trap();
	unit_manager_clear_all_items();
	unit_manager_clear_all_player_bullet();
	unit_manager_clear_all_enemy_bullet();
	unit_manager_clear_all_effect(UNIT_EFFECT_CLEAR_TYPE_NONE);
	map_manager_clear_all_instance();
}

static void play_current_bgm(bool on_off)
{
	bool played = false;
	if (on_off) {
		if (g_stage_data->current_section_data->bgm_list.size() > 0) {
			if (!(g_stage_data->stage_map[g_stage_data->current_stage_map_index].stat & STAGE_MAP_STAT_GOAL)
				&& !(g_stage_data->stage_map[g_stage_data->current_stage_map_index].stat & STAGE_MAP_STAT_WIN)) {
				sound_manager_play(g_stage_data->current_section_data->bgm_list[0]->chunk, SOUND_MANAGER_CH_MUSIC, -1);
				played = true;
			}
		}
	}

	if (played == false) {
		sound_manager_stop(SOUND_MANAGER_CH_MUSIC);
	}
}

static void drop_goal_items()
{
	// drop items
	int x, y, drop_x;
	int drop_y = g_map_y_max * g_tile_height / 2;
	unit_manager_get_center_position((unit_data_t*)&g_player, &x, &y);
	if (x > (g_map_x_max * g_tile_width / 2)) {
		// drop on left side
		drop_x = g_map_x_max * g_tile_width / 4;
	}
	else {
		// drop on right side
		drop_x = g_map_x_max * g_tile_width * 3 / 4;
	}

	int count = (int)g_stage_data->current_section_data->goal_items_id_list.size();
	int i = 0;
	for (int h = 0; h < 3; h++) {
		if (i >= count) break;
		for (int w = 0; w < 3; w++) {
			if (i >= count) break;
			unit_manager_create_items(drop_x + w * g_tile_width, drop_y + h * g_tile_height, g_stage_data->current_section_data->goal_items_id_list[i]);
			i += 1;
		}
	}
}

#ifdef _MAP_OFFSET_ENABLE_
static void scroll_view()
{
	int new_x = 0, new_y = 0;
	unit_manager_get_center_position((unit_data_t*)&g_player, &new_x, &new_y);

	int new_offset_x = VIEW_STAGE(new_x) - VIEW_SCALE(SCREEN_WIDTH) / 2;
	int new_offset_y = VIEW_STAGE(new_y) - VIEW_SCALE(SCREEN_HEIGHT) / 2;
	if ((new_offset_x != g_map_offset_x) || (new_offset_y != g_map_offset_y)) {
		map_manager_set_offset(new_offset_x, new_offset_y);
	}
}
#endif

static void event_msg_handler() 
{
	game_event_t msg;
	while ((g_stage_data->stat == STAGE_STAT_ACTIVE) && (game_event_pop(&msg)))
	{
		if (msg.id == EVENT_MSG_COLLISION_DYNAMIC_PvI) {
			game_event_collision_dynamic_t* msg_param = (game_event_collision_dynamic_t*)msg.param;
			if (LOWER_BIT(msg_param->obj2->group) == COLLISION_GROUP_ITEMS) {
				if (msg_param->obj2->obj) {
					unit_items_data_t* item_data = (unit_items_data_t*)msg_param->obj2->obj;
					if ((item_data->type == UNIT_TYPE_ITEMS) && (item_data->col_shape->stat == COLLISION_STAT_ENABLE)) {
						unit_manager_player_get_item(item_data);
					}
				}
			}
		}

		if (msg.id == EVENT_MSG_COLLISION_DYNAMIC_PvT) {
			game_event_collision_dynamic_t* msg_param = (game_event_collision_dynamic_t*)msg.param;
			if (LOWER_BIT(msg_param->obj2->group) == COLLISION_GROUP_TRAP) {
				if (msg_param->obj2->obj) {
					unit_trap_data_t* trap_data = (unit_trap_data_t*)msg_param->obj2->obj;
					if ((trap_data->type == UNIT_TYPE_TRAP) && (trap_data->col_shape->stat == COLLISION_STAT_ENABLE)) {
						unit_manager_player_trap(trap_data);
					}
				}
			}
		}
		if (msg.id == EVENT_MSG_COLLISION_DYNAMIC_EvT) {
			game_event_collision_dynamic_t* msg_param = (game_event_collision_dynamic_t*)msg.param;
			if (LOWER_BIT(msg_param->obj2->group) == COLLISION_GROUP_TRAP) {
				if (msg_param->obj2->obj) {
					unit_trap_data_t* trap_data = (unit_trap_data_t*)msg_param->obj2->obj;
					if ((trap_data->type == UNIT_TYPE_TRAP) && (trap_data->col_shape->stat == COLLISION_STAT_ENABLE)) {
						unit_enemy_data_t* enemy_data = (unit_enemy_data_t*)msg_param->obj1->obj;
						if ((enemy_data->type == UNIT_TYPE_ENEMY) && (enemy_data->col_shape->stat == COLLISION_STAT_ENABLE)) {
							unit_manager_enemy_trap(enemy_data, trap_data);
						}
					}
				}
			}
		}

		if (msg.id == EVENT_MSG_COLLISION_DYNAMIC_PBvE) {
			game_event_collision_dynamic_t* msg_param = (game_event_collision_dynamic_t*)msg.param;
			if (LOWER_BIT(msg_param->obj1->group) == COLLISION_GROUP_PLAYER_BULLET) {
				if (msg_param->obj1->obj) {
					unit_player_bullet_data_t* p_bullet_data = (unit_player_bullet_data_t*)msg_param->obj1->obj;
					if ((p_bullet_data->type == UNIT_TYPE_PLAYER_BULLET) && (p_bullet_data->col_shape->stat == COLLISION_STAT_ENABLE)) {
						if (LOWER_BIT(msg_param->obj2->group) == COLLISION_GROUP_ENEMY) {
							if (msg_param->obj2->obj) {
								unit_enemy_data_t* enemy_data = (unit_enemy_data_t*)msg_param->obj2->obj;
								if ((enemy_data->type == UNIT_TYPE_ENEMY) && (enemy_data->col_shape->stat == COLLISION_STAT_ENABLE)) {
									unit_manager_enemy_get_damage_with_bullet(enemy_data, p_bullet_data);
								}
							}
						}

						// delete bullet
						unit_manager_player_bullet_set_anim_stat(p_bullet_data->id, ANIM_STAT_FLAG_DIE);
					}
				}
			}
		}
		if (msg.id == EVENT_MSG_COLLISION_DYNAMIC_EBvP) {
			game_event_collision_dynamic_t* msg_param = (game_event_collision_dynamic_t*)msg.param;
			if (LOWER_BIT(msg_param->obj1->group) == COLLISION_GROUP_ENEMY_BULLET) {
				if (msg_param->obj1->obj) {
					unit_enemy_bullet_data_t* e_bullet_data = (unit_enemy_bullet_data_t*)msg_param->obj1->obj;
					if ((e_bullet_data->type == UNIT_TYPE_ENEMY_BULLET) && (e_bullet_data->col_shape->stat == COLLISION_STAT_ENABLE)) {
						if (LOWER_BIT(msg_param->obj2->group) == COLLISION_GROUP_PLAYER) {
							if (msg_param->obj2->obj) {
								unit_player_data_t* player_data = (unit_player_data_t*)msg_param->obj2->obj;
								if ((player_data->type == UNIT_TYPE_PLAYER) && (player_data->col_shape->stat == COLLISION_STAT_ENABLE)) {
									unit_manager_player_get_damage_with_bullet(e_bullet_data);
								}
							}
						}

						// delete bullet
						unit_manager_enemy_bullet_set_anim_stat(e_bullet_data->id, ANIM_STAT_FLAG_DIE);
					}
				}
			}
		}

		if (msg.id == EVENT_MSG_COLLISION_DYNAMIC_PBvM) {
			game_event_collision_dynamic_t* msg_param = (game_event_collision_dynamic_t*)msg.param;
			if (LOWER_BIT(msg_param->obj1->group) == COLLISION_GROUP_PLAYER_BULLET) {
				if (msg_param->obj1->obj) {
					unit_player_bullet_data_t* p_bullet_data = (unit_player_bullet_data_t*)msg_param->obj1->obj;
					if ((p_bullet_data->type == UNIT_TYPE_PLAYER_BULLET) && (p_bullet_data->col_shape->stat == COLLISION_STAT_ENABLE)) {
						// delete bullet
						unit_manager_player_bullet_set_anim_stat(p_bullet_data->id, ANIM_STAT_FLAG_DIE);
					}
				}
			}
		}
		if (msg.id == EVENT_MSG_COLLISION_DYNAMIC_EBvM) {
			game_event_collision_dynamic_t* msg_param = (game_event_collision_dynamic_t*)msg.param;
			if (LOWER_BIT(msg_param->obj1->group) == COLLISION_GROUP_ENEMY_BULLET) {
				if (msg_param->obj1->obj) {
					unit_enemy_bullet_data_t* e_bullet_data = (unit_enemy_bullet_data_t*)msg_param->obj1->obj;
					if ((e_bullet_data->type == UNIT_TYPE_ENEMY_BULLET) && (e_bullet_data->col_shape->stat == COLLISION_STAT_ENABLE)) {
						// delete bullet
						unit_manager_enemy_bullet_set_anim_stat(e_bullet_data->id, ANIM_STAT_FLAG_DIE);
					}
				}
			}
		}

		if (msg.id == EVENT_MSG_COLLISION_DYNAMIC_PvE) {
			game_event_collision_dynamic_t* msg_param = (game_event_collision_dynamic_t*)msg.param;
			if (LOWER_BIT(msg_param->obj2->group) == COLLISION_GROUP_ENEMY) {
				if (msg_param->obj2->obj) {
					unit_enemy_data_t* enemy_data = (unit_enemy_data_t*)msg_param->obj2->obj;
					if ((enemy_data->type == UNIT_TYPE_ENEMY) && (enemy_data->col_shape->stat == COLLISION_STAT_ENABLE)) {
						unit_manager_player_get_damage(-unit_manager_enemy_get_bullet_strength(enemy_data->base->id));
					}
				}
			}
		}

		if (msg.id == EVENT_MSG_UNIT_PLAYER_ATTACK_CMD) {
		}

		if (msg.id == EVENT_MSG_UNIT_ENEMY_ATTACK1_CMD) { // (EVENT_MSG_UNIT_ENEMY_ATTACK_CMD)
			game_event_unit_t* msg_param = (game_event_unit_t*)msg.param;
			unit_enemy_data_t* unit_data = (unit_enemy_data_t*)msg_param->obj1;
			unit_manager_enemy_attack(unit_data, ANIM_STAT_FLAG_ATTACK1);
		}
		if (msg.id == EVENT_MSG_UNIT_ENEMY_ATTACK2_CMD) {
			game_event_unit_t* msg_param = (game_event_unit_t*)msg.param;
			unit_enemy_data_t* unit_data = (unit_enemy_data_t*)msg_param->obj1;
			unit_manager_enemy_attack(unit_data, ANIM_STAT_FLAG_ATTACK2);
		}

		if (msg.id == EVENT_MSG_UNIT_PLAYER_BULLET_DIE_CMD) {
			game_event_unit_t* msg_param = (game_event_unit_t*)msg.param;
			unit_data_t* unit_data = msg_param->obj1;

			char* unit_path = (char*)unit_data->obj;

			// big_point -> 3 point bullet
			if (strcmp(unit_path, g_player_bullet_path[UNIT_BULLET_ID_BIG_POINT]) == 0) {
				int x[3], y[3], other_face;
				float vec_x[3], vec_y[3], abs_vec = 1.0f;
				std::string point_bullet = g_player_bullet_path[UNIT_BULLET_ID_POINT];
				other_face = unit_manager_get_face_other_side(unit_data);

				int bullet_base_id = unit_manager_search_player_bullet(point_bullet);
				unit_manager_get_bullet_start_pos(unit_data, (unit_data_t*)unit_manager_get_player_bullet_base(bullet_base_id), UNIT_BULLET_TRACK_RADIAL, UNIT_BULLET_NUM_TRIPLE, other_face, x, y);
				unit_manager_get_face_velocity(vec_x, vec_y, other_face, abs_vec, UNIT_BULLET_TRACK_RADIAL, UNIT_BULLET_NUM_TRIPLE);

				for (int bi = 0; bi < 3; bi++) {
					int unit_id = unit_manager_create_player_bullet(x[bi], y[bi], vec_x[bi], vec_y[bi], other_face, bullet_base_id);
					unit_manager_player_bullet_set_anim_stat(unit_id, ANIM_STAT_FLAG_ATTACK);
					unit_manager_player_bullet_set_bullet_life_timer(unit_id, unit_manager_player_get_bullet_life_timer());
				}
			}
		}

		if (msg.id == EVENT_MSG_UNIT_ENEMY_BULLET_DIE_CMD) {
			game_event_unit_t* msg_param = (game_event_unit_t*)msg.param;
			unit_enemy_bullet_data_t* unit_data = (unit_enemy_bullet_data_t*)msg_param->obj1;

			char* unit_path = (char*)unit_data->obj;

			// big_point -> 3 point bullet
			if (strcmp(unit_path, g_enemy_bullet_path[UNIT_BULLET_ID_BIG_POINT]) == 0) {
				int x[3], y[3], other_face;
				float vec_x[3], vec_y[3], abs_vec = 1.0f;
				std::string point_bullet = g_enemy_bullet_path[UNIT_BULLET_ID_POINT];
				other_face = unit_manager_get_face_other_side((unit_data_t*)unit_data);

				int bullet_base_id = unit_manager_search_enemy_bullet(point_bullet);
				unit_manager_get_bullet_start_pos((unit_data_t*)unit_data, (unit_data_t*)unit_manager_get_enemy_bullet_base(bullet_base_id), UNIT_BULLET_TRACK_RADIAL, UNIT_BULLET_NUM_TRIPLE, other_face, x, y);
				unit_manager_get_face_velocity(vec_x, vec_y, other_face, abs_vec, UNIT_BULLET_TRACK_RADIAL, UNIT_BULLET_NUM_TRIPLE);

				for (int bi = 0; bi < 3; bi++) {
					int unit_id = unit_manager_create_enemy_bullet(x[bi], y[bi], vec_x[bi], vec_y[bi], other_face, unit_data->owner_base_id, bullet_base_id);
					unit_manager_enemy_bullet_set_anim_stat(unit_id, ANIM_STAT_FLAG_ATTACK);
					unit_manager_enemy_bullet_set_hp(unit_id, unit_manager_enemy_get_bullet_strength(unit_data->owner_base_id));
					unit_manager_enemy_bullet_set_bullet_life_timer(unit_id, unit_manager_enemy_get_bullet_life_timer(unit_data->owner_base_id));
				}
			}
		}

		if (msg.id == EVENT_MSG_UNIT_ITEMS_SPAWN_CMD) {
			game_event_unit_t* msg_param = (game_event_unit_t*)msg.param;
			unit_items_data_t* unit_data = (unit_items_data_t*)msg_param->obj1;
			if (unit_data->group == UNIT_ITEM_GROUP_BOM) {
				// activate bom
				unit_manager_items_set_anim_stat(unit_data->id, ANIM_STAT_FLAG_ATTACK);
			}
		}

		if (msg.id == EVENT_MSG_UNIT_ITEMS_DIE_CMD) {
			game_event_unit_t* msg_param = (game_event_unit_t*)msg.param;
			unit_items_data_t* unit_data = (unit_items_data_t*)msg_param->obj1;
			if (unit_data->group == UNIT_ITEM_GROUP_BOM) {
				unit_manager_items_fire_bom(unit_data);
			}
		}

		if (msg.id == EVENT_MSG_UNIT_TRAP_SPAWN_CMD) {
			game_event_unit_t* msg_param = (game_event_unit_t*)msg.param;
			unit_trap_data_t* unit_data = (unit_trap_data_t*)msg_param->obj1;
			if ((unit_data->group == UNIT_TRAP_GROUP_GATE) && (unit_data->sub_id & UNIT_TRAP_GATE_ID_GO_NEXT)) {
				map_manager_set_door_filter(unit_data->sub_id);
			}
		}

		if (msg.id == EVENT_MSG_UNIT_EFFECT_DIE_CMD) {
		}
	}

	// after event process
	if (g_stage_data->result == STAGE_RESULT_LOSE) {
		sound_manager_stop(SOUND_MANAGER_CH_MUSIC);
		sound_manager_play(resource_manager_getChunkFromPath("music/lose.ogg"), SOUND_MANAGER_CH_MUSIC);
	}
}

// init draw items
static void tex_info_init()
{
}

void scene_play_stage_init() {
	// set stat
	scene_stat = SCENE_STAT_NONE;

	// set fuctions
	scene_func.pre_event      = &pre_event;
	scene_func.key_event      = &key_event;
	scene_func.main_event     = &main_event;
	scene_func.pre_draw       = &pre_draw;
	scene_func.draw           = &draw;
	scene_func.after_draw     = &after_draw;
	scene_func.pre_load_event = &pre_load_event;
	scene_func.load_event     = &load_event;
	scene_func.unload_event   = &unload_event;
	scene_func.get_stat_event = &get_stat_event;
	scene_func.set_stat_event = &set_stat_event;

	reload_player = false;
}

SceneManagerFunc* scene_play_stage_get_func() {
	return &scene_func;
}

void scene_play_stage_set_stage_id(std::string& id) {
	stage_id = id;
}

void scene_play_stage_set_player(std::string& path, bool reload_on) {
	player_path = path;
	reload_player = reload_on;
}

void scene_play_next_stage() {
	game_next_stage_dark_alpha = 0;
	sound_manager_stop(SOUND_MANAGER_CH_MUSIC);
	sound_manager_play(resource_manager_getChunkFromPath("music/win.ogg"), SOUND_MANAGER_CH_MUSIC);
	stage_manager_set_next_load(STAGE_NEXT_LOAD_ON);
}

void scene_play_next_section(int go_next_id) {
	if (g_stage_data->section_stat == SECTION_STAT_NEXT_WAIT) return;

	// backup current section
	map_manager_backup_to_section_map();

	// backup current items
	stage_manager_delete_all_stock_item();
	unit_manager_items_register_item_stock();

	// set next stage_map_index (g_stage_data->current_stage_map_index)
	int stage_map_index = g_stage_data->current_stage_map_index;
	int stage_map_index_x = stage_map_index % STAGE_MAP_WIDTH_NUM;
	int stage_map_index_y = stage_map_index / STAGE_MAP_WIDTH_NUM;
	int section_id = STAGE_MAP_ID_IGNORE;
	if (go_next_id == UNIT_TRAP_GATE_ID_GO_NEXT_N) {
		if ((0 <= stage_map_index_y - 1) && (g_stage_data->stage_map[stage_map_index - STAGE_MAP_WIDTH_NUM].section_id != STAGE_MAP_ID_IGNORE)) {
			g_stage_data->current_stage_map_index = stage_map_index - STAGE_MAP_WIDTH_NUM;

			// set South side
			g_stage_data->section_start_x = (MAP_WIDTH_NUM_MAX / 2) * g_tile_width;
			g_stage_data->section_start_y = (MAP_HEIGHT_NUM_MAX - 1) * g_tile_height - g_tile_height;
		}
		else {
			LOG_ERROR("ERROR: scene_play_next_section(N) not found next section.");
		}
	}
	else if (go_next_id == UNIT_TRAP_GATE_ID_GO_NEXT_S) {
		if ((stage_map_index_y + 1 < STAGE_MAP_HEIGHT_NUM) && (g_stage_data->stage_map[stage_map_index + STAGE_MAP_WIDTH_NUM].section_id != STAGE_MAP_ID_IGNORE)) {
			g_stage_data->current_stage_map_index = stage_map_index + STAGE_MAP_WIDTH_NUM;

			// set North side
			g_stage_data->section_start_x = (MAP_WIDTH_NUM_MAX / 2) * g_tile_width;
			g_stage_data->section_start_y = g_tile_height;
		}
		else {
			LOG_ERROR("ERROR: scene_play_next_section(S) not found next section.");
		}
	}
	else if (go_next_id == UNIT_TRAP_GATE_ID_GO_NEXT_W) {
		if ((0 <= stage_map_index_x - 1) && (g_stage_data->stage_map[stage_map_index - 1].section_id != STAGE_MAP_ID_IGNORE)) {
			g_stage_data->current_stage_map_index = stage_map_index - 1;

			// set East side
			g_stage_data->section_start_x = (MAP_WIDTH_NUM_MAX - 1) * g_tile_width - g_tile_width;
			g_stage_data->section_start_y = (MAP_HEIGHT_NUM_MAX / 2) * g_tile_height;
		}
		else {
			LOG_ERROR("ERROR: scene_play_next_section(W) not found next section.");
		}
	}
	else if (go_next_id == UNIT_TRAP_GATE_ID_GO_NEXT_E) {
		if ((stage_map_index_x + 1 < STAGE_MAP_WIDTH_NUM) && (g_stage_data->stage_map[stage_map_index + 1].section_id != STAGE_MAP_ID_IGNORE)) {
			g_stage_data->current_stage_map_index = stage_map_index + 1;

			// set West side
			g_stage_data->section_start_x = g_tile_width;
			g_stage_data->section_start_y = (MAP_HEIGHT_NUM_MAX / 2) * g_tile_height;
		}
		else {
			LOG_ERROR("ERROR: scene_play_next_section(E) not found next section.");
		}
	}

	// set next section_index
	section_id = g_stage_data->stage_map[g_stage_data->current_stage_map_index].section_id;
	g_stage_data->current_section_data = g_stage_data->section_list[section_id];
	g_stage_data->current_section_index = section_id;

	g_stage_data->section_stat = SECTION_STAT_NEXT_WAIT;
}

void scene_play_stage_set_lose() {
	dialog_message_reset("You Lose... (Exit to TopMenu)", NULL, dialog_message_ok, DIALOG_MSG_TYPE_OK_ONLY);
	dialog_message_set_enable(true);
}
