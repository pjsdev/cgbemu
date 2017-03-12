#include <assert.h>
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

#define PIXEL_COUNT (FULL_SCREEN_WIDTH * FULL_SCREEN_HEIGHT)
#define WINDOW_PIXEL_COUNT (WINDOW_WIDTH * WINDOW_HEIGHT)

SDL_Window* window; 
SDL_Renderer* renderer;
SDL_Texture* texture;

int internal_clock = 0;
u8 prev_horizontal_clock = 0;
u8 prev_vertical_clock = 0;

typedef unsigned int Pixel;

Pixel window_pixels[WINDOW_PIXEL_COUNT];
Pixel all_pixels[PIXEL_COUNT];

u8 pick_bit(u8 data, int bit_in, int bit_out){
    // mask off other bits
    // shift the bit you wanted to the 0th position
    // shift it back to the position you wanted
    u16 masked = (data & (1 << bit_in));
    return ((masked >> bit_in) << bit_out);
}

Pixel get_bg_pixel(u8 palette_index){
    // u8 bg_palette = mem_read_u8(ADDR_BG_PALLETTE);
    // TODO use the actual BG palette instead of our hardcoded shades
    // we want this function to just return full u8 pixel colors
    switch(palette_index){
        case 0x00: return 0xffffffff; //bg_palette & 0x03; // 00000011
        case 0x01: return 0xffbbbbbb; //bg_palette & 0x0c; // 00001100
        case 0x02: return 0xff666666; //bg_palette & 0x30; // 00110000
        case 0x03: return 0xff000000; //bg_palette & 0xc0; // 11000000
        default:
            LOG("UNKNOWN BG COLOUR REQUESTED");
            return 0xff000000;
    }
}

void debug_display(){
    printf("---- Display -----");
    printf("Internal clock: %d\n", internal_clock);
    printf("PrevH: %d, PrevV: %d\n", prev_horizontal_clock, prev_vertical_clock);
}

void render_frame(){
    // TODO handle scroll 
    // u16 start_x = mem_read_u16(ADDR_SCROLL_X); 
    // u16 start_y = mem_read_u16(ADDR_SCROLL_Y); 

    u8 TILE_SIZE = 16;
    u8 TILE_WIDTH = 8;
    u16 tile_index_base;
    u16 tile_data_base = ADDR_TILE_DATA1;
    u16 bg_idx;
    u16 tile_data_addr;
    u8 tile_idx;
    u8 msb;
    u8 lsb;
    u8 shift;
    u8 palette_idx;
    u16 y;
    u16 x;

    // for each pixel line
    for(y = 0; y < FULL_SCREEN_HEIGHT; y++){
        // where to look for this pixels tile, the tile map is 32 wide
        // and each tile is 8 pixels e.g.
        // (0 / 8) * 32 == 0 (first line)
        // (8 / 8) * 32 == 1 (8th pixel down, start of 2nd tile down)
        tile_index_base = ADDR_BGMAP1 + (y / TILE_WIDTH) * 32;

        // each line is 2 bytes long so each y we go down, we have to offset 2 more bytes e.g.
        // (0 % 8) * 2 == 0 (first line data starts at 0)
        // (4 % 8) * 2 == 8 (4th pixel line starts 8 bytes into tile)
        tile_data_base = ADDR_TILE_DATA1 + (y % TILE_WIDTH) * (TILE_SIZE / TILE_WIDTH);

        // for each pixel 
        for(x = 0; x < FULL_SCREEN_WIDTH;  x++){

            // the index of this pixel squashed down into the 8 wide tile
            bg_idx = tile_index_base + (x / TILE_WIDTH);

            // the tile for this pixel
            tile_idx = mem_read_u8(bg_idx);

            // each tile is 16 bytes long so look at our lines tiles plus some lines in
            tile_data_addr = tile_data_base + tile_idx * TILE_SIZE;
            
            // read the two bytes
            msb = mem_read_u8(tile_data_addr);
            lsb = mem_read_u8(tile_data_addr + 1);

            // the shift is the position within the 8 pixel tile (-1)
            shift = TILE_WIDTH - (x % TILE_WIDTH) - 1;

            // pick the two bits and compile into our 2 bit palette id
            palette_idx = pick_bit(msb, shift, 1) | pick_bit(lsb, shift, 0);
            all_pixels[(y * FULL_SCREEN_WIDTH) + x] = get_bg_pixel(palette_idx);
        }

    }

    SDL_UpdateTexture(texture, NULL, all_pixels, FULL_SCREEN_WIDTH * sizeof(Pixel));
    SDL_RenderClear(renderer);
    SDL_RenderCopy(renderer, texture, NULL, NULL);
    SDL_RenderPresent(renderer);
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
            memset(all_pixels, 0, PIXEL_COUNT * sizeof(Pixel));
        }

        return;
    } else {
        // if we are turned on, and our previous clocks are 0
        // then we know that we JUST got turned on and have rendered no previous frames
        if (internal_clock == 0 && prev_vertical_clock == 0 && prev_horizontal_clock == 0){                 // turn on lcd
            LOG("Turning LCD On");
            memset(all_pixels, 0xffffffff, PIXEL_COUNT * sizeof(Pixel));
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
        render_frame();
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
    // TODO windowing instead of showing whole texture
    window = SDL_CreateWindow(
        "cgbemu",                          // window title
        SDL_WINDOWPOS_UNDEFINED,           // initial x position
        SDL_WINDOWPOS_UNDEFINED,           // initial y position
        FULL_SCREEN_WIDTH, // WINDOW_WIDTH,                      // width, in pixels
        FULL_SCREEN_HEIGHT, //WINDOW_HEIGHT,                     // height, in pixels
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

    texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888,
                                SDL_TEXTUREACCESS_STREAMING,
                                FULL_SCREEN_WIDTH, FULL_SCREEN_HEIGHT);

    if (texture == NULL){
        printf("Could not create texture: %s\n", SDL_GetError());
        return 0;
    }
    
    SDL_UpdateTexture(texture, NULL, all_pixels, FULL_SCREEN_WIDTH * sizeof(unsigned int));

    printf("Window created...\n");
    return 1;
}

