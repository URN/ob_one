
#ifndef DEVICE_H
#define DEVICE_H

#define DEFAULT_RATE 48000
#define DEFAULT_CHANNELS 2
#define DEFAULT_FRAME 960
#define DEFAULT_DEVICE "default"
#define DEFAULT_BUFFER_TIME 150 /* milliseconds */
#define DEFAULT_PORT "1350" 

void aerror(const char *msg, int r);
int setup_device(snd_pcm_t *pcm, int rate, int channels, int frame, int buffer_time);

#endif
