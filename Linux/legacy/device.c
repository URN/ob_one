#include <alsa/asoundlib.h>

#include "device.h"

void aerror(const char *msg, int r)
{
    fprintf(stderr, "ALSA %s: %s\n", msg, snd_strerror(r));
}

int setup_device(snd_pcm_t *pcm, int rate, int channels, int frame, int buffer_time)
{
	int r, dir;
	unsigned int p;
	snd_pcm_hw_params_t *hw_params;
	snd_pcm_sw_params_t *sw_params;
	snd_pcm_uframes_t buffer_size, period_size, f;

	snd_pcm_hw_params_alloca(&hw_params);
	snd_pcm_sw_params_alloca(&sw_params);

	r = snd_pcm_hw_params_any(pcm, hw_params);
	if (r < 0) {
		aerror("hw_params_any", r);
		return -1;
	}

	r = snd_pcm_hw_params_set_rate_resample(pcm, hw_params, 1);
	if (r < 0) {
		aerror("hw_params_set_rate_resample", r);
		return -1;
	}

	r = snd_pcm_hw_params_set_access(pcm, hw_params,
					 SND_PCM_ACCESS_RW_INTERLEAVED);
	if (r < 0) {
		aerror("hw_params_set_access", r);
		return -1;
	}

	r = snd_pcm_hw_params_set_format(pcm, hw_params, SND_PCM_FORMAT_S16);
	if (r < 0) {
		aerror("hw_params_set_format", r);
		return -1;
	}

	r = snd_pcm_hw_params_set_rate(pcm, hw_params, rate, 0);
	if (r < 0) {
		aerror("hw_params_set_rate", r);
		return -1;
	}

	r = snd_pcm_hw_params_set_channels(pcm, hw_params, channels);
	if (r < 0) {
		aerror("hw_params_set_channels", r);
		return -1;
	}

	/* Buffer size */

	p = buffer_time * 1000; /* microseconds */
	r = snd_pcm_hw_params_set_buffer_time_near(pcm, hw_params, &p, &dir);
	if (r < 0) {
		aerror("hw_params_set_buffer_time_near", r);
		return -1;
	}
	r = snd_pcm_hw_params_get_buffer_size(hw_params, &buffer_size);
	if (r < 0) {
		aerror("hw_params_get_buffer_size", r);
		return -1;
	}

	/* Period size */

	f = frame;
	r = snd_pcm_hw_params_set_period_size_near(pcm, hw_params, &f, &dir);
	if (r < 0) {
		aerror("hw_params_set_period_time_near", r);
		return -1;
	}
	r = snd_pcm_hw_params_get_period_size(hw_params, &period_size, &dir);
	if (r < 0) {
		aerror("hw_params_get_period_size", r);
		return -1;
	}

	/* Set the hardware parameters */

	r = snd_pcm_hw_params(pcm, hw_params);
	if (r < 0) {
		aerror("hw_params", r);
		return -1;
	}

	r = snd_pcm_sw_params_current(pcm, sw_params);
	if (r < 0) {
		aerror("sw_params_current", r);
		return -1;
	}

	/* Software buffer watersheds */
	
	r = snd_pcm_sw_params_set_start_threshold(pcm, sw_params, buffer_size);
	if (r < 0) {
		aerror("sw_params_set_start_threshold", r);
		return -1;
	}

	r = snd_pcm_sw_params_set_stop_threshold(pcm, sw_params, buffer_size);
	if (r < 0) {
		aerror("sw_params_set_stop_threshold", r);
		return -1;
	}

	r = snd_pcm_sw_params_set_avail_min(pcm, sw_params, period_size);
	if (r < 0) {
		aerror("sw_params_set_avail_min", r);
		return -1;
	}

	/* Set the software parameters */

	r = snd_pcm_sw_params(pcm, sw_params);
	if (r < 0) {
		aerror("sw_params", r);
		return -1;
	}

	return 0;
}
