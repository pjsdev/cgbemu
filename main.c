#include <stdio.h>
#include <string.h>


/*
Zero (0x80):        Set if the last operation produced a result of 0;
Operation (0x40):   Set if the last operation was a subtraction;
Half-carry (0x20):  Set if, in the result of the last operation, the lower half of the byte overflowed past 15;
Carry (0x10):       Set if the last operation produced a result over 255 (for additions) or under 0 (for subtractions).
*/
#define FLAG_MASK_Z (0x80) 
#define FLAG_MASK_N (0x40) 
#define FLAG_MASK_H (0x20) 
#define FLAG_MASK_C (0x10) 

#define MEMORY_SIZE (256*256) // 64k memory

typedef unsigned char u8;
typedef unsigned short u16;

typedef struct {
    union {
        struct {
            u8 A;
            u8 F;
        };

        u16 AF;
    };
    union {
        struct {
            u8 B;
            u8 C;
        };

        u16 BC;
    };
    union {
        struct {
            u8 D;
            u8 E;
        };

        u16 DE;
    };
    union {
        struct {
            u8 H;
            u8 L;
        };

        u16 HL;
    };

    u16 SP;
    u16 PC;
} Registers;

Registers registers;
u8 memory[MEMORY_SIZE];

void print_registers()
{
    printf(" --- Registers ---\n");
    printf(" A: 0x%02X ", registers.A);
    printf(" F: 0x%02X\n", registers.F);
    printf(" B: 0x%02X ", registers.B);
    printf(" C: 0x%02X\n", registers.C);
    printf(" D: 0x%02X ", registers.D);
    printf(" E: 0x%02X\n", registers.E);
    printf(" H: 0x%02X ", registers.H);
    printf(" L: 0x%02X\n", registers.L);
    printf("SP: 0x%02X ", registers.SP);
    printf("PC: 0x%02X\n", registers.PC);
    printf(" ----------------\n");
}

void increment_r16(u16* operand)
{
    // no flags
    (*operand)++;
}

void decrement_r8(u8* operand)
{
    registers.F = 0;

    if (*operand == 0x00)
    {
        registers.F |= FLAG_MASK_Z;
        registers.F |= FLAG_MASK_C;
    }

    (*operand)--;
}

void increment_r8(u8* operand)
{
    registers.F = 0;
   
    if (*operand == 0xff)
    {
        // we are about to produce a 0
        // we are about to carry
        registers.F |= FLAG_MASK_Z;
        registers.F |= FLAG_MASK_C;
    }

    (*operand)++;
}

void load_r8(u8* lhs, u8* rhs)
{
    *lhs = *rhs;
}

void do_instruction(u8 instruction)
{
    switch(instruction)
    {
        case 0x00: break; // NOP
        case 0x01: break; // LD BC, d16
        case 0x02: break; // LD (BC), A
        case 0x03: break; // INC BC
        case 0x04: increment_r8(&registers.B); break; // INC B
        case 0x05: break; // DEC B
        case 0x06: break; // LD B, d8
        case 0x07: break; // RLCA
        case 0x08: break; // LD (a16), SP
        case 0x09: break; // ADD HL, BC
        case 0x0a: break; // LD A, (BC)
        case 0x0b: break; // DEC BC
        case 0x0c: increment_r8(&registers.C); break; // INC C
        case 0x0d: break; // DEC C
        case 0x0e: break; // LD C, d8
        case 0x0f: break; // RRCA
        case 0x10: break; // STOP 0
        case 0x11: break; // LD DE, d16
        case 0x12: break; // LD (DE), A
        case 0x13: break; // INC DE
        case 0x14: break; // INC D
        case 0x15: break; // DEC D
        case 0x16: break; // LD D, d8
        case 0x17: break; // RLA
        case 0x18: break; // JR r8
        case 0x19: break; // ADD HL, DE
        case 0x1a: break; // LD A, (DE)
        case 0x1b: break; // DEC DE
        case 0x1c: break; // INC E
        case 0x1d: break; // DEC E
        case 0x1e: break; // LD E, d8
        case 0x1f: break; // RRA
        case 0x20: break; // JR NZ, r8
        case 0x21: break; // LD HL, d16
        case 0x22: break; // LD (HL+), A
        case 0x23: break; // INC HL
        case 0x24: break; // INC H
        case 0x25: break; // DEC H
        case 0x26: break; // LD H, d8
        case 0x27: break; // DAA
        case 0x28: break; // JR Z, r8
        case 0x29: break; // ADD HL, HL
        case 0x2a: break; // LD A, (HL+)
        case 0x2b: break; // DEC HL
        case 0x2c: break; // INC L
        case 0x2d: break; // DEC L
        case 0x2e: break; // LD L, d8
        case 0x2f: break; // CPL
        case 0x30: break; // JR NC, r8
        case 0x31: break; // LD SP, d16
        case 0x32: break; // LD (HL-), A
        case 0x33: break; // INC SP
        case 0x34: break; // INC (HL)
        case 0x35: break; // DEC (HL)
        case 0x36: break; // LD (HL), d8
        case 0x37: break; // nop
        case 0x38: break; // nop
        case 0x39: break; // nop
        case 0x3a: break; // nop
        case 0x3b: break; // nop
        case 0x3c: increment_r8(&registers.A); break; // nop
        case 0x3d: break; // nop
        case 0x3e: break; // nop
        case 0x3f: break; // nop
        case 0x40: break; // nop
        case 0x41: break; // LD BC, d16
        case 0x42: break; // nop
        case 0x43: break; // nop
        case 0x44: break; // nop
        case 0x45: break; // nop
        case 0x46: break; // nop
        case 0x47: load_r8(&registers.B, &registers.A); break; // nop
        case 0x48: break; // nop
        case 0x49: break; // nop
        case 0x4a: break; // nop
        case 0x4b: break; // nop
        case 0x4c: break; // nop
        case 0x4d: break; // nop
        case 0x4e: break; // nop
        case 0x4f: break; // nop
        case 0x50: break; // nop
        case 0x51: break; // LD BC, d16
        case 0x52: break; // nop
        case 0x53: break; // nop
        case 0x54: break; // nop
        case 0x55: break; // nop
        case 0x56: break; // nop
        case 0x57: break; // nop
        case 0x58: break; // nop
        case 0x59: break; // nop
        case 0x5a: break; // nop
        case 0x5b: break; // nop
        case 0x5c: break; // nop
        case 0x5d: break; // nop
        case 0x5e: break; // nop
        case 0x5f: break; // nop
        case 0x60: break; // nop
        case 0x61: break; // LD BC, d16
        case 0x62: break; // nop
        case 0x63: break; // nop
        case 0x64: break; // nop
        case 0x65: break; // nop
        case 0x66: break; // nop
        case 0x67: break; // nop
        case 0x68: break; // nop
        case 0x69: break; // nop
        case 0x6a: break; // nop
        case 0x6b: break; // nop
        case 0x6c: break; // nop
        case 0x6d: break; // nop
        case 0x6e: break; // nop
        case 0x6f: break; // nop
        case 0x70: break; // nop
        case 0x71: break; // LD BC, d16
        case 0x72: break; // nop
        case 0x73: break; // nop
        case 0x74: break; // nop
        case 0x75: break; // nop
        case 0x76: break; // nop
        case 0x77: break; // nop
        case 0x78: break; // nop
        case 0x79: break; // nop
        case 0x7a: break; // nop
        case 0x7b: break; // nop
        case 0x7c: break; // nop
        case 0x7d: break; // nop
        case 0x7e: break; // nop
        case 0x7f: break; // nop
        case 0x81: break; // LD BC, d16
        case 0x82: break; // nop
        case 0x83: break; // nop
        case 0x84: break; // nop
        case 0x85: break; // nop
        case 0x86: break; // nop
        case 0x87: break; // nop
        case 0x88: break; // nop
        case 0x89: break; // nop
        case 0x8a: break; // nop
        case 0x8b: break; // nop
        case 0x8c: break; // nop
        case 0x8d: break; // nop
        case 0x8e: break; // nop
        case 0x8f: break; // nop
        case 0x91: break; // LD BC, d16
        case 0x92: break; // nop
        case 0x93: break; // nop
        case 0x94: break; // nop
        case 0x95: break; // nop
        case 0x96: break; // nop
        case 0x97: break; // nop
        case 0x98: break; // nop
        case 0x99: break; // nop
        case 0x9a: break; // nop
        case 0x9b: break; // nop
        case 0x9c: break; // nop
        case 0x9d: break; // nop
        case 0x9e: break; // nop
        case 0x9f: break; // nop
        case 0xa1: break; // LD BC, d16
        case 0xa2: break; // nop
        case 0xa3: break; // nop
        case 0xa4: break; // nop
        case 0xa5: break; // nop
        case 0xa6: break; // nop
        case 0xa7: break; // nop
        case 0xa8: break; // nop
        case 0xa9: break; // nop
        case 0xaa: break; // nop
        case 0xab: break; // nop
        case 0xac: break; // nop
        case 0xad: break; // nop
        case 0xae: break; // nop
        case 0xaf: break; // nop
        case 0xb1: break; // LD BC, d16
        case 0xb2: break; // nop
        case 0xb3: break; // nop
        case 0xb4: break; // nop
        case 0xb5: break; // nop
        case 0xb6: break; // nop
        case 0xb7: break; // nop
        case 0xb8: break; // nop
        case 0xb9: break; // nop
        case 0xba: break; // nop
        case 0xbb: break; // nop
        case 0xbc: break; // nop
        case 0xbd: break; // nop
        case 0xbe: break; // nop
        case 0xbf: break; // nop
        case 0xc1: break; // LD BC, d16
        case 0xc2: break; // nop
        case 0xc3: break; // nop
        case 0xc4: break; // nop
        case 0xc5: break; // nop
        case 0xc6: break; // nop
        case 0xc7: break; // nop
        case 0xc8: break; // nop
        case 0xc9: break; // nop
        case 0xca: break; // nop
        case 0xcb: break; // nop
        case 0xcc: break; // nop
        case 0xcd: break; // nop
        case 0xce: break; // nop
        case 0xcf: break; // nop
        case 0xd1: break; // LD BC, d16
        case 0xd2: break; // nop
        case 0xd3: break; // nop
        case 0xd4: break; // nop
        case 0xd5: break; // nop
        case 0xd6: break; // nop
        case 0xd7: break; // nop
        case 0xd8: break; // nop
        case 0xd9: break; // nop
        case 0xda: break; // nop
        case 0xdb: break; // nop
        case 0xdc: break; // nop
        case 0xdd: break; // nop
        case 0xde: break; // nop
        case 0xdf: break; // nop
        case 0xe1: break; // LD BC, d16
        case 0xe2: break; // nop
        case 0xe3: break; // nop
        case 0xe4: break; // nop
        case 0xe5: break; // nop
        case 0xe6: break; // nop
        case 0xe7: break; // nop
        case 0xe8: break; // nop
        case 0xe9: break; // nop
        case 0xea: break; // nop
        case 0xeb: break; // nop
        case 0xec: break; // nop
        case 0xed: break; // nop
        case 0xee: break; // nop
        case 0xef: break; // nop
        case 0xf1: break; // LD BC, d16
        case 0xf2: break; // nop
        case 0xf3: break; // nop
        case 0xf4: break; // nop
        case 0xf5: break; // nop
        case 0xf6: break; // nop
        case 0xf7: break; // nop
        case 0xf8: break; // nop
        case 0xf9: break; // nop
        case 0xfa: break; // nop
        case 0xfb: break; // nop
        case 0xfc: break; // nop
        case 0xfd: break; // nop
        case 0xfe: break; // nop
        case 0xff: break; // nop
        default:
            printf("Unknown instruction");
            break;
    }
}

int main()
{
    // http://gbdev.gg8.se/wiki/articles/Gameboy_Bootstrap_ROM#Contents_of_the_ROM 
    printf("%zu\n", sizeof(memory)); 

    registers.A = 0xff;
    registers.B = 0x00;
    print_registers();
    {
        do_instruction(0x3c);
    }
    print_registers();


    return 0;
}

