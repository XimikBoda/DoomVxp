#include "mixer.h"

typedef struct ChanelInfo {
	Mix_Chunk *chunk;
	VMUINT pos;
	char play;
	VMUINT8 left_planning;
	VMUINT8 right_planning;
};

static struct ChanelInfo channels[16];

static int sample_rate = 0;

static void bitstream_handle(enum bitstream_events event) {
	if (event != BITSTREAM_EVENT_ERROR){
		mixer_update();
		//if (event == BITSTREAM_EVENT_BUFFER_UNDERFLOW)
		//	injected_bitstream_resume();
	}
}


VMINT mixer_init(VMINT num_chanels, VMBOOL isStereo, VMUINT8 bitPerSample, vm_bitstream_sample_freq_enum sampleFreq) {
	int res = bitstream_open(isStereo, bitPerSample, sampleFreq, bitstream_handle);
	if (res)
		return res;

	for (int i = 0; i < 16; ++i) {
		channels[i].chunk = 0;
		channels[i].play = 0;
		channels[i].left_planning = 255;
		channels[i].right_planning = 255;
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

void mixer_set_planing(VMINT chnanel, VMUINT8 left, VMUINT8 right) {
	channels[chnanel].left_planning = left;
	channels[chnanel].right_planning = right;
}

void mixer_stop(VMINT chnanel) {
	channels[chnanel].play = 0;
}

VMBOOL mixer_is_playing(VMINT chnanel) {
	return channels[chnanel].play;
}

static void mix(short buf_a[], short buf_b[], int samples, VMUINT8 left, VMUINT8 right) {
	short* dst = buf_a;
	short* src = buf_b;

	int vol_l = left;
	int vol_r = right;

	for (int i = 0; i < samples / 2; ++i) {
		// Left chanel
		int val = *dst;  
		int inc = (*src++) * vol_l;
		inc >>= 8;

		val += inc;

		if (val > 32767) val = 32767;
		else if (val < -32768) val = -32768;

		// Right chanel
		*dst++ = (short)val;
		val = *dst;
		inc = (*src++) * vol_r;
		inc >>= 8;

		val += inc;

		if (val > 32767) val = 32767;
		else if (val < -32768) val = -32768;

		*dst++ = (short)val;
	}
}

static short buf_samples[8*1024];
void mixer_update() {
	int free_buf_size = bitstream_get_free_buffer_size();

	int play_buf_size = free_buf_size;

	if (play_buf_size > free_buf_size)
		play_buf_size = free_buf_size;

	play_buf_size &= ~0b11l;

	if (play_buf_size > sizeof(buf_samples))
		play_buf_size = sizeof(buf_samples);


	if (play_buf_size > 0) {
		memset(buf_samples, 0, play_buf_size);

		for (int i = 0; i < 16; ++i)
			if (channels[i].play) {
				struct ChanelInfo* channel = &channels[i];

				int to_play = channel->chunk->alen - channel->pos;
				if (to_play > play_buf_size)
					to_play = play_buf_size;

				mix(buf_samples, channel->chunk->abuf + channel->pos, to_play / 2,
					channel->left_planning, channel->right_planning);

				channel->pos += to_play;
				if (channel->pos >= channel->chunk->alen)
					channel->play = 0;
			}
	}
	VMUINT writen;
	bitstream_put_data((char*)buf_samples, play_buf_size, &writen);
}

void mixer_close() {
	bitstream_close();
}