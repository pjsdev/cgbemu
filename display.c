#include <stdio.h>

#include "SDL.h"


SDL_Window *window;                    // Declare a pointer
SDL_Surface* screenSurface = NULL;

int display_init(){
    if (SDL_Init(SDL_INIT_VIDEO) < 0){
        printf("SDL Failed to INIT: %s\n", SDL_GetError());
        return 0;
    }

    // Create an application window with the following settings:
    window = SDL_CreateWindow(
        "cgbemu",                          // window title
        SDL_WINDOWPOS_UNDEFINED,           // initial x position
        SDL_WINDOWPOS_UNDEFINED,           // initial y position
        160,                               // width, in pixels
        144,                               // height, in pixels
        SDL_WINDOW_SHOWN                   // flags - see below
    );

    // Check that the window was successfully created
    if (window == NULL) {
        // In the case that the window could not be made...
        printf("Could not create window: %s\n", SDL_GetError());
        return 0;
    }

    
    //Get window surface
    screenSurface = SDL_GetWindowSurface( window );

    //Fill the surface white
    SDL_FillRect( screenSurface, NULL, SDL_MapRGB( screenSurface->format, 0xFF, 0xFF, 0xFF ) );
    
    //Update the surface
    SDL_UpdateWindowSurface( window );

    printf("Window created...");
    return 1;
}

void display_tick(){
}

void display_shutdown(){
    // Close and destroy the window
    SDL_DestroyWindow(window);

    // Clean up
    SDL_Quit();
}


