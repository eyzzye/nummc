#pragma once
#include "game_common.h"
#include "resource_manager.h"

#define STORY_STAT_NONE       0
#define STORY_STAT_IDLE       1
#define STORY_STAT_ACTIVE     2
#define STORY_STAT_END        3

typedef struct _auto_text_data_t auto_text_data_t;
typedef struct _story_data_t story_data_t;

struct _auto_text_data_t {
	int type;        // NONE:0
	int id;          // index
	node_data_t* prev;
	node_data_t* next;

	char auto_text[GAME_UTILS_STRING_CHAR_BUF_SIZE];
};

struct _story_data_t {
	char* id;
	int stat;

	char* story_path;
	ResourceImg* res_img;
	ResourceChunk* res_chunk;

	int auto_text_time;
	node_buffer_info_t* auto_text_list;
};

extern void story_manager_init();
extern void story_manager_unload();
extern void story_manager_set_stat(int stat);
extern int story_manager_load(char* path);

extern story_data_t* g_story_data;
