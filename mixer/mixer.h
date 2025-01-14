#pragma once
#ifdef __cplusplus
extern "C" {
#endif

#include <bitstream.h>

typedef struct Mix_Chunk {
    int allocated;
    VMUINT8* abuf;
    VMUINT32 alen;
    VMUINT8 volume;		/* Per-sample volume, 0-128 */
} Mix_Chunk;

VMINT mixer_init(VMINT num_chanels, VMBOOL isStereo, VMUINT8 bitPerSample, vm_bitstream_sample_freq_enum sampleFreq);
void mixer_setup_channel(VMINT chnanel, Mix_Chunk* chunk, VMBOOL play);
void mixer_stop(VMINT chnanel);
VMBOOL mixer_is_playing(VMINT chnanel);
void mixer_update();
void mixer_close();

#ifdef __cplusplus
}
#endif