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

#define QUEST_LOG_MESSAGE_SIZE (256 - 5)
struct _quest_log_data_t {
	int id;
	quest_log_data_t* prev;
	quest_log_data_t* next;

	char message[QUEST_LOG_MESSAGE_SIZE];
	int message_length;
	int regist_timer;
};

#define QUEST_LOG_LIST_SIZE (20)
static quest_log_data_t quest_log_list[QUEST_LOG_LIST_SIZE];
static tex_info_t tex_info_quest_log_list[QUEST_LOG_LIST_SIZE];
static int quest_log_start_idx;
static int quest_log_end_idx;
static int quest_log_count;

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
	memset(quest_log_list, 0, sizeof(quest_log_list));
	memset(tex_info_quest_log_list, 0, sizeof(tex_info_quest_log_list));
	quest_log_start_idx = 0;
	quest_log_end_idx = 0;
	quest_log_count = 0;

	tex_info_init();
	return 0;
}

void quest_log_manager_unload()
{
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
	quest_log_data_t* log_data = &quest_log_list[quest_log_start_idx];
	while ((log_data != NULL) && (log_count < quest_log_count)) {
		w = tex_info_quest_log_list[log_data->id].src_rect.w;
		h = tex_info_quest_log_list[log_data->id].src_rect.h;
		w_pos = start_pos_list[log_count][0];
		h_pos = start_pos_list[log_count][1];
		tex_info_quest_log_list[log_data->id].dst_rect = VIEW_SCALE_RECT(w_pos, h_pos, w / 2, h / 2);
		tex_info_quest_log_list[log_data->id].dst_rect_base = { w_pos, h_pos, w / 2, h / 2 };

		log_data = log_data->next;
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
	int new_index = -1;
	if (quest_log_count >= QUEST_LOG_LIST_SIZE) {
		// delete head log
		new_index = quest_log_start_idx;
		quest_log_manager_clear_message(quest_log_start_idx);
	}
	else {
		// search empty log
		for (int i = 0; i < QUEST_LOG_LIST_SIZE; i++) {
			if (quest_log_list[i].message_length == 0) {
				new_index = i;
				break;
			}
		}
	}

	if (new_index != -1) {
		// regist end index
		quest_log_list[new_index].id = new_index;
		quest_log_list[new_index].prev = &quest_log_list[quest_log_end_idx];
		quest_log_list[new_index].next = NULL;
		quest_log_list[quest_log_end_idx].next = &quest_log_list[new_index];
		quest_log_end_idx = new_index;

		memcpy(quest_log_list[new_index].message, message, message_length);
		quest_log_list[new_index].message_length = message_length;
		quest_log_list[new_index].regist_timer = regist_timer;
		quest_log_count += 1;

		// create message texture
		int w, h;
		std::string tex_message = message;
		tex_info_quest_log_list[new_index].tex = resource_manager_getFontTextureFromPath(tex_message);
		int ret = SDL_QueryTexture(tex_info_quest_log_list[new_index].tex, NULL, NULL, &w, &h);
		if (ret == 0) {
			tex_info_quest_log_list[new_index].src_rect = { 0, 0, w, h };
		}

		// reset position
		all_message_reset();
	}
}

void quest_log_manager_clear_message(int quest_log_index)
{
	if (quest_log_list[quest_log_index].next) {
		quest_log_list[quest_log_index].next->prev = NULL;
		if (quest_log_start_idx == quest_log_index) {
			quest_log_start_idx = quest_log_list[quest_log_index].next->id;
		}

		if (quest_log_list[quest_log_index].prev) {
			quest_log_list[quest_log_index].prev->next = quest_log_list[quest_log_index].next;
		}
	}
	else if (quest_log_list[quest_log_index].prev) {
		quest_log_list[quest_log_index].prev->next = NULL;
		if (quest_log_end_idx == quest_log_index) {
			quest_log_end_idx = quest_log_list[quest_log_index].prev->id;
		}
	}

	//
	// delete message texture
	//

	// clear log
	memset(&quest_log_list[quest_log_index], 0, sizeof(quest_log_data_t));
	quest_log_count -= 1;
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
	quest_log_data_t* log_data = &quest_log_list[quest_log_start_idx];
	while ((log_data != NULL) && (log_count < QUEST_LOG_LIST_SIZE)) {
		GUI_tex_info_draw(&tex_info_quest_log_list[log_data->id]);
		log_data = log_data->next;
		log_count += 1;
	}
}
