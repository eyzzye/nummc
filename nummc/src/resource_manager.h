#pragma once
#include <vector>
#include <string>
#include "game_common.h"

#define RESOURCE_MANAGER_TYPE_NONE    0
#define RESOURCE_MANAGER_TYPE_STATIC  1
#define RESOURCE_MANAGER_TYPE_DYNAMIC 2

typedef struct _ResourceImg ResourceImg;
typedef struct _ResourceMusic ResourceMusic;
typedef struct _ResourceChunk ResourceChunk;
typedef struct _ResourceProfile ResourceProfile;

struct _ResourceImg {
	int type;
	std::string path;
	SDL_Texture* tex;
};

struct _ResourceMusic {
	int type;
	std::string path;
	Mix_Music* music;
};

struct _ResourceChunk {
	int type;
	std::string path;
	Mix_Chunk* chunk;
};

struct _ResourceProfile {
	const char* name;
	const char* unit_path;
	const char* icon_img_path;
	const char* portrait_img_path;
	const char* opening_path;
	const char* ending_path;
};

#define RESOURCE_MANAGER_PROFILE_LIST_SIZE  3
extern ResourceProfile g_resource_manager_profile[RESOURCE_MANAGER_PROFILE_LIST_SIZE];

extern void resource_manager_init();
extern void resource_manager_unload();
extern int resource_manager_load_dat(std::string path);
extern ResourceImg* resource_manager_load_img(std::string path, int type = RESOURCE_MANAGER_TYPE_DYNAMIC);
extern ResourceImg* resource_manager_getTextureFromPath(std::string path);
extern ResourceImg* resource_manager_load_font(std::string message, int type = RESOURCE_MANAGER_TYPE_DYNAMIC);
extern ResourceImg* resource_manager_getFontTextureFromPath(std::string message);
extern ResourceMusic* resource_manager_load_music(std::string path, int type = RESOURCE_MANAGER_TYPE_DYNAMIC);
extern ResourceMusic* resource_manager_getMusicFromPath(std::string path);
extern ResourceChunk* resource_manager_load_chunk(std::string path, int type = RESOURCE_MANAGER_TYPE_DYNAMIC);
extern ResourceChunk* resource_manager_getChunkFromPath(std::string path);
