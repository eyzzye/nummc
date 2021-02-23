#include "game_common.h"
#include "sound_manager.h"

#include "resource_manager.h"
#include "game_save.h"
#include "game_log.h"

// track data
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

// sound buffer
#define SOUND_STACK_SIZE  16
typedef struct _sound_stack_t sound_stack_t;
struct _sound_stack_t {
	ResourceChunk* res_chunk;
	int channel;
	int loop;
};
static sound_stack_t sound_stack[SOUND_STACK_SIZE];
static int sound_stack_num;

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

	// clear buffer
	memset(track_data, 0, sizeof(track_data));
	memset(sound_stack, 0, sizeof(sound_stack));
	sound_stack_num = 0;

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

void sound_manager_set(ResourceChunk* res_chunk, int channel, int loop)
{
	if ((res_chunk == NULL) || (sound_stack_num >= SOUND_STACK_SIZE)) return;

	// check overwrite
	for (int i = 0; i < sound_stack_num; i++) {
		if ((sound_stack[i].res_chunk == res_chunk)
			&& (sound_stack[i].channel == channel)
			&& (sound_stack[i].loop == loop))
		{
			// already stack
			return;
		}
	}

	// stack new_chunk
	sound_stack[sound_stack_num].res_chunk = res_chunk;
	sound_stack[sound_stack_num].channel = channel;
	sound_stack[sound_stack_num].loop = loop;
	sound_stack_num += 1;
}

int sound_manager_play(ResourceChunk* res_chunk, int channel, int loop)
{
	if (res_chunk == NULL) return (-1);
	if (channel != SOUND_MANAGER_CH_AUTO) track_data[channel].chunk = res_chunk->chunk;
	int play_channel = Mix_PlayChannel(channel, res_chunk->chunk, loop);

	//LOG_DEBUG("SND:%d %s\n", play_channel, res_chunk->path.c_str());
	return play_channel;
}

void sound_manager_play_all()
{
	if (sound_stack_num <= 0) return;

	bool played_channel[SOUND_MANAGER_CH_END] = { false };

	// fixed channel
	for (int i = 0; i < sound_stack_num; i++) {
		if ((sound_stack[i].channel >= 0) && (played_channel[sound_stack[i].channel] == false)) {
			int play_channel = sound_manager_play(sound_stack[i].res_chunk, sound_stack[i].channel, sound_stack[i].loop);
			if (play_channel >= 0) {
				played_channel[play_channel] = true;
			}
		}
	}

	// auto channel
	for (int i = 0; i < sound_stack_num; i++) {
		if (sound_stack[i].channel < 0) {
			int play_channel = sound_manager_play(sound_stack[i].res_chunk, sound_stack[i].channel, sound_stack[i].loop);
			if (play_channel >= 0) {
				played_channel[play_channel] = true;
			}
		}
	}

	// clear buffer
	memset(sound_stack, 0, sizeof(sound_stack));
	sound_stack_num = 0;
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
