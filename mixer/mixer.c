#include "mixer.h"

typedef struct ChanelInfo {
	Mix_Chunk *chunk;
	VMUINT pos;
	char play;
};

static struct ChanelInfo channels[16];

static int sample_rate = 0;


VMINT mixer_init(VMINT num_chanels, VMBOOL isStereo, VMUINT8 bitPerSample, vm_bitstream_sample_freq_enum sampleFreq) {
	int res = bitstream_open(isStereo, bitPerSample, sampleFreq);
	if (res)
		return res;

	for (int i = 0; i < 16; ++i) {
		channels[i].chunk = 0;
		channels[i].play = 0;
	}

	bitstream_start(0, 0, 6);

	sample_rate = bitstream_get_sample_rate();

	return 0;
}

void mixer_setup_channel(VMINT chnanel, Mix_Chunk* chunk, VMBOOL play) {
	channels[chnanel].chunk = chunk;
	channels[chnanel].pos = 0;
	channels[chnanel].play = play;
}

void mixer_stop(VMINT chnanel) {
	channels[chnanel].play = 0;
}

VMBOOL mixer_is_playing(VMINT chnanel) {
	return channels[chnanel].play;
}



static short buf_samples[4086];
void mixer_update() {
	int free_buf_size = bitstream_get_free_buffer_size();
	int all_buf_size = bitstream_get_buffer_size();
	int used_buf_size = all_buf_size - free_buf_size;

	int play_buf_size = sample_rate * 2 / 8 - used_buf_size;

	if (play_buf_size > free_buf_size)
		play_buf_size = free_buf_size;

	play_buf_size &= ~0b11l;

	if (play_buf_size > 4086)
		play_buf_size = 4086;

	if (play_buf_size <= 0)
		return;

	memset(buf_samples, 0, play_buf_size);

	char first = 1;

	for (int i = 0; i < 16; ++i) 
		if(channels[i].play){
			struct ChanelInfo* channel = &channels[i];

			int to_play = channel->chunk->alen - channel->pos;
			if (to_play > play_buf_size)
				to_play = play_buf_size;

			if (first) {
				memcpy(buf_samples, channel->chunk->abuf + channel->pos, to_play);
				first = 0;
			}

			channel->pos += to_play;
			if (channel->pos >= channel->chunk->alen)
				channel->play = 0;
		}

	VMUINT writen;
	bitstream_put_data((char*)buf_samples, play_buf_size, &writen);
}

void mixer_close() {
	bitstream_close();
}