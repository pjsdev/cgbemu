#include <stdio.h>
#include "SDL.h"

#include "common.h"
#include "logging.h"
#include "memory.h"

#define FULL_SCREEN_WIDTH (256)
#define FULL_SCREEN_HEIGHT (256)
#define WINDOW_HEIGHT (144)
#define WINDOW_WIDTH (160)
#define FRAME_RENDER_CLOCKS (17556)
#define CLOCKS_PER_LINE (114)
#define PIXEL_COUNT (WINDOW_HEIGHT*WINDOW_WIDTH)

SDL_Window* window; 
SDL_Renderer* renderer;
SDL_Texture* texture;

int internal_clock = 0;
u8 prev_horizontal_clock = 0;
u8 prev_vertical_clock = 0;

typedef unsigned int Pixel;
Pixel pixels[PIXEL_COUNT];

void debug_display(){
    printf("---- Display -----");
    printf("Internal clock: %d\n", internal_clock);
    printf("PrevH: %d, PrevV: %d\n", prev_horizontal_clock, prev_vertical_clock);
}

u8 get_bg_color(u8 palette_index){
    u8 bg_palette = mem_read_u8(ADDR_BG_PALLETTE);
    // TODO use the actual BG palette instead of our hardcoded shades
    // we want this function to just return full u8 pixel colors
    switch(palette_index){
        case 0x00: return 0xff; //bg_palette & 0x03; // 00000011
        case 0x01: return 0xbb; //bg_palette & 0x0c; // 00001100
        case 0x02: return 0x66; //bg_palette & 0x30; // 00110000
        case 0x03: return 0x00; //bg_palette & 0xc0; // 11000000
        default:
            LOG("UNKNOWN BG COLOUR REQUESTED");
            return 0x00;
    }
}


void display_tick(int clocks){

    // only clock up if we are being displayed
    if ((mem_read_u8(ADDR_LCD_CONTROL) & 0x80) == 0){
        if (internal_clock > 0){                                                                            // turn off lcd
            LOG("Turning LCD Off");
            // we were showing before so turn off, resetting clocks
            // and setting black in SDL to mimic no image
            internal_clock = 0;
            prev_horizontal_clock = 0;
            prev_vertical_clock = 0;
            memset(pixels, 0, PIXEL_COUNT);
        }

        return;
    } else {
        // if we are turned on, and our previous clocks are 0
        // then we know that we JUST got turned on and have rendered no previous frames
        if (internal_clock == 0 && prev_vertical_clock == 0 && prev_horizontal_clock == 0){                 // turn on lcd
            LOG("Turning LCD On");
            memset(pixels, 0xffffffff, PIXEL_COUNT * sizeof(Pixel));
        }
    }
    
    int overflow = (internal_clock + clocks) - FRAME_RENDER_CLOCKS;
    if(overflow > 0){
        internal_clock = overflow; 
    } else {
        internal_clock += clocks;
    }

    u8 horizontal_clock = internal_clock % CLOCKS_PER_LINE; // it takes 114 clocks per line
    u8 vertical_clock = (internal_clock / CLOCKS_PER_LINE) + 1; // how many horizontal lines we've done +1

    mem_write_u8(ADDR_LCDY_COORD, vertical_clock);
   
    // TODO 
    // psmith Match 10 2017
    // set the lcd status register 0xFF41
    // trigger interrupts if selected
    // draw stuff, can probably draw the whole frame in the first 0-64 clocks...
    // could fake line by line rendering?

    // fake rendering at the beginning of a frame we just render all
    // TODO fake horizontal lines...
    if (prev_vertical_clock > 144 && vertical_clock < 144){
        SDL_UpdateTexture(texture, NULL, pixels, WINDOW_WIDTH * sizeof(Pixel));
        SDL_RenderClear(renderer);
        SDL_RenderCopy(renderer, texture, NULL, NULL);
        SDL_RenderPresent(renderer);
    }

    if (vertical_clock >= 145){                   // vblank
        if(prev_vertical_clock <= 144){
            // we just entered vblank
            mem_set_flag(ADDR_INTERRUPT_FLAGS, INTERRUPT_VBLANK_BIT);
        }
    } else if (horizontal_clock < 21){            // oam load
        if(prev_horizontal_clock >= 155){
            // TODO interrupt
        }
    } else if(horizontal_clock < 64){             // pixel transfer
        if(prev_horizontal_clock < 21){
            // TODO interrupt
        }
    } else {                                      // hblank
        if(prev_horizontal_clock <= 64){
            // TODO interrupt
        }
    }

    prev_horizontal_clock = horizontal_clock;
    prev_vertical_clock = vertical_clock;
}

void display_shutdown(){
    // Close and destroy the window
    SDL_DestroyTexture(texture);
    SDL_DestroyRenderer(renderer); 
    SDL_DestroyWindow(window);
}

int display_init(){
    // Create an application window with the following settings:
    window = SDL_CreateWindow(
        "cgbemu",                          // window title
        SDL_WINDOWPOS_UNDEFINED,           // initial x position
        SDL_WINDOWPOS_UNDEFINED,           // initial y position
        WINDOW_WIDTH,                      // width, in pixels
        WINDOW_HEIGHT,                     // height, in pixels
        SDL_WINDOW_SHOWN                   // flags - see below
    );

    // Check that the window was successfully created
    if (window == NULL) {
        // In the case that the window could not be made...
        printf("Could not create window: %s\n", SDL_GetError());
        return 0;
    }

    // SDL_SetWindowBordered(window, false);

    renderer = SDL_CreateRenderer(window, -1, 0);
    if (renderer == NULL) {
        printf("Could not create renderer: %s\n", SDL_GetError());
        return 0;
    }

    // TODO this should be 256*256 and use the window to look at a different rect of the texture
    texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888,
                                SDL_TEXTUREACCESS_STREAMING, WINDOW_WIDTH, WINDOW_HEIGHT);

    if (texture == NULL){
        printf("Could not create texture: %s\n", SDL_GetError());
        return 0;
    }
    
    SDL_UpdateTexture(texture, NULL, pixels, 160 * sizeof(unsigned int));

    printf("Window created...\n");
    return 1;
}

