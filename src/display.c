#include <stdio.h>
#include "SDL.h"

#include "memory.h"

SDL_Window* window;                    // Declare a pointer
SDL_Surface* screenSurface = NULL;

void display_tick(int clocks){

}

void display_shutdown(){
    // Close and destroy the window
    SDL_DestroyWindow(window);
}

int display_init(){
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

    SDL_SetWindowBordered(window, false);

    //Get window surface
    screenSurface = SDL_GetWindowSurface( window );

    //Fill the surface white
    SDL_FillRect( screenSurface, NULL, SDL_MapRGB( screenSurface->format, 0x00, 0x00, 0x00 ) );
    
    //Update the surface
    SDL_UpdateWindowSurface( window );

    printf("Window created...\n");
    return 1;
}
