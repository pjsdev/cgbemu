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

    // setup memory with the boot rom
    const char* filename = "data/DMG_ROM.bin";
    u8_buffer* boot_rom = read_binary_file(filename);
    printf("\n");
    print_u16_chunks(boot_rom);
    assert(boot_rom->size == 256);
   
    memcpy(memory, boot_rom->data, boot_rom->size);
    free_u8_buffer(boot_rom);
    assert(memory[255] == 0x50);

    // point to beginning of boot rom
    cpu_registers.PC = 0;
    
    /* test mem_read_u16
    assert(memory[cpu_registers.PC++] == 0x31);
    assert(mem_read_u16(cpu_registers.PC) == 0xfffe);
    */

    /* test load_into_addr_from_r8 && hl--   (0x32)
    cpu_registers.HL = 0x00ff; // addr
    cpu_registers.A = 0x45;
    cpu_do_instruction(0x32);

    assert(memory[0x00ff] == 0x45);
    assert(cpu_registers.HL == 0x00fe);
    */

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
        cpu_do_instruction(memory[cpu_registers.PC++]);
        
        cpu_total_clock.m += cpu_tick_clock.m;
        cpu_total_clock.t += cpu_tick_clock.t;

        if (cpu_registers.PC > 255) break;
    }

    printf("done...");
    return 0;
}

