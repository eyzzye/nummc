#include "game_common.h"
#include "sound_manager.h"

#include "resource_manager.h"
#include "game_save.h"

#define TRACK_NUM 8
typedef struct _track_data_t track_data_t;
struct _track_data_t {
	Mix_Chunk* chunk;
	Uint32 len;
	Uint8 volume;
	Uint8 pan_left;
	Uint8 pan_right;
};
static track_data_t track_data[TRACK_NUM];

static void callback_mix_func(void* udata, Uint8* stream, int len);
static void callback_channel_finished_func(int channel);

int sound_manager_init_track_volume()
{
	int volume;
	game_save_get_config_sfx_volume(&volume);
	for (int i = 0; i < TRACK_NUM - 1; i++) {
		track_data[i].volume = volume * 128 / 100;
		Mix_Volume(i, track_data[i].volume);
	}

	// music channel
	game_save_get_config_music_volume(&volume);
	track_data[TRACK_NUM - 1].volume = volume * 128 / 100;
	Mix_Volume(TRACK_NUM - 1, track_data[TRACK_NUM - 1].volume);

	return 0;
}

int sound_manager_init()
{
	Mix_SetPostMix(callback_mix_func, NULL);
	Mix_ChannelFinished(callback_channel_finished_func);

	memset(track_data, 0, sizeof(track_data));
	sound_manager_init_track_volume();
	return 0;
}

void sound_manager_close()
{
	Mix_HaltMusic();
	for (int i = 0; i < TRACK_NUM; i++) {
		Mix_HaltChannel(i);
	}
}

void sound_manager_play(ResourceChunk* res_chunk, int channel, int loop)
{
	if (res_chunk == NULL) return;
	if (channel != SOUND_MANAGER_CH_AUTO) track_data[channel].chunk = res_chunk->chunk;
	Mix_PlayChannel(channel, res_chunk->chunk, loop);
}

void sound_manager_stop(int channel)
{
	Mix_HaltChannel(channel);
	track_data[channel].chunk = NULL;
}

static void callback_mix_func(void* udata, Uint8* stream, int len)
{
	for (int i = 0; i < TRACK_NUM; i++) {
		if (track_data[i].chunk != NULL) track_data[i].len += len;
	}
}

static void callback_channel_finished_func(int channel)
{
	track_data[channel].len = 0;
}
