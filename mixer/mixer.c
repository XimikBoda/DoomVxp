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


VMINT mixer_init(VMINT num_chanels, VMBOOL isStereo, VMUINT8 bitPerSample, vm_bitstream_sample_freq_enum sampleFreq) {
	int res = bitstream_open(isStereo, bitPerSample, sampleFreq);
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
	for (int i = 0; i < samples; ++i) {
		int a = buf_a[i];
		int b = buf_b[i];
		int m;

		b = ((b * (i & 1 ? right : left)) >> 8);

		// Make both samples unsigned (0..65535)
		a += 32768;
		b += 32768;

		// Pick the equation
		if ((a < 32768) && (b < 32768)) {
			// Viktor's first equation when both sources are "quiet"
			// (i.e. less than middle of the dynamic range)
			m = a * b / 32768;
		}
		else {
			// Viktor's second equation when one or both sources are loud
			m = 2 * (a + b) - (a * b) / 32768 - 65536;
		}

		// Output is unsigned (0..65536) so convert back to signed (-32768..32767)
		if (m == 65536) m = 65535;
		m -= 32768;
		buf_a[i] = m;
	}
}

static short buf_samples[4086];
void mixer_update() {
	static int last_fps_time = 0;
	static int a_fps = 10;
	static int a_fps_c = 0;
	int cur_tick = vm_get_tick_count();

	a_fps_c++;

	if (cur_tick - last_fps_time > 1000) {
		last_fps_time = cur_tick;
		a_fps = a_fps_c - 1;
		a_fps_c = 0;

		if (a_fps < 1)
			a_fps = 1;
		if (a_fps > 100)
			a_fps = 100;
	}

	int free_buf_size = bitstream_get_free_buffer_size();
	int all_buf_size = bitstream_get_buffer_size();
	int used_buf_size = all_buf_size - free_buf_size;

	int play_buf_size = sample_rate * 4 / a_fps - used_buf_size;
	mre_printf("used_buf_size: %4d, fps: %2d, pbs: %d\n", used_buf_size, a_fps, play_buf_size);
	mre_printf("free_buf_size: %4d, all_buf_size: %4d\n", free_buf_size, all_buf_size);

	play_buf_size = play_buf_size * 120 / 100;

	if (play_buf_size > free_buf_size)
		play_buf_size = free_buf_size;

	play_buf_size &= ~0b11l;

	if (play_buf_size > 4096)
		play_buf_size = 4096;

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

			mix(buf_samples, channel->chunk->abuf + channel->pos, to_play / 2, 
				channel->left_planning, channel->right_planning);

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