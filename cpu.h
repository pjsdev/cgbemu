#ifndef CPU_H
#define CPU_H

#include "common.h"

typedef struct {
    // note: the anonymous structs are reversed for little endianness...
    // maybe I should handle both?
    union {
        struct {u8 F; u8 A;};
        u16 AF;
    };
    union {
        struct {u8 C; u8 B;};
        u16 BC;
    };
    union {
        struct {u8 E; u8 D;};
        u16 DE;
    };
    union {
        struct {u8 L; u8 H;};
        u16 HL;
    };

    u16 SP;
    u16 PC;
} Registers;

typedef struct {
    int m;
    int t;
} Clock;

Clock cpu_tick_clock;
Clock cpu_total_clock;
Registers cpu_registers;

void cpu_do_instruction(u8 opcode);
void cpu_print_registers();
void cpu_run_tests();

char cpu_interrupt_master_enable;

#endif
