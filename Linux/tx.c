#include <netdb.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <alsa/asoundlib.h>
#include <arpa/inet.h>
#include <opus/opus.h>
#include <netinet/in.h>

#include "device.h"

#define DEFAULT_BYTES_PER_FRAME 96

int main(int argc, char *argv[])
{
	int sd, r;
	ssize_t bytes;
	struct addrinfo hints, *servinfo, *p;
	opus_int16 *pcm;
	OpusEncoder *encoder;
	opus_int32 frame_size;
	
	/* options, and variables for calling getopt */
	int c;
	int bytes_per_frame = DEFAULT_BYTES_PER_FRAME;
	int channels = DEFAULT_CHANNELS;
	opus_int32 rate = DEFAULT_RATE;
	int frame = DEFAULT_FRAME;
	int buffer_time = DEFAULT_BUFFER_TIME;
	char *port = DEFAULT_PORT;
	char *device = DEFAULT_DEVICE;
	
	int verbosity = 1;
	
	fputs("tx: George O'Neill, 2015\n", stderr);

	while ((c = getopt (argc, argv, "b:B:c:d:f:p:qr:v")) != -1) {
		switch (c) {
			case 'b':
				bytes_per_frame = atoi(optarg);
				break;
			case 'B':
				buffer_time = atoi(optarg);
				break;
			case 'c':
				channels = atoi(optarg);
				break;
			case 'd':
				device = optarg;
				break;
			case 'f':
				frame = atoi(optarg);
				break;
			case 'p':
				port = optarg;
				break;
			case 'q':
				if (verbosity==2) {
					fprintf(stderr, "-q and -v may not be set together.\n");
					return -1;
				} else {
					verbosity = 0;
				}
				break;
			case 'r':
				rate = atoi(optarg);
				break;
			case 'v':
				if (verbosity==0) {
					fprintf(stderr, "-q and -v may not be set together.\n");
					return -1;
				} else {
					verbosity = 2;
				}
				break;
			case '?':
				switch (optopt) {
					case 'b':
					case 'B':
					case 'c':
					case 'd':
					case 'f':
					case 'p':
					case 'r':
						fprintf(stderr, "Option -%c requires an argument.\n", optopt);
						break;
					default:
						if (isprint(optopt)) {
							fprintf (stderr, "Unknown option `-%c'.\n", optopt);
						}
				}
				break;
			default: abort();
		}
	}
	
	/* these declarations moved down here, because the size depends on the options */
	short abuf[frame * channels * sizeof(opus_int16)];
	char encoded[bytes_per_frame];
	
	if (argc-optind != 1) {
		fputs("usage: tx [options] <hostname>\n", stderr);
		return -1;
	}

	/* Bind to network socket, for sending */

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_DGRAM;

	r = getaddrinfo(argv[optind], port, &hints, &servinfo);
	if (r != 0) {
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(r));
		return -1;
	}

	for (p = servinfo; p != NULL; p = p->ai_next) {
		sd = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
		if (sd == -1) {
			perror("socket");
			continue;
		}
		break;
	}

	if (p == NULL) {
		fputs("Failed to bind socket\n", stderr);
		return -1;
	}

	/* Prepare the audio codec */

	encoder = opus_encoder_create(rate, channels, OPUS_APPLICATION_AUDIO, r);
	if (r < 0 ) {
		fprintf(stderr,"Failed to initialise Opus codec: %s\n", opus_strerror(r));
		return -1;
	}

	/* Prepare the audio device */

	r = snd_pcm_open(&pcm, device, SND_PCM_STREAM_CAPTURE, 0);
	if (r < 0) {
		aerror("open", r);
		return -1;
	}

	if (setup_device(pcm, rate, channels, frame, buffer_time) == -1)
		return -1;

	/* Main loop */
	
	for (;;) {

		r = snd_pcm_readi(pcm, abuf, frame);
		if (r < 0) {
			aerror("snd_pcm_readi", r);
			if (r == -EPIPE) {
				snd_pcm_prepare(pcm);
				continue;
			}
			return -1;
		}

		r = opus_encode(encoder, abuf, frame, encoded,
				bytes_per_frame);
		if (r < 0) {
			fprintf(stderr, "Error encoding PCM to Opus: %s\n", opus_strerror(r));
			return -1;
		}

		bytes = sendto(sd, encoded, r, 0,
			       p->ai_addr, p->ai_addrlen);
		if (bytes == -1) {
			perror("sendto");
			return -1;
		}
		
		switch (verbosity) {
			case 0:
				fprintf(stderr, "Transmission started.\n");
				verbosity = 255;
				break;
			case 1:
				fputc('t', stderr);
				break;
			case 2:
				fprintf(stderr, "Sent %d bytes to %s\n", bytes_per_frame, argv[optind]);
				break;
		}
	}

	/* Close audio */

	r = snd_pcm_close(pcm);
	if (r < 0) {
		aerror("close", r);
		return -1;
	}

	/* Close codec */

	opus_encoder_destroy(encoder);

	/* Close networking */

	freeaddrinfo(servinfo);

	if (close(sd) == -1) {
		perror("close");
		return -1;
	}

	return 0;
}
