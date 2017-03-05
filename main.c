#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>
#include <assert.h>
#include <string.h>

#include "common.h"
#include "file.h"
#include "logging.h"
#include "memory.h"

// TODO consider moving the actual instructions into their own 'cpu.h'
/*
Zero (0x80):        Set if the last operation produced a result of 0;
Operation (0x40):   Set if the last operation was a subtraction;
Half-carry (0x20):  Set if, in the result of the last operation, the lower half of the byte overflowed past 15;
Carry (0x10):       Set if the last operation produced a result over 255 (for additions) or under 0 (for subtractions).
*/
#define FLAGS_ZERO      (0x80) 
#define FLAGS_NEGATIVE  (0x40) 
#define FLAGS_HALFCARRY (0x20) 
#define FLAGS_CARRY     (0x10) 

/*
#define FLAGS_ISSET(x) (registers.F & (x))
#define FLAGS_SET(x) (registers.F |= (x))
// clear by inverting the bit position then ANDing it.
#define FLAGS_CLEAR(x) (registers.F &= ~(x))
*/

// memory boundaries
#define ROM_BANK0                   (0x0000) // 16kb
#define SWITCHABLE_ROM              (0x4000) // 16kb
#define VIDEO_RAM                   (0x8000) // 8kb
#define SWITCHABLE_RAM              (0xa000) // 8kb
#define INTERNAL_RAM1               (0xc000) // 8kb
#define INTERNAL_RAM1_ECHO          (0xe000) // 8kb
#define SPRITE_ATTR_OAM             (0xfe00) // ?
#define EMPTY_1                     (0xfea0) // ?
#define IO_PORTS                    (0xff00) // ?
#define EMPTY_2                     (0xfea0) // ?
#define INTERNEL_RAM2               (0xff80) // ?
#define INTERRUPT_ENABLE_REGISTER   (0xffff) // ? 

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

typedef struct {
    int m;
    int t;
} Clock;

Clock tick_clock = {0,0};
Clock total_clock = {0,0};
Registers registers;

void set_ticks(int t){
    tick_clock.t = t;
    tick_clock.m = t/4;
}

void print_registers() {
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

void increment_r16(u16* operand){
    // no flags
    (*operand)++;
    set_ticks(8);
}

void increment_r8(u8* operand){
    registers.F = 0;
   
    if (*operand == 0xff)
    {
        // we are about to produce a 0
        // we are about to carry
        registers.F |= FLAGS_ZERO;
        registers.F |= FLAGS_CARRY;
    }

    (*operand)++;
    set_ticks(4);
}

void decrement_r16(u16* operand){
    // no flags
    (*operand)--;
    set_ticks(8);
}

void decrement_r8(u8* operand){
    registers.F = 0;

    if (*operand == 0x00)
    {
        registers.F |= FLAGS_ZERO;
        registers.F |= FLAGS_CARRY;
    }

    (*operand)--;
    set_ticks(4);
}

void add_r16(u16* lhs, u16* rhs)
{
    (*lhs) += (*rhs);
    set_ticks(8);
}

void add_a_r8(u8* rhs){
    registers.F = 0;
    // TODO flags
    registers.A += *rhs;
    set_ticks(4);
} 

void adc_a_r8(u8* rhs){
    registers.F = 0;
    // TODO flags
    set_ticks(4);
} 

void sub_a_r8(u8* rhs){
    registers.F = 0;
    registers.A -= *rhs;
    set_ticks(4);
}

void subc_a_r8(u8* rhs){
    registers.F = 0;
    set_ticks(4);
}

void xor_a_r8(u8* rhs){
    registers.F = 0;
    registers.A ^= *rhs;

    if(registers.A == 0x00) registers.F |= FLAGS_ZERO;
    set_ticks(4);
}

void or_a_r8(u8* rhs){
    registers.F = 0;
    registers.A |= *rhs;

    if(registers.A == 0x00) registers.F |= FLAGS_ZERO;
    set_ticks(4);
}

void and_a_r8(u8* rhs){
    registers.F = 0;
    registers.F |= FLAGS_HALFCARRY;

    registers.A &= *rhs;

    if(registers.A == 0x00) registers.F |= FLAGS_ZERO;
    set_ticks(4);
}

void nop(){
    set_ticks(4);
}

void rotate_right(u8* operand){
    u8 old_carry = registers.F & FLAGS_CARRY;
    registers.F = 0; 

    u8 carry = ((*operand) & 0x01);
    registers.F |= (carry << 4);

    (*operand) = (*operand) >> 1;
    (*operand) |= old_carry << 3;
}

void rotate_left(u8* operand){
    u8 old_carry = registers.F & FLAGS_CARRY;
    registers.F = 0;

    // grab my carry and put it in carry flag
    u8 carry = ((*operand) & 0x80);
    registers.F |= (carry >> 3);

    // shift left and put the old carry into the first bit
    (*operand) = (*operand) << 1;
    (*operand) |= old_carry >> 4;
}

void rotate_right_carry(u8* operand){
    registers.F = 0;
   
    // store lowest bit
    u8 carry = ((*operand) & 0x01);

    // shift lowest bit out
    (*operand) = (*operand) >> 1;

    // carry old highest bit to lowest
    (*operand) |= (carry << 7);

    // assign carry flag
    registers.F |= (carry << 4);
}

void rotate_left_carry(u8* operand){
    registers.F = 0;
   
    // store highest bit
    u8 carry = ((*operand) & 0x80);

    // shift highest bit out
    (*operand) = (*operand) << 1;

    // carry old highest bit to lowest
    (*operand) |= (carry >> 7);

    // assign carry flag
    registers.F |= (carry >> 3);
}

void rotate_right_a(u8* operand)
{
    rotate_right(operand);
    set_ticks(4);
}

void rotate_left_a(u8* operand)
{
    rotate_left(operand);
    set_ticks(4);
}

void rotate_right_carry_a(u8* operand)
{
    rotate_right_carry(operand);
    set_ticks(4);
}

void rotate_left_carry_a(u8* operand)
{
    rotate_left_carry(operand);
    set_ticks(4);
}

void rotate_right_cb(u8* operand)
{
    rotate_right(operand);
    set_ticks(8);
}

void rotate_left_cb(u8* operand)
{
    rotate_left(operand);
    set_ticks(8);
}

void rotate_right_carry_cb(u8* operand)
{
    rotate_right_carry(operand);
    set_ticks(8);
}

void rotate_left_carry_cb(u8* operand)
{
    rotate_left_carry(operand);
    set_ticks(8);
}

void halt(){
    assert(0);
}

void stop(){
    assert(0);
}

void undefined(u8 opcode){
    printf("Undefined instruction 0x%02x", opcode);
    assert(0);
}

void load_r8(u8* lhs, u8* rhs){
    *lhs = *rhs;
    set_ticks(4);
}

void load_into_r8_from_addr(u8* lhs, u16* addr){
    *lhs = mem_read_u8(*addr);
    set_ticks(8);
}

void load_into_addr_from_r8(u16* addr, u8* value){
    mem_write_u8(*addr, *value);
    set_ticks(8);
}

void load_r16_value(u16* lhs){
    *lhs = mem_read_u16(registers.SP);
    registers.PC += 2;
    set_ticks(12);
}

void do_cb_instruction(){

}

void do_instruction(u8 instruction){
    switch(instruction){
        case 0x00: nop(); break; // NOP
        case 0x01: break; // LD BC, d16
        case 0x02: break; // LD (BC), A
        case 0x03: increment_r16(&registers.BC); break; // INC BC
        case 0x04: increment_r8(&registers.B); break; // INC B
        case 0x05: decrement_r8(&registers.B); break; // DEC B
        case 0x06: break; // LD B, d8
        case 0x07: rotate_left_carry(&registers.A); break; // RLCA
        case 0x08: break; // LD (a16), SP
        case 0x09: add_r16(&registers.HL, &registers.BC); break; // ADD HL, BC
        case 0x0a: break; // LD A, (BC)
        case 0x0b: break; // DEC BC
        case 0x0c: increment_r8(&registers.C); break; // INC C
        case 0x0d: decrement_r8(&registers.C); break; // DEC C
        case 0x0e: break; // LD C, d8
        case 0x0f: rotate_right_carry(&registers.A); break; // RRCA
        case 0x10: stop(); break; // STOP 0
        case 0x11: break; // LD DE, d16
        case 0x12: break; // LD (DE), A
        case 0x13: increment_r16(&registers.DE); break; // INC DE
        case 0x14: increment_r8(&registers.D); break; // INC D
        case 0x15: decrement_r8(&registers.D); break; // DEC D
        case 0x16: break; // LD D, d8
        case 0x17: rotate_left(&registers.A); break; // RLA
        case 0x18: break; // JR r8
        case 0x19: add_r16(&registers.HL, &registers.DE); break; // ADD HL, DE
        case 0x1a: break; // LD A, (DE)
        case 0x1b: decrement_r16(&registers.DE); break; // DEC DE
        case 0x1c: increment_r8(&registers.E); break; // INC E
        case 0x1d: decrement_r8(&registers.E); break; // DEC E
        case 0x1e: break; // LD E, d8
        case 0x1f: rotate_right(&registers.A); break; // RRA
        case 0x20: break; // JR NZ, r8
        case 0x21: load_r16_value(&registers.HL); OPLOG(0x21, "LD HL, D16"); break; // LD HL, d16
        case 0x22: break; // LD (HL+), A
        case 0x23: increment_r16(&registers.HL); break; // INC HL
        case 0x24: increment_r8(&registers.H); break; // INC H
        case 0x25: decrement_r8(&registers.H); break; // DEC H
        case 0x26: break; // LD H, d8
        case 0x27: break; // DAA
        case 0x28: break; // JR Z, r8
        case 0x29: add_r16(&registers.HL, &registers.HL); break; // ADD HL, HL
        case 0x2a: break; // LD A, (HL+)
        case 0x2b: decrement_r16(&registers.HL); break; // DEC HL
        case 0x2c: increment_r8(&registers.L); break; // INC L
        case 0x2d: decrement_r8(&registers.L); break; // DEC L
        case 0x2e: break; // LD L, d8
        case 0x2f: break; // CPL
        case 0x30: break; // JR NC, r8
        case 0x31: load_r16_value(&registers.SP); OPLOG(0x31, "LD SP, d16"); break; // LD SP, d16
        case 0x32: load_into_addr_from_r8(&registers.HL, &registers.A); registers.HL--; OPLOG(0x32, "LD (HL-), A"); break; // LD (HL-), A
        case 0x33: increment_r16(&registers.SP); break; // INC SP
        case 0x34: break; // INC (HL)
        case 0x35: break; // DEC (HL)
        case 0x36: break; // LD (HL), d8
        case 0x37: break; // SCF
        case 0x38: break; // JR C, r8
        case 0x39: add_r16(&registers.HL, &registers.SP); break; // ADD HL, SP
        case 0x3a: break; // LD A, (HL-)
        case 0x3b: decrement_r16(&registers.SP); break; // DEC SP
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
        case 0x76: halt(); break; // HALT
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
        case 0x90: sub_a_r8(&registers.B); break; // SUB B
        case 0x91: sub_a_r8(&registers.C); break; // SUB C
        case 0x92: sub_a_r8(&registers.D); break; // SUB D
        case 0x93: sub_a_r8(&registers.E); break; // SUB E
        case 0x94: sub_a_r8(&registers.H); break; // SUB H
        case 0x95: sub_a_r8(&registers.L); break; // SUB L
        case 0x96: break; // SUB (HL)
        case 0x97: sub_a_r8(&registers.A); break; // SUB A
        case 0x98: subc_a_r8(&registers.B); break; // SBC A, B
        case 0x99: subc_a_r8(&registers.C); break; // SBC A, C
        case 0x9a: subc_a_r8(&registers.D); break; // SBC A, D
        case 0x9b: subc_a_r8(&registers.E); break; // SBC A, E
        case 0x9c: subc_a_r8(&registers.H); break; // SBC A, H
        case 0x9d: subc_a_r8(&registers.L); break; // SBC A, L
        case 0x9e: break; // SBC A, (HL)
        case 0x9f: subc_a_r8(&registers.A); break; // SBC A, A
        case 0xa0: and_a_r8(&registers.B); break; // AND B 
        case 0xa1: and_a_r8(&registers.C); break; // AND C
        case 0xa2: and_a_r8(&registers.D); break; // AND D
        case 0xa3: and_a_r8(&registers.E); break; // AND E
        case 0xa4: and_a_r8(&registers.H); break; // AND H
        case 0xa5: and_a_r8(&registers.L); break; // AND L
        case 0xa6: break; // AND (HL)
        case 0xa7: and_a_r8(&registers.A); break; // AND A
        case 0xa8: xor_a_r8(&registers.B); break; // XOR B
        case 0xa9: xor_a_r8(&registers.C); break; // XOR C
        case 0xaa: xor_a_r8(&registers.D); break; // XOR D
        case 0xab: xor_a_r8(&registers.E); break; // XOR E
        case 0xac: xor_a_r8(&registers.H); break; // XOR H
        case 0xad: xor_a_r8(&registers.L); break; // XOR L
        case 0xae: break; // XOR (HL)
        case 0xaf: OPLOG(0xaf, "XOR A"); xor_a_r8(&registers.A); break; // XOR A
        case 0xb0: or_a_r8(&registers.B); break; // OR B
        case 0xb1: or_a_r8(&registers.C); break; // OR C
        case 0xb2: or_a_r8(&registers.D); break; // OR D
        case 0xb3: or_a_r8(&registers.E); break; // OR E
        case 0xb4: or_a_r8(&registers.H); break; // OR H
        case 0xb5: or_a_r8(&registers.L); break; // OR L
        case 0xb6: break; // OR (HL)
        case 0xb7: or_a_r8(&registers.A); break; // OR A
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
        case 0xcb: do_cb_instruction(); break; // PREFIX CB
        case 0xcc: break; // CALL Z, a16
        case 0xcd: break; // CALL a16
        case 0xce: break; // ADC A, d8
        case 0xcf: break; // RST 08H
        case 0xd0: break; // RET NC
        case 0xd1: break; // POP DE
        case 0xd2: break; // JP NC, a16
        case 0xd3: undefined(0xd3); break; // NO INSTRUCTION
        case 0xd4: break; // CALL NC, a16 
        case 0xd5: break; // PUSH DE
        case 0xd6: break; // SUB d8
        case 0xd7: break; // RST 10H
        case 0xd8: break; // RET C
        case 0xd9: break; // RETI
        case 0xda: break; // JP C, a16
        case 0xdb: undefined(0xdb); break; // NO INSTRUCTION
        case 0xdc: break; // CALL C, a16
        case 0xdd: undefined(0xdd); break; // NO INSTRUCTION
        case 0xde: break; // SBC A, d8
        case 0xdf: break; // RST 18H
        case 0xe0: break; // LDH (a8), A
        case 0xe1: break; // POP HL
        case 0xe2: break; // LD (C), A
        case 0xe3: undefined(0xe3); break; // NO INSTRUCTION
        case 0xe4: undefined(0xe4); break; // NO INSTRUCTION 
        case 0xe5: break; // PUSH HL
        case 0xe6: break; // AND d8
        case 0xe7: break; // RST 20H
        case 0xe8: break; // ADD SP, r8
        case 0xe9: break; // JP (HL)
        case 0xea: break; // LD (a16), A
        case 0xeb: undefined(0xeb); break; // NO INSTRUCTION
        case 0xec: undefined(0xec); break; // NO INSTRUCTION 
        case 0xed: undefined(0xed); break; // NO INSTRUCTION 
        case 0xee: break; // XOR d8
        case 0xef: break; // RST 28H
        case 0xf0: break; // LDH A,(a8)  
        case 0xf1: break; // POP AF
        case 0xf2: break; // LD A, (C)
        case 0xf3: break; // DI
        case 0xf4: undefined(0xf4); break; // NO INSTRUCTION
        case 0xf5: break; // PUSH AF
        case 0xf6: break; // OR d8
        case 0xf7: break; // RST 30H
        case 0xf8: break; // LD HL, SP+r8
        case 0xf9: break; // LD SP, HL
        case 0xfa: break; // LD A, (a16)
        case 0xfb: break; // EI
        case 0xfc: undefined(0xfc); break; // NO INSTRUCTION
        case 0xfd: undefined(0xfd); break; // NO INSTRUCTION
        case 0xfe: break; // CP d8
        case 0xff: break; // RST 38H
        default:
            printf("Unknown instruction");
            break;
    }
}


int main(){
    // http://gbdev.gg8.se/wiki/articles/Gameboy_Bootstrap_ROM#Contents_of_the_ROM 
    // printf("%zu\n", sizeof(memory)); 

    // setup memory with the boot rom
    const char* filename = "data/DMG_ROM.bin";
    u8_buffer* boot_rom = read_binary_file(filename);
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

    while(0)
    {
        // TODO interrupts
        do_instruction(memory[registers.PC++]);
        total_clock.m += tick_clock.m;
        total_clock.t += tick_clock.t;
    }

    // test rotate left carry
    registers.A = 0xf0; // 11110000
    do_instruction(0x07);
    assert(registers.A == 0xe1); // 11100001
    assert(registers.F & FLAGS_CARRY);
    
    registers.HL = 0x0001;
    do_instruction(0x29);
    assert(registers.HL == 0x0002);

    return 0;
}

