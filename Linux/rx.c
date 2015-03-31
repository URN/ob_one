#include <netdb.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <alsa/asoundlib.h>
#include <arpa/inet.h>
#include <opus/opus.h>
#include <netinet/in.h>

#include "device.h"

#define DEFAULT_MAX_ENCODED_BYTES 8192

void *get_in_addr(struct sockaddr *sa)
{
	if (sa->sa_family == AF_INET) {
		return &(((struct sockaddr_in*)sa)->sin_addr);
	}
	return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

int main(int argc, char *argv[])
{
	int sd, r;
	char s[INET6_ADDRSTRLEN];
	size_t addr_len;
	ssize_t bytes;
	struct addrinfo hints, *servinfo, *p;
	struct sockaddr_storage their_addr;
	opus_int16 *pcm;
	OpusDecoder *decoder;
	
	/* options, and variables for calling getopt */
	int c;
	int err;
	int channels = DEFAULT_CHANNELS;
	opus_int32 rate = 48000;
	int frame = DEFAULT_FRAME;
	int buffer_time = DEFAULT_BUFFER_TIME;
	int max_encoded_bytes = DEFAULT_MAX_ENCODED_BYTES;
	char *port = DEFAULT_PORT;
	char *device = DEFAULT_DEVICE;
	
	int verbosity = 1;

	fputs("rx: George O'Neill, 2015\n", stderr);
	
	while ((c = getopt (argc, argv, "B:c:f:m:p:qr:v")) != -1) {
		switch (c) {
			case 'B':
				buffer_time = atoi(optarg);
				break;
			case 'c':
				channels = atoi(optarg);
				break;
			case 'f':
				frame = atoi(optarg);
				break;
			case 'm':
				max_encoded_bytes = atoi(optarg);
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
					case 'B':
					case 'c':
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
	opus_int16 abuf[frame * channels *sizeof(opus_int16)];
	char encoded[max_encoded_bytes];

	if (argc-optind != 0) {
		fputs("usage: rx [options]\n", stderr);
		return -1;
	}
	

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_DGRAM;
	hints.ai_flags = AI_PASSIVE; /* use my IP */

	r = getaddrinfo(NULL, port, &hints, &servinfo);
	if (r == -1) {
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(r));
		return -1;
	}

	/* Bind to the first one we can */

	for (p = servinfo; p != NULL; p = p->ai_next) {
		sd = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
		if (sd == -1) {
			perror("socket");
			continue;
		}

		if (bind(sd, p->ai_addr, p->ai_addrlen) == -1) {
			if (close(sd) == -1) {
				perror("close");
				return -1;
			}
			perror("bind");
			continue;
		}

		break;
	}


	if (p == NULL) {
		fputs("Failed to bind socket\n", stderr);
		return -1;
	}

	freeaddrinfo(servinfo);

	fputs("Waiting to recvfrom()...\n", stderr);

	addr_len = sizeof(their_addr);

	/* Prepare the audio codec */

	decoder = opus_decoder_create(rate, channels, &err);
	if (err<0) {
		fprintf(stderr,"Error creating decoder: %s\n", opus_strerror(err));
		return -1;
	}

	/* Prepare the audio device */
	r = snd_pcm_open(&pcm, device, SND_PCM_STREAM_PLAYBACK, SND_PCM_NONBLOCK);
	if (r < 0) {
		aerror("open", r);
		return -1;
	}

	if (setup_device(pcm, rate, channels, frame, buffer_time) == -1)
		return -1;


	/* Main loop */

	for (;;) {
		bytes = recvfrom(sd, encoded, sizeof(encoded), 0,
				 (struct sockaddr*)&their_addr, &addr_len);
		if (bytes == -1) {
			perror("recvfrom");
			return -1;
		}
		
		switch (verbosity) {
			case 0:
				fprintf(stderr, "Receiving started.\n");
				verbosity = 255;
				break;
			case 1:
				fputc('|', stderr);
				break;
			case 2:
				fprintf(stderr, "%s: %d bytes\n",
					inet_ntop(their_addr.ss_family,
					get_in_addr((struct sockaddr*)&their_addr),
					s, sizeof(s)), bytes);
				break;
		}

		r = opus_decode(decoder, encoded, bytes, abuf, frame, 0);
		if(r<0) {
      fprintf(stderr, "Opus failed to decode: %s\n", opus_strerror(r));
      // return -1;
   } 

		r = snd_pcm_writei(pcm, abuf, frame);
		if (r < 0) {
			fprintf(stderr,".");
			//fprintf(stderr,"%s",snd_strerror(r));
			if (r == -EPIPE) {
				snd_pcm_prepare(pcm);
				continue;
			}
			if (r == -EAGAIN)
				continue;
			return -1;
		}
	}

	/* Close audio */

	r = snd_pcm_close(pcm);
	if (r < 0) {
		aerror("close", r);
		return -1;
	}

	/* Close codec */

	opus_decoder_destroy(decoder);
	
	/* Close networking */

	if (close(sd) == -1) {
		perror("close");
		return -1;
	}

	return 0;
}
