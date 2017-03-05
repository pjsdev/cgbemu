#include "common.h"
#include "memory.h"

void mem_write_u8(u16 addr, u8 value){

}

u16 mem_read_u16(u16 addr){
    // note: the gameboy was little endian, so we shift the second byte
    // to the left then add the first byte
    return ((u16)memory[addr + 1] << 8) + memory[addr];
}

u8 mem_read_u8(u16 addr){
    return memory[addr];
}
