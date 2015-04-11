#include <AudioToolbox/AudioToolbox.h>
#include <AudioUnit/AudioUnit.h>
#include <Carbon/Carbon.h>
#include <CoreAudio/CoreAudio.h>
#include <CoreServices/CoreServices.h>
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
#define defaultBufferTime 25 // milliseconds
#define defaultPort "1350"


int main(int argc, const char * argv[]) {
    
    int sd, r;
    ssize_t bytes;
    struct addrinfo hints, *servinfo, *p;
    PaError err;
    PaStream *stream;
    OpusEncoder *encoder;
    
    /* options, and variables for calling getopt */
    int c;
    int bytes_per_frame = defaultBytesPerFrame;
    int channels = defaultChannels;
    opus_int32 rate = defaultSampleRate;
    int frame = defaultFrameSize;
    int buffer_time = defaultBufferTime;
    char *port = defaultPort;
    
    int verbosity = 1;
    
    fputs(":: [OB ONE] ::\n", stderr);
    fputs("TX: George O'Neill, 2015\n", stderr);
    
    while ((c = getopt (argc, argv, "b:B:c:f:p:qr:v")) != -1) {
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
    
    if (argc-optind != 1) {
        fputs("usage: tx [options] <hostname>\n", stderr);
        return -1;
    }
    
    printf("\nTX destination: \t%s:%s\n",argv[optind],port);
    printf("Channels: \t\t%d\n",channels);
    printf("Sample rate: \t\t%d Hz\n",rate);
    printf("Packet length: \t\t%d ms\n",1000 * frame/rate);
    
    /* these declarations moved down here, because the size depends on the options */
    char* abuf[frame * channels * sizeof(opus_int16)];
    char encoded[bytes_per_frame];
    
    printf("Preparing tranmission:\n");
    
    /* Prepare networking, bind to the sockets */
    
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
    
    /* Intiate audio harcdware */
    
    err = Pa_Initialize();
    if (err != paNoError) {
        printf("PortAudio failed to initialise: %s\n", Pa_GetErrorText(r));
        return -1;
    }
    
    err = Pa_OpenDefaultStream(&stream, channels, 0, paInt16, rate, frame, NULL, NULL);
    if (err != paNoError) {
        printf("PortAudio failed to open stream: %s\n", Pa_GetErrorText(r));
        return -1;
    }
    
    /* Initiate Opus */
    
    encoder = opus_encoder_create(rate, channels, OPUS_APPLICATION_AUDIO, &r);
    if (r < 0 ) {
        fprintf(stderr,"Failed to initialise Opus codec: %s\n", opus_strerror(r));
        return -1;
    }
    
    err = Pa_StartStream(stream);
    if (err != paNoError) {
        printf("PortAudio failed to start stream: %s\n", Pa_GetErrorText(r));
        return -1;
    }
    
    /* Tranmission Loop */
    
    printf("Tranmission started: Press ctrl + C to abort\n\n");
    for(;;) {
        
        err = Pa_ReadStream(stream, abuf, frame);
        if (err != paNoError) {
            printf("PortAudio failed to read stream: %s\n", Pa_GetErrorText(r));
            return -1;
        }
        
        r = opus_encode(encoder, abuf, frame, encoded, bytes_per_frame * channels);
        if (r < 0) {
            fprintf(stderr, "Error encoding PCM to Opus: %s\n", opus_strerror(r));
            return -1;
        }
        
        bytes = sendto(sd, encoded, r, 0,
                       p->ai_addr, p->ai_addrlen);
        if (bytes == -1) {
            perror("sendto");
            return -1;
        }else{
            printf("t");
            fflush(stdout);
        }
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
