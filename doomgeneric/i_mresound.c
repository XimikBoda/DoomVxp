//
// Copyright(C) 1993-1996 Id Software, Inc.
// Copyright(C) 2005-2014 Simon Howard
// Copyright(C) 2008 David Flater
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// DESCRIPTION:
//	System interface for sound.
//

#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <ctype.h>

#ifdef HAVE_LIBSAMPLERATE
#include <samplerate.h>
#endif

#include "deh_str.h"
#include "i_sound.h"
#include "i_system.h"
#include "i_swap.h"
#include "m_argv.h"
#include "m_misc.h"
#include "w_wad.h"
#include "z_zone.h"

#include "doomtype.h"

#include <mixer.h>

#define LOW_PASS_FILTER 0
//#define DEBUG_DUMP_WAVS
#define NUM_CHANNELS 16

typedef struct allocated_sound_s allocated_sound_t;



struct allocated_sound_s
{
	sfxinfo_t* sfxinfo;
	Mix_Chunk chunk;
	int use_count;
	allocated_sound_t* prev, * next;
};

//static VMINT bitstream_handle = -1;

//static VMINT bitstreams_handles[NUM_CHANNELS];

static boolean setpanning_workaround = false;

static boolean sound_initialized = false;

static sfxinfo_t* channels_playing[NUM_CHANNELS];

static int mixer_freq;
//static Uint16 mixer_format;
static int mixer_channels;
static boolean use_sfx_prefix;
static boolean(*ExpandSoundData)(sfxinfo_t* sfxinfo,
	byte* data,
	int samplerate,
	int length) = NULL;

// Doubly-linked list of allocated sounds.
// When a sound is played, it is moved to the head, so that the oldest
// sounds not used recently are at the tail.

static allocated_sound_t* allocated_sounds_head = NULL;
static allocated_sound_t* allocated_sounds_tail = NULL;
static int allocated_sounds_size = 0;

int use_libsamplerate = 0;

// Scale factor used when converting libsamplerate floating point numbers
// to integers. Too high means the sounds can clip; too low means they
// will be too quiet. This is an amount that should avoid clipping most
// of the time: with all the Doom IWAD sound effects, at least. If a PWAD
// is used, clipping might occur.

float libsamplerate_scale = 0.65f;

// Hook a sound into the linked list at the head.

static void AllocatedSoundLink(allocated_sound_t* snd)
{
	snd->prev = NULL;

	snd->next = allocated_sounds_head;
	allocated_sounds_head = snd;

	if (allocated_sounds_tail == NULL)
	{
		allocated_sounds_tail = snd;
	}
	else
	{
		snd->next->prev = snd;
	}
}

// Unlink a sound from the linked list.

static void AllocatedSoundUnlink(allocated_sound_t* snd)
{
	if (snd->prev == NULL)
	{
		allocated_sounds_head = snd->next;
	}
	else
	{
		snd->prev->next = snd->next;
	}

	if (snd->next == NULL)
	{
		allocated_sounds_tail = snd->prev;
	}
	else
	{
		snd->next->prev = snd->prev;
	}
}

static void FreeAllocatedSound(allocated_sound_t* snd)
{
	// Unlink from linked list.

	AllocatedSoundUnlink(snd);

	// Unlink from higher-level code.

	snd->sfxinfo->driver_data = NULL;

	// Keep track of the amount of allocated sound data:

	allocated_sounds_size -= snd->chunk.alen;

	free(snd);
}

// Search from the tail backwards along the allocated sounds list, find
// and free a sound that is not in use, to free up memory.  Return true
// for success.

static boolean FindAndFreeSound(void)
{
	allocated_sound_t* snd;

	snd = allocated_sounds_tail;

	while (snd != NULL)
	{
		if (snd->use_count == 0)
		{
			FreeAllocatedSound(snd);
			return true;
		}

		snd = snd->prev;
	}

	// No available sounds to free...

	return false;
}

// Enforce SFX cache size limit.  We are just about to allocate "len"
// bytes on the heap for a new sound effect, so free up some space
// so that we keep allocated_sounds_size < snd_cachesize

static void ReserveCacheSpace(size_t len)
{
	if (snd_cachesize <= 0)
	{
		return;
	}

	// Keep freeing sound effects that aren't currently being played,
	// until there is enough space for the new sound.

	while (allocated_sounds_size + len > snd_cachesize)
	{
		// Free a sound.  If there is nothing more to free, stop.

		if (!FindAndFreeSound())
		{
			break;
		}
	}
}

// Allocate a block for a new sound effect.

static Mix_Chunk* AllocateSound(sfxinfo_t* sfxinfo, size_t len)
{
	allocated_sound_t* snd;

	// Keep allocated sounds within the cache size.

	ReserveCacheSpace(len);

	// Allocate the sound structure and data.  The data will immediately
	// follow the structure, which acts as a header.

	do
	{
		snd = malloc(sizeof(allocated_sound_t) + len);

		// Out of memory?  Try to free an old sound, then loop round
		// and try again.

		if (snd == NULL && !FindAndFreeSound())
		{
			return NULL;
		}

	} while (snd == NULL);

	// Skip past the chunk structure for the audio buffer

	snd->chunk.abuf = (byte*)(snd + 1);
	snd->chunk.alen = len;
	snd->chunk.allocated = 1;
	snd->chunk.volume = 100;

	snd->sfxinfo = sfxinfo;
	snd->use_count = 0;

	// driver_data pointer points to the allocated_sound structure.

	sfxinfo->driver_data = snd;

	// Keep track of how much memory all these cached sounds are using...

	allocated_sounds_size += len;

	AllocatedSoundLink(snd);

	return &snd->chunk;
}

// Lock a sound, to indicate that it may not be freed.

static void LockAllocatedSound(allocated_sound_t* snd)
{
	// Increase use count, to stop the sound being freed.

	++snd->use_count;

	//mre_printf("++ %s: Use count=%i\n", snd->sfxinfo->name, snd->use_count);

	// When we use a sound, re-link it into the list at the head, so
	// that the oldest sounds fall to the end of the list for freeing.

	AllocatedSoundUnlink(snd);
	AllocatedSoundLink(snd);
}

// Unlock a sound to indicate that it may now be freed.

static void UnlockAllocatedSound(allocated_sound_t* snd)
{
	if (snd->use_count <= 0)
	{
		I_Error("Sound effect released more times than it was locked...");
	}

	--snd->use_count;

	if (snd->use_count == 0)
		FreeAllocatedSound(snd);

	//mre_printf("-- %s: Use count=%i\n", snd->sfxinfo->name, snd->use_count);
}

// When a sound stops, check if it is still playing.  If it is not, 
// we can mark the sound data as CACHE to be freed back for other
// means.

static void ReleaseSoundOnChannel(int channel)
{
	sfxinfo_t* sfxinfo = channels_playing[channel];

	if (sfxinfo == NULL)
	{
		return;
	}

	channels_playing[channel] = NULL;

	UnlockAllocatedSound(sfxinfo->driver_data);
}

#ifdef HAVE_LIBSAMPLERATE

// Returns the conversion mode for libsamplerate to use.

static int SRC_ConversionMode(void)
{
	switch (use_libsamplerate)
	{
		// 0 = disabled

	default:
	case 0:
		return -1;

		// Ascending numbers give higher quality

	case 1:
		return SRC_LINEAR;
	case 2:
		return SRC_ZERO_ORDER_HOLD;
	case 3:
		return SRC_SINC_FASTEST;
	case 4:
		return SRC_SINC_MEDIUM_QUALITY;
	case 5:
		return SRC_SINC_BEST_QUALITY;
	}
}

// libsamplerate-based generic sound expansion function for any sample rate
//   unsigned 8 bits --> signed 16 bits
//   mono --> stereo
//   samplerate --> mixer_freq
// Returns number of clipped samples.
// DWF 2008-02-10 with cleanups by Simon Howard.

static boolean ExpandSoundData_SRC(sfxinfo_t* sfxinfo,
	byte* data,
	int samplerate,
	int length)
{
	SRC_DATA src_data;
	uint32_t i, abuf_index = 0, clipped = 0;
	uint32_t alen;
	int retn;
	int16_t* expanded;
	Mix_Chunk* chunk;

	src_data.input_frames = length;
	src_data.data_in = malloc(length * sizeof(float));
	src_data.src_ratio = (double)mixer_freq / samplerate;

	// We include some extra space here in case of rounding-up.
	src_data.output_frames = src_data.src_ratio * length + (mixer_freq / 4);
	src_data.data_out = malloc(src_data.output_frames * sizeof(float));

	assert(src_data.data_in != NULL && src_data.data_out != NULL);

	// Convert input data to floats

	for (i = 0; i < length; ++i)
	{
		// Unclear whether 128 should be interpreted as "zero" or whether a
		// symmetrical range should be assumed.  The following assumes a
		// symmetrical range.
		src_data.data_in[i] = data[i] / 127.5 - 1;
	}

	// Do the sound conversion

	retn = src_simple(&src_data, SRC_ConversionMode(), 1);
	assert(retn == 0);

	// Allocate the new chunk.

	alen = src_data.output_frames_gen * 4;

	chunk = AllocateSound(sfxinfo, src_data.output_frames_gen * 4);

	if (chunk == NULL)
	{
		return false;
	}

	expanded = (int16_t*)chunk->abuf;

	// Convert the result back into 16-bit integers.

	for (i = 0; i < src_data.output_frames_gen; ++i)
	{
		// libsamplerate does not limit itself to the -1.0 .. 1.0 range on
		// output, so a multiplier less than INT16_MAX (32767) is required
		// to avoid overflows or clipping.  However, the smaller the
		// multiplier, the quieter the sound effects get, and the more you
		// have to turn down the music to keep it in balance.

		// 22265 is the largest multiplier that can be used to resample all
		// of the Vanilla DOOM sound effects to 48 kHz without clipping
		// using SRC_SINC_BEST_QUALITY.  It is close enough (only slightly
		// too conservative) for SRC_SINC_MEDIUM_QUALITY and
		// SRC_SINC_FASTEST.  PWADs with interestingly different sound
		// effects or target rates other than 48 kHz might still result in
		// clipping--I don't know if there's a limit to it.

		// As the number of clipped samples increases, the signal is
		// gradually overtaken by noise, with the loudest parts going first.
		// However, a moderate amount of clipping is often tolerated in the
		// quest for the loudest possible sound overall.  The results of
		// using INT16_MAX as the multiplier are not all that bad, but
		// artifacts are noticeable during the loudest parts.

		float cvtval_f =
			src_data.data_out[i] * libsamplerate_scale * INT16_MAX;
		int32_t cvtval_i = cvtval_f + (cvtval_f < 0 ? -0.5 : 0.5);

		// Asymmetrical sound worries me, so we won't use -32768.
		if (cvtval_i < -INT16_MAX)
		{
			cvtval_i = -INT16_MAX;
			++clipped;
		}
		else if (cvtval_i > INT16_MAX)
		{
			cvtval_i = INT16_MAX;
			++clipped;
		}

		// Left and right channels

		expanded[abuf_index++] = cvtval_i;
		expanded[abuf_index++] = cvtval_i;
	}

	free(src_data.data_in);
	free(src_data.data_out);

	if (clipped > 0)
	{
		mre_fprintf(stderr, "Sound '%s': clipped %u samples (%0.2f %%)\n",
			sfxinfo->name, clipped,
			400.0 * clipped / chunk->alen);
	}

	return true;
}

#endif

static boolean ConvertibleRatio(int freq1, int freq2)
{
	int ratio;

	if (freq1 > freq2)
	{
		return ConvertibleRatio(freq2, freq1);
	}
	else if ((freq2 % freq1) != 0)
	{
		// Not in a direct ratio

		return false;
	}
	else
	{
		// Check the ratio is a power of 2

		ratio = freq2 / freq1;

		while ((ratio & 1) == 0)
		{
			ratio = ratio >> 1;
		}

		return ratio == 1;
	}
}

#ifdef DEBUG_DUMP_WAVS

// Debug code to dump resampled sound effects to WAV files for analysis.

static void WriteWAV(char* filename, byte* data,
	uint32_t length, int samplerate)
{
	FILE* wav;
	unsigned int i;
	unsigned short s;

	wav = fopen(filename, "wb");

	// Header

	fwrite("RIFF", 1, 4, wav);
	i = LONG(36 + samplerate);
	fwrite(&i, 4, 1, wav);
	fwrite("WAVE", 1, 4, wav);

	// Subchunk 1

	fwrite("fmt ", 1, 4, wav);
	i = LONG(16);
	fwrite(&i, 4, 1, wav);           // Length
	s = SHORT(1);
	fwrite(&s, 2, 1, wav);           // Format (PCM)
	s = SHORT(2);
	fwrite(&s, 2, 1, wav);           // Channels (2=stereo)
	i = LONG(samplerate);
	fwrite(&i, 4, 1, wav);           // Sample rate
	i = LONG(samplerate * 2 * 2);
	fwrite(&i, 4, 1, wav);           // Byte rate (samplerate * stereo * 16 bit)
	s = SHORT(2 * 2);
	fwrite(&s, 2, 1, wav);           // Block align (stereo * 16 bit)
	s = SHORT(16);
	fwrite(&s, 2, 1, wav);           // Bits per sample (16 bit)

	// Data subchunk

	fwrite("data", 1, 4, wav);
	i = LONG(length);
	fwrite(&i, 4, 1, wav);           // Data length
	fwrite(data, 1, length, wav);    // Data

	fclose(wav);
}

#endif

// Generic sound expansion function for any sample rate.
// Returns number of clipped samples (always 0).

static boolean ExpandSoundData_MRE(sfxinfo_t* sfxinfo,
	byte* data,
	int samplerate,
	int length)
{
	//    SDL_AudioCVT convertor;
	Mix_Chunk* chunk;
	uint32_t expanded_length;

	// Calculate the length of the expanded version of the sample.    

	expanded_length = (uint32_t)((((uint64_t)length) * mixer_freq) / samplerate);

	// Double up twice: 8 -> 16 bit and mono -> stereo

	expanded_length *= 4;

	// Allocate a chunk in which to expand the sound

	chunk = AllocateSound(sfxinfo, expanded_length);

	if (chunk == NULL)
	{
		return false;
	}

	// If we can, use the standard / optimized SDL conversion routines.
	VMINT16* expanded = (VMINT16*)chunk->abuf;
	int expand_ratio;
	int i;

	// Generic expansion if conversion does not work:
	//
	// SDL's audio conversion only works for rate conversions that are
	// powers of 2; if the two formats are not in a direct power of 2
	// ratio, do this naive conversion instead.

	// number of samples in the converted sound

	expanded_length = ((uint64_t)length * mixer_freq) / samplerate;
	expand_ratio = (length << 8) / expanded_length;

	for (i = 0; i < expanded_length; ++i)
	{
		VMINT16 sample;
		int src;

		src = (i * expand_ratio) >> 8;

		sample = data[src] | (data[src] << 8);
		sample -= 32768;

		// expand 8->16 bits, mono->stereo

		expanded[i * 2] = expanded[i * 2 + 1] = sample;
	}

#ifdef LOW_PASS_FILTER
	// Perform a low-pass filter on the upscaled sound to filter
	// out high-frequency noise from the conversion process.

	{
		float rc, dt, alpha;

		// Low-pass filter for cutoff frequency f:
		//
		// For sampling rate r, dt = 1 / r
		// rc = 1 / 2*pi*f
		// alpha = dt / (rc + dt)

		// Filter to the half sample rate of the original sound effect
		// (maximum frequency, by nyquist)

		dt = 1.0f / mixer_freq;
		rc = 1.0f / (3.14f * samplerate);
		alpha = dt / (rc + dt);

		// Both channels are processed in parallel, hence [i-2]:

		for (i = 2; i < expanded_length * 2; ++i)
		{
			expanded[i] = (VMINT16)(alpha * expanded[i]
				+ (1 - alpha) * expanded[i - 2]);
		}
	}
#endif /* #ifdef LOW_PASS_FILTER */

	return true;
}

// Load and convert a sound effect
// Returns true if successful

static boolean CacheSFX(sfxinfo_t* sfxinfo)
{
	int lumpnum;
	unsigned int lumplen;
	int samplerate;
	unsigned int length;
	byte* data;

	// need to load the sound

	lumpnum = sfxinfo->lumpnum;
	data = W_CacheLumpNum(lumpnum, PU_STATIC);
	lumplen = W_LumpLength(lumpnum);

	// Check the header, and ensure this is a valid sound

	if (lumplen < 8
		|| data[0] != 0x03 || data[1] != 0x00)
	{
		// Invalid sound

		return false;
	}

	// 16 bit sample rate field, 32 bit length field

	samplerate = (data[3] << 8) | data[2];
	length = (data[7] << 24) | (data[6] << 16) | (data[5] << 8) | data[4];

	// If the header specifies that the length of the sound is greater than
	// the length of the lump itself, this is an invalid sound lump

	// We also discard sound lumps that are less than 49 samples long,
	// as this is how DMX behaves - although the actual cut-off length
	// seems to vary slightly depending on the sample rate.  This needs
	// further investigation to better understand the correct
	// behavior.

	if (length > lumplen - 8 || length <= 48)
	{
		return false;
	}

	// The DMX sound library seems to skip the first 16 and last 16
	// bytes of the lump - reason unknown.

	data += 16;
	length -= 32;

	// Sample rate conversion

	if (!ExpandSoundData(sfxinfo, data + 8, samplerate, length))
	{
		return false;
	}

#ifdef DEBUG_DUMP_WAVS
	{
		char filename[16];

		M_snprintf(filename, sizeof(filename), "%s.wav",
			DEH_String(S_sfx[sound].name));
		WriteWAV(filename, sound_chunks[sound].abuf,
			sound_chunks[sound].alen, mixer_freq);
	}
#endif

	// don't need the original lump any more

	W_ReleaseLumpNum(lumpnum);

	return true;
}

static void GetSfxLumpName(sfxinfo_t* sfx, char* buf, size_t buf_len)
{
	// Linked sfx lumps? Get the lump number for the sound linked to.

	if (sfx->link != NULL)
	{
		sfx = sfx->link;
	}

	// Doom adds a DS* prefix to sound lumps; Heretic and Hexen don't
	// do this.

	if (use_sfx_prefix)
	{
		M_snprintf(buf, buf_len, "ds%s", DEH_String(sfx->name));
	}
	else
	{
		M_StringCopy(buf, DEH_String(sfx->name), buf_len);
	}
}

static void I_MRE_PrecacheSounds(sfxinfo_t* sounds, int num_sounds)
{
	// no-op
}


// Load a SFX chunk into memory and ensure that it is locked.

static boolean LockSound(sfxinfo_t* sfxinfo)
{
	// If the sound isn't loaded, load it now

	if (sfxinfo->driver_data == NULL)
	{
		if (!CacheSFX(sfxinfo))
		{
			return false;
		}
	}

	LockAllocatedSound(sfxinfo->driver_data);

	return true;
}

//
// Retrieve the raw data lump index
//  for a given SFX name.
//

static int I_MRE_GetSfxLumpNum(sfxinfo_t* sfx)
{
	char namebuf[9];

	GetSfxLumpName(sfx, namebuf, sizeof(namebuf));

	return W_GetNumForName(namebuf);
}

static void I_MRE_UpdateSoundParams(int handle, int vol, int sep)
{
	int left, right;

	if (!sound_initialized || handle < 0 || handle >= NUM_CHANNELS)
	{
		return;
	}

	left = ((254 - sep) * vol) / 127;
	right = ((sep)*vol) / 127;

	if (left < 0) left = 0;
	else if (left > 255) left = 255;
	if (right < 0) right = 0;
	else if (right > 255) right = 255;

	// SDL_mixer version 1.2.8 and earlier has a bug in the Mix_SetPanning
	// function.  A workaround is to call Mix_UnregisterAllEffects for
	// the channel before calling it.  This is undesirable as it may lead
	// to the channel volumes resetting briefly.

	if (setpanning_workaround)
	{
		//Mix_UnregisterAllEffects(handle);
	}

	mixer_set_planing(handle, left, right);
	//Mix_SetPanning(handle, left, right);
}

//
// Starting a sound means adding it
//  to the current list of active sounds
//  in the internal channels.
// As the SFX info struct contains
//  e.g. a pointer to the raw data,
//  it is ignored.
// As our sound handling does not handle
//  priority, it is ignored.
// Pitching (that is, increased speed of playback)
//  is set, but currently not used by mixing.
//

static int I_MRE_StartSound(sfxinfo_t* sfxinfo, int channel, int vol, int sep)
{
	allocated_sound_t* snd;
	VMUINT writen = 0;

	if (!sound_initialized || channel < 0 || channel >= NUM_CHANNELS)
	{
		return -1;
	}

	// Release a sound effect if there is already one playing
	// on this channel

	ReleaseSoundOnChannel(channel);

	// Get the sound data

	if (!LockSound(sfxinfo))
	{
		return -1;
	}

	snd = sfxinfo->driver_data;

	// play sound

	mixer_setup_channel(channel, &snd->chunk, 1);

	//Mix_PlayChannelTimed(channel, &snd->chunk, 0, -1);

	channels_playing[channel] = sfxinfo;

	// set separation, etc.

	I_MRE_UpdateSoundParams(channel, vol, sep);

	return channel;
}

static void I_MRE_StopSound(int handle)
{
	if (!sound_initialized || handle < 0 || handle >= NUM_CHANNELS)
	{
		return;
	}

	mixer_stop(handle);

	// Sound data is no longer needed; release the
	// sound data being used for this channel

	ReleaseSoundOnChannel(handle);
}


static boolean I_MRE_SoundIsPlaying(int handle)
{
	if (!sound_initialized || handle < 0 || handle >= NUM_CHANNELS)
	{
		return false;
	}

	return mixer_is_playing(handle);
}

// 
// Periodically called to update the sound system
//

static void I_MRE_UpdateSound(void)
{
	int i;

	mixer_update();

	// Check all channels to see if a sound has finished

	for (i = 0; i < NUM_CHANNELS; ++i)
	{
		if (channels_playing[i] && !I_MRE_SoundIsPlaying(i))
		{
			// Sound has finished playing on this channel,
			// but sound data has not been released to cache

			ReleaseSoundOnChannel(i);
		}
	}
}


static void I_MRE_ShutdownSound(void)
{
	if (!sound_initialized)
	{
		return;
	}

	mixer_close();

	sound_initialized = false;
}

static void bitstream_audio_result(VMINT handle, VMINT result) {}

static boolean I_MRE_InitSound(boolean _use_sfx_prefix)
{
	int i;

	use_sfx_prefix = _use_sfx_prefix;

	// No sounds yet

	for (i = 0; i < NUM_CHANNELS; ++i)
	{
		channels_playing[i] = NULL;
	}

	mixer_freq = 11025;

	if (mixer_init(NUM_CHANNELS, 1, 16, VM_BITSTREAM_SAMPLE_FREQ_11025))
	{
		mre_fprintf(stderr, "Unable to set up sound.\n");
		return false;
	}

	ExpandSoundData = ExpandSoundData_MRE;

	sound_initialized = true;

	return true;
}

static snddevice_t sound_mre_devices[] =
{
	SNDDEVICE_SB,
	SNDDEVICE_PAS,
	SNDDEVICE_GUS,
	SNDDEVICE_WAVEBLASTER,
	SNDDEVICE_SOUNDCANVAS,
	SNDDEVICE_AWE32,
};

sound_module_t DG_sound_module =
{
	sound_mre_devices,
	arrlen(sound_mre_devices),
	I_MRE_InitSound,
	I_MRE_ShutdownSound,
	I_MRE_GetSfxLumpNum,
	I_MRE_UpdateSound,
	I_MRE_UpdateSoundParams,
	I_MRE_StartSound,
	I_MRE_StopSound,
	I_MRE_SoundIsPlaying,
	I_MRE_PrecacheSounds,
};

