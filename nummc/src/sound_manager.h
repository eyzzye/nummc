#pragma once
#include "game_common.h"
#include "resource_manager.h"

#define SOUND_MANAGER_CH_AUTO   (-1)
#define SOUND_MANAGER_CH_MAIN1  0
#define SOUND_MANAGER_CH_MAIN2  1
#define SOUND_MANAGER_CH_SUB1   2
#define SOUND_MANAGER_CH_SUB2   3
#define SOUND_MANAGER_CH_SUB3   4
#define SOUND_MANAGER_CH_SFX1   5
#define SOUND_MANAGER_CH_SFX2   6
#define SOUND_MANAGER_CH_MUSIC  7

extern int sound_manager_init_track_volume();
extern int sound_manager_init();
extern void sound_manager_close();
extern void sound_manager_play(ResourceChunk* res_chunk, int channel= SOUND_MANAGER_CH_AUTO, int loop = 0);
extern void sound_manager_stop(int channel);
