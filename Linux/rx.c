#include <alsa/asoundlib.h>
#include <netdb.h>
#include <portaudio.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <opus/opus.h>
#include <netinet/in.h>

#define defaultBytesPerFrame 96
#define defaultChannels 2
#define defaultSampleRate 48000
#define defaultFrameSize 480
#define defaultBufferTime 50 // milliseconds
#define defaultPort "1350"
#define defaultDevice 0 

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
    ssize_t bytes;
    size_t addr_len;
    struct addrinfo hints, *servinfo, *p;
    struct sockaddr_storage their_addr;
    PaError err;
    PaStream *stream;
    PaStreamParameters outputParameters;
    OpusDecoder *decoder;
    
    /* options, and variables for calling getopt */
    int c;
    int bytes_per_frame = defaultBytesPerFrame;
    int channels = defaultChannels;
    opus_int32 rate = defaultSampleRate;
    int frame = defaultFrameSize;
    PaTime buffer_time = defaultBufferTime;
    int device = defaultDevice;
    char *port = defaultPort;
    
    int verbosity = 1;
    
    fputs(":: [OB ONE] ::\n", stderr);
    fputs("RX: George O'Neill, 2015\n", stderr);
    
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
                device = atoi(optarg);
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
    opus_int16 abuf[frame * channels *sizeof(opus_int16)];
    char encoded[DEFAULT_MAX_ENCODED_BYTES];
    
    if (argc-optind != 0) {
        fputs("usage: rx [options]\n", stderr);
        return -1;
    }
    
    printf("Listening on port %s: establishing connection",port);
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
    
    /* Prepare the audio device via portaudio*/

    err = Pa_Initialize();
    if (err != paNoError) {
        printf("PortAudio failed to initialise: %s\n", Pa_GetErrorText(err));
        return -1;
    }
    
    if(device !=0) { 
        outputParameters.device = device; 
    } else {
        outputParameters.device = Pa_GetDefaultOutputDevice();
    }
    outputParameters.channelCount = channels;
    outputParameters.sampleFormat = paInt16;
    outputParameters.suggestedLatency = buffer_time / 1000;
    outputParameters.hostApiSpecificStreamInfo = NULL;
    
    err = Pa_OpenStream(&stream, NULL, &outputParameters, rate, frame, paNoFlag, NULL, NULL);
    if (err != paNoError) {
        printf("PortAudio failed to open stream: %s\n", Pa_GetErrorText(err));
        return -1;
    }
    
    err = Pa_StartStream(stream);
    if (err != paNoError) {
        printf("PortAudio failed to start stream: %s\n", Pa_GetErrorText(err));
        return -1;
    }

    
    /* Main loop */
    printf("RXing, press ctrl + C to abort\n");
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
                fprintf(stderr, "%s: %zd bytes\n",
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
        
        err = Pa_WriteStream(stream, abuf, frame);
        if (err != paNoError) {
            printf(".");
            continue;
        }
        fflush(stderr);
    }
    
    /* Close audio, networking */
    
    //    opus_encoder_destroy(encoder);
    //
    //    freeaddrinfo(servinfo);
    //
    //    if (close(sd) == -1) {
    //        perror("close");
    //        return -1;
    //    }
    //
    //    err = Pa_Terminate();
    //    if (err != paNoError) {
    //        printf("PortAudio failed to shut down: %s\n", Pa_GetErrorText(r));
    //        return -1;
    //    }
    
    return 0;
}
