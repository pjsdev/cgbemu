#ifndef CPU_H
#define CPU_H

typedef struct {
    union {
        struct {u8 A; u8 F;};
        u16 AF;
    };
    union {
        struct {u8 B; u8 C;};
        u16 BC;
    };
    union {
        struct {u8 D;u8 E;};
        u16 DE;
    };
    union {
        struct {u8 H;u8 L;};
        u16 HL;
    };

    u16 SP;
    u16 PC;
} Registers;

typedef struct {
    int m;
    int t;
} Clock;

Clock tick_clock;
Clock total_clock;
Registers registers;

void cpu_do_instruction(u8 opcode);
void cpu_print_registers();
void cpu_run_tests();

#endif
