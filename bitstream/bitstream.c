#include "bitstream.h"
#include "injected_bitstream.h"
#include <vmbitstream.h>
#include <vmmm.h>

static const int sample_rate_enum_to_int[VM_BITSTREAM_SAMPLE_FREQ_TOTAL] =
{
	8000, 11025, 16000, 22050, 24000, 32000, 44100, 48000
};

static int mre_bitstream_alloved = -1;
static int mre_bitstream_handle = -1;
static int mre_bitstream_buffer_size = -1;
static int mre_bitstream_sample_rate = -1;

void mre_bitstream_callback(VMINT handle, VMINT result) {}
void injected_bitstream_callback(PCM* handle, PCM_Event event) {}

static VMINT bitstream_sample_rate_enum_to_int(vm_bitstream_sample_freq_enum sampleFreq) {
	if ((VMUINT)sampleFreq >= VM_BITSTREAM_SAMPLE_FREQ_TOTAL)
		return 0;
	else
		return sample_rate_enum_to_int[sampleFreq];
}

VMINT bitstream_open(VMBOOL isStereo, VMUINT8 bitPerSample, vm_bitstream_sample_freq_enum sampleFreq) {
	if (mre_bitstream_alloved == -1)
		mre_bitstream_alloved = (vm_get_sym_entry("vm_bitstream_audio_open_pcm") != 0);

	mre_bitstream_buffer_size = -1;

	mre_bitstream_sample_rate = bitstream_sample_rate_enum_to_int(sampleFreq);

	if (mre_bitstream_alloved)
	{
		if (mre_bitstream_handle != -1)
			vm_bitstream_audio_close(mre_bitstream_handle);

		vm_bitstream_pcm_audio_cfg_struct settings;
		settings.vm_codec_type = VM_BITSTREAM_CODEC_TYPE_PCM;
		settings.isStereo = isStereo;
		settings.bitPerSample = bitPerSample;
		settings.sampleFreq = sampleFreq;

		return vm_bitstream_audio_open_pcm(&mre_bitstream_handle, &settings, mre_bitstream_callback);
	}
	else
	{
		if (((VMUINT32)sampleFreq) >= VM_BITSTREAM_SAMPLE_FREQ_TOTAL)
			return 1;

		PCM_Param pcmParam;
		pcmParam.isStereo = isStereo;
		pcmParam.bitPerSample = bitPerSample;
		pcmParam.sampleFreq = bitstream_sample_rate_enum_to_int(sampleFreq);
		pcmParam.forceVoice = FALSE;

		return injected_bitstream_init(&pcmParam, injected_bitstream_callback);
	}
}

VMINT bitstream_close() {
	mre_bitstream_buffer_size = -1;

	if (mre_bitstream_alloved)
	{
		int res = vm_bitstream_audio_close(mre_bitstream_handle);
		mre_bitstream_handle = -1;
		return res;
	}
	else
	{
		injected_bitstream_deinit();
		return 0;
	}
}

VMINT bitstream_put_data(VMUINT8* buffer, VMUINT buffer_size, VMUINT* written) {
	if (mre_bitstream_alloved)
	{
		return vm_bitstream_audio_put_data(mre_bitstream_handle, buffer, buffer_size, written);
	}
	else
	{
		injected_bitstream_put_buffer(buffer, buffer_size, written);
		return 0;
	}
}

VMINT bitstream_get_free_buffer_size() {
	if (mre_bitstream_alloved)
	{
		vm_bitstream_audio_buffer_status status;
		vm_bitstream_audio_get_buffer_status(mre_bitstream_handle, &status);
		return status.free_buf_size;
	}
	else
	{
		return injected_bitstream_get_free_buffer_size();
	}
}

VMINT bitstream_get_sample_rate() {
	return mre_bitstream_sample_rate;
}

VMINT bitstream_get_buffer_size() {
	if (mre_bitstream_alloved)
	{
		if (mre_bitstream_buffer_size == -1) {
			vm_bitstream_audio_buffer_status status;
			vm_bitstream_audio_get_buffer_status(mre_bitstream_handle, &status);
			mre_bitstream_buffer_size = status.total_buf_size;
		}
		return mre_bitstream_buffer_size;
	}
	else
	{
		return injected_bitstream_get_buffer_size();
	}
}

VMINT bitstream_start(VMUINT start_time, VMUINT8 audio_path, VMINT volume) {
	if (mre_bitstream_alloved)
	{
		vm_bitstream_audio_start_param param;
		param.start_time = start_time;
		param.audio_path = audio_path;
		param.volume = volume;
		return vm_bitstream_audio_start(mre_bitstream_handle, &param);
	}
	else
	{
		vm_set_volume(volume);
		injected_bitstream_start();
	}

}