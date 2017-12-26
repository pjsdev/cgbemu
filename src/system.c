#include <stdio.h>
#include <math.h>

#include "SDL.h"
#include "display.h"
#include "common.h"
#include "sound.h"

SDL_Event event;

void system_tick()
{
    while(SDL_PollEvent(&event))
    {
        if(event.type == SDL_QUIT || event.type == SDL_WINDOWEVENT_CLOSE)
        {
            running = 0;
            return;
        }

        if(event.type == SDL_KEYDOWN)
        {
            if(event.key.keysym.sym == SDLK_F1)
            {
                display_cycle_window_mode();
            }
        }
    }

}

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

