#pragma once
#include <vector>
#include <string>
#include "game_common.h"

#define STORY_STAT_NONE       0
#define STORY_STAT_IDLE       1
#define STORY_STAT_ACTIVE     2
#define STORY_STAT_END        3

typedef struct _story_data_t story_data_t;
struct _story_data_t {
	std::string id;
	int stat;

	std::string story_path;
	SDL_Texture* tex;
	Mix_Chunk* bgm_chunk;

	int auto_text_time;
	std::vector<std::string> auto_text_list;
};

extern void story_manager_init();
extern void story_manager_unload();
extern void story_manager_set_stat(int stat);
extern int story_manager_load(std::string path);

extern story_data_t* g_story_data;
