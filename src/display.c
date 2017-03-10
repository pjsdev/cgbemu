#include <stdio.h>
#include "SDL.h"

#include "common.h"
#include "logging.h"
#include "memory.h"

#define FRAME_RENDER_CLOCKS (17556)
#define CLOCKS_PER_LINE (114)

SDL_Window* window; 
SDL_Surface* screenSurface = NULL;

int internal_clock = 0;

void display_tick(int clocks){

    // only clock up if we are being displayed
    if ((mem_read_u8(ADDR_LCD_CONTROL) & 0x80) == 0){
        if (internal_clock > 0){
            // we were showing before so turn off
            internal_clock = 0;
            SDL_FillRect( screenSurface, NULL, SDL_MapRGB( screenSurface->format, 0x00, 0x00, 0x00 ) );
        }
        return;
    }

    u8 prev_horizontal_clock = internal_clock % CLOCKS_PER_LINE; // it takes 114 clocks per line
    u8 prev_vertical_clock = (internal_clock / CLOCKS_PER_LINE) + 1; // how many horizontal lines we've done +1

    int overflow = (internal_clock + clocks) - FRAME_RENDER_CLOCKS;
    if(overflow > 0){
        internal_clock = overflow; 
    } else {
        internal_clock += clocks;
    }

    u8 horizontal_clock = internal_clock % CLOCKS_PER_LINE; // it takes 114 clocks per line
    u8 vertical_clock = (internal_clock / CLOCKS_PER_LINE) + 1; // how many horizontal lines we've done +1

    mem_write_u8(ADDR_LCDY_COORD, vertical_clock);
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
