#include <stdio.h>
#include <math.h>

#include "SDL.h"
#include "display.h"
#include "sound.h"

int system_init(){
    if( SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER | SDL_INIT_AUDIO ) <0 ) {
        printf("Unable to init SDL: %s\n", SDL_GetError());
        return 0;
    }

    if(!display_init()){
        printf("Unable to init SDL display: %s\n", SDL_GetError());
        return 0;
    }
    
    if(!sound_init()){
        printf("Unable to init SDL display: %s\n", SDL_GetError());
        return 0;
    }

    return 1;
}

void system_shutdown(){
    display_shutdown();
    sound_shutdown();

    // Clean up
    SDL_Quit();
}

