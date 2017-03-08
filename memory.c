#include <stdio.h>
 
#include "common.h"
#include "memory.h"

void mem_write_u16(u16 addr, u16 value){
    memory[addr + 1] = (u8)(value >> 8) & 0x00ff;
    memory[addr] = (u8)(value & 0x00ff);
}

void mem_write_u8(u16 addr, u8 value){
    // printf("Writing u8 (0x%02x) to 0x%04x ", value, addr);
    memory[addr] = value;
}

u16 mem_read_u16(u16 addr){
    // note: the gameboy was little endian, so we shift the second byte
    // to the left then add the first byte
    return ( ((u16)memory[addr + 1]) << 8) + memory[addr]; 
}

u8 mem_read_u8(u16 addr){
    return memory[addr];
}
