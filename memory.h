#ifndef MEMORY_H
#define MEMORY_H

#include "common.h"
 
#define MEMORY_SIZE (256*256) // 64k memory
u8 memory[MEMORY_SIZE];

u16 mem_read_u16(u16 addr);
u8 mem_read_u8(u16 addr);
void mem_write_u8(u16 addr, u8 value);

#endif
