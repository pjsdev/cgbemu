#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>
#include <assert.h>
#include <string.h>

#include "system.h"
#include "common.h"
#include "file.h"
#include "memory.h"
#include "cpu.h"
#include "logging.h"


int main(){
    // http://gbdev.gg8.se/wiki/articles/Gameboy_Bootstrap_ROM#Contents_of_the_ROM 
    // printf("%zu\n", sizeof(memory)); 
    // it looks like we map the cartridge ROM immediately then run bios...?

    // TODO command line args for debug mode that will disable audio/graphics and
    // allow breakpoints whilst dumping instructions... - psmith march 9 2017
    // if(!system_init()) return 1;

    // load cartridge into memory
    const char* cartridge_path = "data/Tetris_World.gb";
    cartridge = read_binary_file(cartridge_path);
    assert(cartridge->size == 32768);
    memcpy(memory, cartridge->data, cartridge->size); 
    assert(memory[0x0101] == 0xc3);
    
    debug_print_cartridge_header();

    // setup memory with the boot rom
    const char* filename = "data/DMG_ROM.bin";
    u8_buffer* boot_rom = read_binary_file(filename);
    printf("\n");
    assert(boot_rom->size == 256);
   
    memcpy(memory, boot_rom->data, boot_rom->size); 
    assert(memory[255] == 0x50);

    // point to beginning of boot rom
    cpu_registers.PC = 0;
    
    running = 1;
    while(running){

        if (cpu_interrupt_master_enable){
            char interrupts = memory[ADDR_INTERRUPT_FLAGS] & memory[ADDR_INTERRUPT_ENABLE];

            if (interrupts & INTERRUPT_VBLANK_BIT){
                LOG("Interrupt Vblank");

            }
            if (interrupts & INTERRUPT_LCDC_BIT){
                LOG("Interrupt LCDC");

            }
            if (interrupts & INTERRUPT_TIMER_BIT){
                LOG("Interrupt Timer");
            
            }
            if (interrupts & INTERRUPT_SERIAL_IO_BIT){
                LOG("Interrupt SerialIO");
            
            }
            if (interrupts & INTERRUPT_JOYPAD_BIT){
                LOG("Interrupt Joypad");
            
            }
        }

        printf("(0x%04x)\t", cpu_registers.PC);
        u8 opcode = memory[cpu_registers.PC++];
        cpu_do_instruction(opcode);
        
        cpu_total_clock.m += cpu_tick_clock.m;
        cpu_total_clock.t += cpu_tick_clock.t;

        if (cpu_registers.PC == 0x0028){
            BREAK;
        }

        debug_tick();
        system_tick();
    }

    printf("\n");
    print_u16_chunks(boot_rom);
    free_u8_buffer(cartridge);
    free_u8_buffer(boot_rom);

    // system_shutdown();
    return 0;
}
