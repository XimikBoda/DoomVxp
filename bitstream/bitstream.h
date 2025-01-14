#pragma once
#ifdef __cplusplus
extern "C" {
#endif

#include <vmbitstream.h>

VMINT bitstream_open(VMBOOL isStereo, VMUINT8 bitPerSample, vm_bitstream_sample_freq_enum sampleFreq);
VMINT bitstream_close();
VMINT bitstream_put_data(VMUINT8* buffer, VMUINT buffer_size, VMUINT* written);
VMINT bitstream_get_free_buffer_size();
VMINT bitstream_start(VMUINT start_time, VMUINT8 audio_path, VMINT volume);

#ifdef __cplusplus
}
#endif