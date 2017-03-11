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

void debug_display(){
    printf("---- Display -----");
    printf("Internal clock: %d\n", internal_clock);
    printf("PrevH: %d, PrevV: %d\n", prev_horizontal_clock, prev_vertical_clock);
}

Pixel get_bg_pixel(u8 palette_index){
    u8 bg_palette = mem_read_u8(ADDR_BG_PALLETTE);
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

u16 pick_bit(u16 data, int bit_in, int bit_out){
    // mask off other bits
    // shift the bit you wanted to the 0th position
    // shift it back to the position you wanted
    u16 masked = (data & (1 << bit_in));
    return ((masked >> bit_in) << bit_out);
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
        // TODO handle scroll 
        // u16 start_x = mem_read_u16(ADDR_SCROLL_X); 
        // u16 start_y = mem_read_u16(ADDR_SCROLL_Y); 

        Pixel pixel_no = 0;
        for(u8 i = 0; i < 32; i++){
            u8 base = i * 32;
            for(u8 j = 0; j < 32; j++){
                u8 idx = base + j;
                // e.g. if we are looking for tile 6, look 96 bytes into tile data
                u8 bg_tile_index = mem_read_u8(ADDR_BGMAP1 + idx) * 16; // 16 == the number of bytes in each tile
                u16 tile_addr = ADDR_TILE_DATA1 + bg_tile_index;

                // extract it into all_pixels
                for(int t = 0; t < 8; t++){ // for all 8 lines
                    u16 line = mem_read_u16(tile_addr + (t * 2));
                    u8 p1_col = (u8)(pick_bit(line, 7, 0) | pick_bit(line, 15, 1));
                    u8 p2_col = (u8)(pick_bit(line, 6, 0) | pick_bit(line, 14, 1));
                    u8 p3_col = (u8)(pick_bit(line, 5, 0) | pick_bit(line, 13, 1));
                    u8 p4_col = (u8)(pick_bit(line, 4, 0) | pick_bit(line, 12, 1));
                    u8 p5_col = (u8)(pick_bit(line, 3, 0) | pick_bit(line, 11, 1));
                    u8 p6_col = (u8)(pick_bit(line, 2, 0) | pick_bit(line, 10, 1));
                    u8 p7_col = (u8)(pick_bit(line, 1, 0) | pick_bit(line, 9, 1));
                    u8 p8_col = (u8)(pick_bit(line, 0, 0) | pick_bit(line, 8, 1));

                    assert(p1_col == 0);
                    assert(p2_col == 0);
                    assert(p3_col == 0);
                    assert(p4_col == 0);
                    assert(p5_col == 0);
                    assert(p6_col == 0);
                    assert(p7_col == 0);
                    assert(p8_col == 0);


                    //LOG("1: 0x%02x, 2: 0x%02x, 3: 0x%02x, 4: 0x%02x, 5: 0x%02x, 6: 0x%02x, 7: 0x%02x, 8: 0x%02x\n", 
                    //    p1_col, p2_col, p3_col, p4_col, p5_col, p6_col, p7_col, p8_col);

                    // LOG("Setting 8 pixels from %d\n", pixel_no);
                    all_pixels[pixel_no++] = get_bg_pixel(p1_col);
                    all_pixels[pixel_no++] = get_bg_pixel(p2_col);
                    all_pixels[pixel_no++] = get_bg_pixel(p3_col);
                    all_pixels[pixel_no++] = get_bg_pixel(p4_col);
                    all_pixels[pixel_no++] = get_bg_pixel(p5_col);
                    all_pixels[pixel_no++] = get_bg_pixel(p6_col);
                    all_pixels[pixel_no++] = get_bg_pixel(p7_col);
                    all_pixels[pixel_no++] = get_bg_pixel(p8_col);
                }
            }
        }

        //for(int i = 0; i < FULL_SCREEN_WIDTH; i++){
        //    int base = i * FULL_SCREEN_WIDTH;
        //    for(int j = 0; j < FULL_SCREEN_HEIGHT; j++){
        //        printf("%d: %d ", base + j, all_pixels[base + j]);
        //    }
        //    printf("\n");
        //}

        LOG("Updated %d pixels", pixel_no);
        SDL_UpdateTexture(texture, NULL, all_pixels, FULL_SCREEN_WIDTH * sizeof(Pixel));
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

