#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>
#include <assert.h>
#include <string.h>

#include "common.h"
#include "file.h"
#include "memory.h"
#include "cpu.h"


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
    registers.PC = 0;

    /* test mem_read_u16
    assert(memory[registers.PC++] == 0x31);
    assert(mem_read_u16(registers.PC) == 0xfffe);
    */


    /* test load_into_addr_from_r8 && hl--   (0x32)
    registers.HL = 0x00ff; // addr
    registers.A = 0x45;
    cpu_do_instruction(0x32);

    assert(memory[0x00ff] == 0x45);
    assert(registers.HL == 0x00fe);
    */

    while(1)
    {
        // TODO interrupts
        cpu_do_instruction(memory[registers.PC++]);
        // total_clock.m += tick_clock.m;
        // total_clock.t += tick_clock.t;

        if (registers.PC > 255) break;
    }

    printf("done...");
    return 0;
}

