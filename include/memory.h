#ifndef MEMORY_H
#define MEMORY_H

#include "common.h"
 
#define MEMORY_SIZE (256*256) // 64k memory

u8 memory[MEMORY_SIZE];

u16 mem_read_u16(u16 addr);
u8 mem_read_u8(u16 addr);
void mem_write_u8(u16 addr, u8 value);
void mem_write_u16(u16 addr, u16 value);
void mem_set_flag(u16 addr, u8 mask);
void mem_unset_flag(u16 addr, u8 mask);

#define ADDR_TILE_DATA1  (0x8000)
#define ADDR_TILE_DATA2  (0x8800)
#define ADDR_BGMAP1 (0x9800)
#define ADDR_BGMAP2 (0x9c00)

#define ADDR_SPRITE_RAM         (0xfe00) // ends fe9f (159 bytes)
#define ADDR_JOYPAD_INFO        (0xff00)
#define ADDR_SERIAL_TRANSFER    (0xff01)
#define ADDR_SIO_CONTROL        (0xff02)
#define ADDR_DIV_REGISTER       (0xff04)
#define ADDR_TIMER_COUNTER      (0xff05)
#define ADDR_TIMER_MODULO       (0xff06)
#define ADDR_TIMER_CONTROL      (0xff07)
#define ADDR_INTERRUPT_FLAGS    (0xff0f)

// SOUND GOES IN HERE
#define ADDR_LCD_CONTROL        (0xff40)
#define ADDR_LCD_STATUS         (0xff41)
#define ADDR_SCROLL_Y           (0xff42)
#define ADDR_SCROLL_X           (0xff43)
#define ADDR_LCDY_COORD         (0xff44)
#define ADDR_LCDY_COMPARE       (0xff45)
#define ADDR_DMA_TRANSFER       (0xff46)
#define ADDR_BG_PALLETTE        (0xff47)
#define ADDR_SPRITE_PALLETTE0   (0xff48)
#define ADDR_SPRITE_PALLETTE1   (0xff49)
#define ADDR_WINDOW_Y           (0xff4a)
#define ADDR_WINDOW_X           (0xff4b)
#define ADDR_INTERRUPT_ENABLE   (0xffff)


#define INTERRUPT_VBLANK_BIT    (0x01)
#define INTERRUPT_LCDC_BIT      (0x02)
#define INTERRUPT_TIMER_BIT     (0x04)
#define INTERRUPT_SERIAL_IO_BIT (0x08)
#define INTERRUPT_JOYPAD_BIT    (0x10)

#endif
