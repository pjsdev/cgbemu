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

void adc_a_r8(u8* rhs)
{
    registers.F = 0;
    // TODO flags
    registers.A += *rhs;
} 


void add_a_r8(u8* rhs)
{
    registers.F = 0;
    // TODO flags
    registers.A += *rhs;
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
        case 0x37: break; // SCF
        case 0x38: break; // JR C, r8
        case 0x39: break; // ADD HL, SP
        case 0x3a: break; // LD A, (HL-)
        case 0x3b: break; // DEC SP
        case 0x3c: increment_r8(&registers.A); break; // INC A
        case 0x3d: decrement_r8(&registers.A); break; // DEC A
        case 0x3e: break; // LD A, d8
        case 0x3f: break; // CCF
        case 0x40: load_r8(&registers.B, &registers.B); break; // LD B, B
        case 0x41: load_r8(&registers.B, &registers.C); break; // LD B, C
        case 0x42: load_r8(&registers.B, &registers.D); break; // LD B, D
        case 0x43: load_r8(&registers.B, &registers.E); break; // LD B, E 
        case 0x44: load_r8(&registers.B, &registers.H); break; // LD B, H  
        case 0x45: load_r8(&registers.B, &registers.L); break; // LD B, L  
        case 0x46: break; // LD B, (HL)  
        case 0x47: load_r8(&registers.B, &registers.A); break; // LD B, A 
        case 0x48: load_r8(&registers.C, &registers.B); break; // LD C, B
        case 0x49: load_r8(&registers.C, &registers.C); break; // LD C, C
        case 0x4a: load_r8(&registers.C, &registers.D); break; // LD C, D 
        case 0x4b: load_r8(&registers.C, &registers.E); break; // LD C, E
        case 0x4c: load_r8(&registers.C, &registers.H); break; // LD C, H
        case 0x4d: load_r8(&registers.C, &registers.L); break; // LD C, L
        case 0x4e: break; // LD C, (HL)
        case 0x4f: load_r8(&registers.C, &registers.A); break; // LD C, A
        case 0x50: load_r8(&registers.D, &registers.B); break; // LD D, B
        case 0x51: load_r8(&registers.D, &registers.C); break; // LD D, C
        case 0x52: load_r8(&registers.D, &registers.D); break; // LD D, D
        case 0x53: load_r8(&registers.D, &registers.E); break; // LD D, E
        case 0x54: load_r8(&registers.D, &registers.H); break; // LD D, H
        case 0x55: load_r8(&registers.D, &registers.L); break; // LD D, L
        case 0x56: break; // LD D, (HL)
        case 0x57: load_r8(&registers.D, &registers.A); break; // LD D, A
        case 0x58: load_r8(&registers.E, &registers.B); break; // LD E, B
        case 0x59: load_r8(&registers.E, &registers.C); break; // LD E, C
        case 0x5a: load_r8(&registers.E, &registers.D); break; // LD E, D
        case 0x5b: load_r8(&registers.E, &registers.E); break; // LD E, E
        case 0x5c: load_r8(&registers.E, &registers.H); break; // LD E, H
        case 0x5d: load_r8(&registers.E, &registers.L); break; // LD E, L
        case 0x5e: break; // LD E, (HL)
        case 0x5f: load_r8(&registers.E, &registers.A); break; // LD E, A
        case 0x60: load_r8(&registers.H, &registers.B); break; // LD H, B 
        case 0x61: load_r8(&registers.H, &registers.C); break; // LD H, C
        case 0x62: load_r8(&registers.H, &registers.D); break; // LD H, D 
        case 0x63: load_r8(&registers.H, &registers.E); break; // LD H, E 
        case 0x64: load_r8(&registers.H, &registers.H); break; // LD H, H 
        case 0x65: load_r8(&registers.H, &registers.L); break; // LD H, L 
        case 0x66: break; // LD H, (HL)                                   
        case 0x67: load_r8(&registers.H, &registers.A); break; // LD H, A 
        case 0x68: load_r8(&registers.L, &registers.B); break; // LD L, B 
        case 0x69: load_r8(&registers.L, &registers.C); break; // LD L, C 
        case 0x6a: load_r8(&registers.L, &registers.D); break; // LD L, D 
        case 0x6b: load_r8(&registers.L, &registers.E); break; // LD L, E 
        case 0x6c: load_r8(&registers.L, &registers.H); break; // LD L, H 
        case 0x6d: load_r8(&registers.L, &registers.L); break; // LD L, L 
        case 0x6e: break; // LD L, (HL)                                   
        case 0x6f: load_r8(&registers.L, &registers.A); break; // LD L, A 
        case 0x70: break; // LD (HL), B
        case 0x71: break; // LD (HL), C 
        case 0x72: break; // LD (HL), D
        case 0x73: break; // LD (HL), E
        case 0x74: break; // LD (HL), H
        case 0x75: break; // LD (HL), L
        case 0x76: break; // HALT
        case 0x77: break; // LD (HL), A
        case 0x78: load_r8(&registers.A, &registers.B); break; // LD A, B  
        case 0x79: load_r8(&registers.A, &registers.C); break; // LD A, C  
        case 0x7a: load_r8(&registers.A, &registers.D); break; // LD A, D  
        case 0x7b: load_r8(&registers.A, &registers.E); break; // LD A, E  
        case 0x7c: load_r8(&registers.A, &registers.H); break; // LD A, H  
        case 0x7d: load_r8(&registers.A, &registers.L); break; // LD A, L  
        case 0x7e: break; // LD A, (HL)  
        case 0x7f: load_r8(&registers.A, &registers.A); break; // LD A, A  

        // MATHS
        case 0x80: add_a_r8(&registers.B); break; // ADD A, B
        case 0x81: add_a_r8(&registers.C); break; // ADD A, C
        case 0x82: add_a_r8(&registers.D); break; // ADD A, D
        case 0x83: add_a_r8(&registers.E); break; // ADD A, E
        case 0x84: add_a_r8(&registers.H); break; // ADD A, H
        case 0x85: add_a_r8(&registers.L); break; // ADD A, L
        case 0x86: break; // ADD A, (HL)
        case 0x87: add_a_r8(&registers.A); break; // ADD A, A
        case 0x88: adc_a_r8(&registers.B); break; // ADC A, B 
        case 0x89: adc_a_r8(&registers.C); break; // ADC A, C 
        case 0x8a: adc_a_r8(&registers.D); break; // ADC A, D 
        case 0x8b: adc_a_r8(&registers.E); break; // ADC A, E 
        case 0x8c: adc_a_r8(&registers.H); break; // ADC A, H 
        case 0x8d: adc_a_r8(&registers.L); break; // ADC A, L 
        case 0x8e: break; // ADC A, (HL)                      
        case 0x8f: adc_a_r8(&registers.A); break; // ADC A, A 
        case 0x90: break; // SUB B
        case 0x91: break; // SUB C
        case 0x92: break; // SUB D
        case 0x93: break; // SUB E
        case 0x94: break; // SUB H
        case 0x95: break; // SUB L
        case 0x96: break; // SUB (HL)
        case 0x97: break; // SUB A
        case 0x98: break; // SBC A, B
        case 0x99: break; // SBC A, C
        case 0x9a: break; // SBC A, D
        case 0x9b: break; // SBC A, E
        case 0x9c: break; // SBC A, H
        case 0x9d: break; // SBC A, L
        case 0x9e: break; // SBC A, (HL)
        case 0x9f: break; // SBC A, A
        case 0xa0: break; // AND B 
        case 0xa1: break; // AND C
        case 0xa2: break; // AND D
        case 0xa3: break; // AND E
        case 0xa4: break; // AND H
        case 0xa5: break; // AND L
        case 0xa6: break; // AND (HL)
        case 0xa7: break; // AND A
        case 0xa8: break; // XOR B
        case 0xa9: break; // XOR C
        case 0xaa: break; // XOR D
        case 0xab: break; // XOR E
        case 0xac: break; // XOR H
        case 0xad: break; // XOR L
        case 0xae: break; // XOR (HL)
        case 0xaf: break; // XOR A
        case 0xb0: break; // OR B
        case 0xb1: break; // OR C
        case 0xb2: break; // OR D
        case 0xb3: break; // OR E
        case 0xb4: break; // OR H
        case 0xb5: break; // OR L
        case 0xb6: break; // OR (HL)
        case 0xb7: break; // OR A
        case 0xb8: break; // CP B
        case 0xb9: break; // CP C
        case 0xba: break; // CP D
        case 0xbb: break; // CP E
        case 0xbc: break; // CP H
        case 0xbd: break; // CP L
        case 0xbe: break; // CP (HL)
        case 0xbf: break; // CP A
        // end math
        case 0xc0: break; // RET NZ
        case 0xc1: break; // POP BC 
        case 0xc2: break; // JP NZ, a16
        case 0xc3: break; // JP a16
        case 0xc4: break; // CALL NZ, a16
        case 0xc5: break; // PUSH BC
        case 0xc6: break; // ADD A, d8
        case 0xc7: break; // RST 00H
        case 0xc8: break; // RET Z
        case 0xc9: break; // RET
        case 0xca: break; // JP Z, a16
        case 0xcb: break; // PREFIX CB
        case 0xcc: break; // CALL Z, a16
        case 0xcd: break; // CALL a16
        case 0xce: break; // ADC A, d8
        case 0xcf: break; // RST 08H
        case 0xd0: break; // RET NC
        case 0xd1: break; // POP DE
        case 0xd2: break; // JP NC, a16
        case 0xd3: break; // NO INSTRUCTION
        case 0xd4: break; // CALL NC, a16 
        case 0xd5: break; // PUSH DE
        case 0xd6: break; // SUB d8
        case 0xd7: break; // RST 10H
        case 0xd8: break; // RET C
        case 0xd9: break; // RETI
        case 0xda: break; // JP C, a16
        case 0xdb: break; // NO INSTRUCTION
        case 0xdc: break; // CALL C, a16
        case 0xdd: break; // NO INSTRUCTION
        case 0xde: break; // SBC A, d8
        case 0xdf: break; // RST 18H
        case 0xe0: break; // LDH (a8), A
        case 0xe1: break; // POP HL
        case 0xe2: break; // LD (C), A
        case 0xe3: break; // NO INSTRUCTION
        case 0xe4: break; // NO INSTRUCTION 
        case 0xe5: break; // PUSH HL
        case 0xe6: break; // AND d8
        case 0xe7: break; // RST 20H
        case 0xe8: break; // ADD SP, r8
        case 0xe9: break; // JP (HL)
        case 0xea: break; // LD (a16), A
        case 0xeb: break; // NO INSTRUCTION
        case 0xec: break; // NO INSTRUCTION 
        case 0xed: break; // NO INSTRUCTION 
        case 0xee: break; // XOR d8
        case 0xef: break; // RST 28H
        case 0xf0: break; // LDH A,(a8)  
        case 0xf1: break; // POP AF
        case 0xf2: break; // LD A, (C)
        case 0xf3: break; // DI
        case 0xf4: break; // NO INSTRUCTION
        case 0xf5: break; // PUSH AF
        case 0xf6: break; // OR d8
        case 0xf7: break; // RST 30H
        case 0xf8: break; // LD HL, SP+r8
        case 0xf9: break; // LD SP, HL
        case 0xfa: break; // LD A, (a16)
        case 0xfb: break; // EI
        case 0xfc: break; // NO INSTRUCTION
        case 0xfd: break; // NO INSTRUCTION
        case 0xfe: break; // CP d8
        case 0xff: break; // RST 38H
        default:
            printf("Unknown instruction");
            break;
    }
}

int main()
{
    // http://gbdev.gg8.se/wiki/articles/Gameboy_Bootstrap_ROM#Contents_of_the_ROM 
    // printf("%zu\n", sizeof(memory)); 

    registers.A = 0xff;
    registers.B = 0x00;
    print_registers();
    {
        do_instruction(0x3c);
    }
    print_registers();


    return 0;
}

