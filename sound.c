#include <stdbool.h>
#include <stdio.h>
#include <math.h>
#include "SDL.h"

unsigned int sampleFrequency = 0;
unsigned int audioBufferSize = 0;
unsigned int outputAudioBufferSize = 0;

unsigned int freq1 = 1000;
unsigned int fase1 = 0;
unsigned int freq2 = 5000;
unsigned int fase2 = 0;

void example_mixaudio(void *unused, Uint8 *stream, int len) {

    unsigned int bytesPerPeriod1 = sampleFrequency / freq1;
    unsigned int bytesPerPeriod2 = sampleFrequency / freq2;

    for (int i=0;i<len;i++) {
        int channel1 = 150*sin(fase1*6.28/bytesPerPeriod1);
        int channel2 = 150*sin(fase2*6.28/bytesPerPeriod2);

        int outputValue = channel1 + channel2;           // just add the channels
        if (outputValue > 127) outputValue = 127;        // and clip the result
        if (outputValue < -128) outputValue = -128;      // this seems a crude method, but works very well

        stream[i] = outputValue;

        fase1++;
        fase1 %= bytesPerPeriod1;
        fase2++;
        fase2 %= bytesPerPeriod2;
    }
}

int sound_init(){
    if( SDL_Init(SDL_INIT_TIMER | SDL_INIT_AUDIO ) <0 ) {
        printf("Unable to init SDL Audio: %s\n", SDL_GetError());
        return 1;
    }

    // setup audio
    SDL_AudioSpec *desired, *obtained;

    // Allocate a desired SDL_AudioSpec
    desired = (SDL_AudioSpec *) malloc(sizeof(SDL_AudioSpec));

    // Allocate space for the obtained SDL_AudioSpec
    obtained = (SDL_AudioSpec *) malloc(sizeof(SDL_AudioSpec));

    // choose a samplerate and audio-format
    desired->freq = 44100;
    desired->format = AUDIO_S8;

    // Large audio buffers reduces risk of dropouts but increases response time.
    desired->samples = 4096;

    // Our callback function
    desired->callback=example_mixaudio;
    desired->userdata=NULL;

    desired->channels = 1;

    // Open the audio device and start playing sound!
    if ( SDL_OpenAudio(desired, obtained) < 0 ) {
        fprintf(stderr, "AudioMixer, Unable to open audio: %s\n", SDL_GetError());
        exit(1);
    }

    audioBufferSize = obtained->samples;
    sampleFrequency = obtained->freq;

    // if the format is 16 bit, two bytes are written for every sample
    if (obtained->format==AUDIO_U16 || obtained->format==AUDIO_S16) {
        outputAudioBufferSize = 2*audioBufferSize;
    } else {
        outputAudioBufferSize = audioBufferSize;
    }

    SDL_PauseAudio(0);
    return 1;
}

void sound_tick(){
}

void sound_shutdown(){
    SDL_CloseAudio();
}


