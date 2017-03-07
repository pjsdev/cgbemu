#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>
#include <assert.h>
#include <string.h>

#include "common.h"
#include "file.h"
#include "memory.h"
#include "cpu.h"
#include "logging.h"

bool running = true;

int main(){
    // http://gbdev.gg8.se/wiki/articles/Gameboy_Bootstrap_ROM#Contents_of_the_ROM 
    // printf("%zu\n", sizeof(memory)); 

    // TODO it seems like the BIOS is not mapped into memory but instead a dedicated zone
    // it looks like we map the cartridge ROM immediately then run bios...?
    
    // setup memory with the boot rom
    const char* filename = "data/DMG_ROM.bin";
    u8_buffer* boot_rom = read_binary_file(filename);
    printf("\n");
    assert(boot_rom->size == 256);
   
    memcpy(memory, boot_rom->data, boot_rom->size); 
    assert(memory[255] == 0x50);

    // point to beginning of boot rom
    cpu_registers.PC = 0;
    
    while(running){

        debug_tick();

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

        if (cpu_registers.PC == 33){
            BREAK;
        }
    }

    printf("\n");
    print_u16_chunks(boot_rom);
    free_u8_buffer(boot_rom);
    return 0;
}

