#include <memory>
#include "game_common.h"
#include "game_event.h"

#include "game_log.h"

static game_event_t event_que[EVENT_MESSAGE_MAX];
static game_event_param_t event_param[EVENT_MESSAGE_MAX];
static int event_push_index;  // push pos
static int event_pop_index;   // pop pos
static int event_count;

int game_event_init()
{
	memset(event_que, 0, sizeof(game_event_t) * EVENT_MESSAGE_MAX);
	memset(event_param, 0, sizeof(game_event_param_t) * EVENT_MESSAGE_MAX);
	event_push_index = 0;
	event_pop_index = 0;
	event_count = 0;
	return 0;
}

void game_event_clear()
{
	// don't need to clear
#if 0
	for (int i = 0; i < EVENT_MESSAGE_MAX; i++) {
		if (event_que[i].id) {
			event_que[i].id = 0;
			//if (event_que[i].param) {
			//	delete event_que[i].param;
			//	event_que[i].param = NULL;
			//}
		}
	}
#endif
}

game_event_param_t* game_event_get_new_param()
{
	if (event_count >= EVENT_MESSAGE_MAX) {
		LOG_ERROR("Error: game_event_get_new_param() stack over flow \n");
		return NULL;
	}
	memset(&event_param[event_push_index], 0, sizeof(game_event_param_t));
	return &event_param[event_push_index];
}

int game_event_push(game_event_t* msg)
{
	if (event_count >= EVENT_MESSAGE_MAX) {
		LOG_ERROR("Error: event_push() stack over flow \n");
		return -1;
	}

	memcpy(&event_que[event_push_index], msg, sizeof(game_event_t));
	event_count++;
	event_push_index++;
	if (event_push_index >= EVENT_MESSAGE_MAX) {
		event_push_index = 0;
	}
	return 0;
}

int game_event_pop(game_event_t* msg)
{
	if (event_count <= 0) {
		msg->id = 0;
		return msg->id;
	}

	memcpy(msg, &event_que[event_pop_index], sizeof(game_event_t));
	memset(&event_que[event_pop_index], 0, sizeof(game_event_t));
	event_count--;
	event_pop_index++;
	if (event_pop_index >= EVENT_MESSAGE_MAX) {
		event_pop_index = 0;
	}
	return msg->id;
}
