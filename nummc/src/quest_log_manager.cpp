#include "game_common.h"
#include "quest_log_manager.h"

#include "resource_manager.h"
#include "game_window.h"
#include "game_utils.h"
#include "game_log.h"
#include "gui_common.h"
#include "unit_manager.h"
#include "inventory_manager.h"
#include "stage_manager.h"

typedef struct _quest_log_data_t quest_log_data_t;

#define QUEST_LOG_MESSAGE_SIZE 128
struct _quest_log_data_t {
	int type;
	int id;
	node_data_t* prev;
	node_data_t* next;

	char message[QUEST_LOG_MESSAGE_SIZE];
	int message_length;
	int regist_timer;
};

#define QUEST_LOG_LIST_SIZE (20)
static tex_info_t tex_info_quest_log_list[QUEST_LOG_LIST_SIZE];
static node_buffer_info_t quest_log_buffer_info;

static int start_pos_list[QUEST_LOG_LIST_SIZE][2]; // x, y

static int frame_points[4] = { 964, 4, 1272, 652 };
#define HORIZONTAL_LINES_SIZE  2
#define VERTICAL_LINES_SIZE    2
static SDL_Rect horizontal_lines[HORIZONTAL_LINES_SIZE];
static SDL_Rect vertical_lines[VERTICAL_LINES_SIZE];
static SDL_Rect quest_log_background;

static void tex_info_init();
static void all_message_reset();

int quest_log_manager_init()
{
	memset(tex_info_quest_log_list, 0, sizeof(tex_info_quest_log_list));
	game_utils_node_init(&quest_log_buffer_info, (int)sizeof(quest_log_data_t));

	tex_info_init();
	return 0;
}

void quest_log_manager_unload()
{
	if (quest_log_buffer_info.used_buffer_size > 0) {
		node_data_t* node = quest_log_buffer_info.start_node;
		while (node != NULL) {
			node_data_t* del_node = node;
			node = node->next;
			game_utils_node_delete(del_node, &quest_log_buffer_info);
		}
	}
}

void quest_log_manager_reset()
{
	int line_size = VIEW_SCALE(4);

	horizontal_lines[0] = {	VIEW_SCALE_X(frame_points[0]), VIEW_SCALE_Y(frame_points[1]),
		VIEW_SCALE_X(frame_points[2]) - VIEW_SCALE_X(frame_points[0]) + line_size, line_size };
	horizontal_lines[1] = { VIEW_SCALE_X(frame_points[0]), VIEW_SCALE_Y(frame_points[3]),
		VIEW_SCALE_X(frame_points[2]) - VIEW_SCALE_X(frame_points[0]) + line_size, line_size };

	vertical_lines[0] = { VIEW_SCALE_X(frame_points[0]), VIEW_SCALE_Y(frame_points[1]),
		line_size, VIEW_SCALE_Y(frame_points[3]) - VIEW_SCALE_Y(frame_points[1]) + line_size };
	vertical_lines[1] = { VIEW_SCALE_X(frame_points[2]), VIEW_SCALE_Y(frame_points[1]),
		line_size, VIEW_SCALE_Y(frame_points[3]) - VIEW_SCALE_Y(frame_points[1]) + line_size };

	quest_log_background = { VIEW_SCALE_X(frame_points[0]), VIEW_SCALE_Y(frame_points[1]),
		VIEW_SCALE(frame_points[2] - frame_points[0] + 1), VIEW_SCALE(frame_points[3] - frame_points[1] + 1) };

	all_message_reset();
}

static void all_message_reset()
{
	int w, h, w_pos, h_pos;

	// reset position
	int log_count = 0;
	quest_log_data_t* log_data = (quest_log_data_t*)quest_log_buffer_info.start_node;
	while (log_data != NULL) {
		w = tex_info_quest_log_list[log_data->id].src_rect.w;
		h = tex_info_quest_log_list[log_data->id].src_rect.h;
		w_pos = start_pos_list[log_count][0];
		h_pos = start_pos_list[log_count][1];
		tex_info_quest_log_list[log_data->id].dst_rect = VIEW_SCALE_RECT(w_pos, h_pos, w / 2, h / 2);
		tex_info_quest_log_list[log_data->id].dst_rect_base = { w_pos, h_pos, w / 2, h / 2 };

		log_data = (quest_log_data_t*)log_data->next;
		log_count += 1;
	}
}

static void tex_info_init()
{
	// default screen size
	int y_pos = 4 + 4 + 4;
	for (int i = 0; i < QUEST_LOG_LIST_SIZE; i++) {
		start_pos_list[i][0] = 964 + 4 + 4;
		start_pos_list[i][1] = y_pos;
		y_pos += 32;
	}
}

void quest_log_manager_message(const char* message_fmt, ...)
{
	char buff[32];
	va_list args;
	va_start(args, message_fmt);
	vsnprintf_s(buff, 31, message_fmt, args);
	va_end(args);

	quest_log_manager_set_new_message((char*)buff, (int)strlen(buff));
}

void quest_log_manager_set_new_message(char* message, int message_length, int regist_timer)
{
	quest_log_data_t* new_node = NULL;
	int new_id = 0;
	if (quest_log_buffer_info.used_buffer_size >= QUEST_LOG_LIST_SIZE) {
		// delete start node
		new_id = ((quest_log_data_t*)quest_log_buffer_info.start_node)->id;
		game_utils_node_delete(quest_log_buffer_info.start_node, &quest_log_buffer_info);
	}
	else {
		new_id = quest_log_buffer_info.used_buffer_size;
	}
	new_node = (quest_log_data_t*)game_utils_node_new(&quest_log_buffer_info);

	if (new_node != NULL) {
		strcpy_s(new_node->message, (QUEST_LOG_MESSAGE_SIZE - 1), message);
		new_node->message_length = message_length;
		new_node->regist_timer = regist_timer;
		new_node->id = new_id;

		// create message texture
		int w, h;
		tex_info_quest_log_list[new_node->id].res_img = resource_manager_getFontTextureFromPath(message);
		int ret = GUI_QueryTexture(tex_info_quest_log_list[new_node->id].res_img, NULL, NULL, &w, &h);
		if (ret == 0) {
			tex_info_quest_log_list[new_node->id].src_rect = { 0, 0, w, h };
		}

		// reset position
		all_message_reset();
	}
}

void quest_log_manager_update()
{
}

void quest_log_manager_display()
{
	// draw frame
	SDL_SetRenderDrawColor(g_ren, 255, 255, 255, 255);
	SDL_RenderFillRect(g_ren, &quest_log_background);
	SDL_SetRenderDrawColor(g_ren, 0, 0, 0, 255);
	SDL_RenderFillRects(g_ren, horizontal_lines, (int)LENGTH_OF(horizontal_lines));
	SDL_RenderFillRects(g_ren, vertical_lines, (int)LENGTH_OF(vertical_lines));

	// draw log message
	int log_count = 0;
	quest_log_data_t* log_data = (quest_log_data_t*)quest_log_buffer_info.start_node;
	while (log_data != NULL) {
		GUI_tex_info_draw(&tex_info_quest_log_list[log_data->id]);
		log_data = (quest_log_data_t*)log_data->next;
		log_count += 1;
	}
}
