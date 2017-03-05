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

void load_r8_value(u8* lhs){
    *lhs = mem_read_u8(registers.PC);
    registers.PC++;
    set_ticks(8);
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

void load_a_into_offset(){
    mem_write_u8(0xff00 + memory[registers.PC++], registers.A);
    set_ticks(12);
}

void load_a_into_c_offset(){
    mem_write_u8(0xff00 + registers.C, registers.A);
    set_ticks(8);
}

void bit_compare_r8(int bitpos, u8* operand){
    if(*operand & (1 << bitpos)){
        //
        registers.F &= ~(FLAGS_ZERO);
    }
    else{
        // zero flag
        registers.F |= FLAGS_ZERO;
    }

    registers.F &= ~(FLAGS_NEGATIVE);
    registers.F |= FLAGS_CARRY;
    set_ticks(8);
}

void bit_compare_at_addr(int bitpos, u16* addr){
    u8 value = mem_read_u8(*addr);
    bit_compare_r8(bitpos, &value);
    set_ticks(16);
}

void do_cb_instruction(){
    u8 opcode = memory[registers.PC++];
    switch(opcode){
        case 0x7c: bit_compare_r8(7, &registers.H); OPLOG(0x7c, "BIT 7, H"); break;
        default:
            printf("Opcode not implemented 0x%02x\n", opcode);
    }
}

void jump_nz(u8* relative_addr){
    registers.PC += 1;
    if (registers.F & FLAGS_ZERO){
        registers.PC += (signed char)*relative_addr; 
    }

    set_ticks(8);
}

void do_instruction(u8 instruction){
    switch(instruction){
        case 0x00: {  // NOP
            nop();
            OPLOG(0x00, "NOP");
        } break;
        case 0x01: {  // LD BC, d16
            load_r16_value(&registers.BC);
            OPLOG(0x01, "LD BC, d16");
        } break;
        case 0x02: {  // LD (BC), A
            load_into_addr_from_r8(&registers.BC, &registers.A);
            OPLOG(0x02, "LD (BC), A");
        } break;
        case 0x03: {  // INC BC
            increment_r16(&registers.BC);
            OPLOG(0x03, "INC BC");
        } break;
        case 0x04: {  // INC B
            increment_r8(&registers.B);
            OPLOG(0x04, "INC B");
        } break;
        case 0x05: {  // DEC B
            decrement_r8(&registers.B);
            OPLOG(0x05, "DEC B");
        } break;
        case 0x06: {  // LD B, d8
            load_r8_value(&registers.B);
            OPLOG(0x06, "LD B, d8");
        } break;
        case 0x07: {  // RLCA
            rotate_left_carry(&registers.A);
            OPLOG(0x07, "RCLA");
        } break;
        case 0x08: {  // LD (a16), SP
        } break;
        case 0x09: {  // ADD HL, BC
            add_r16(&registers.HL, &registers.BC);
        } break;
        case 0x0a: {  // LD A, (BC)
            load_into_r8_from_addr(&registers.A, &registers.BC);
            OPLOG(0x0a, "LD A, (BC)");
        }  break;
        case 0x0b: {  // DEC BC
            decrement_r16(&registers.BC);
            OPLOG(0x0b, "DEC BC");
        } break;
        case 0x0c: {  // INC C
            increment_r8(&registers.C);
            OPLOG(0x0c, "INC C");
        }  break;
        case 0x0d: {  // DEC C
            decrement_r8(&registers.C);
        } break;
        case 0x0e: {  // LD C, d8
            load_r8_value(&registers.C);
            OPLOG(0x06, "LD C, d8");
        }  break;
        case 0x0f: {  // RRCA
            rotate_right_carry(&registers.A);
        } break;
        case 0x10: {  // STOP 0
            stop();
            OPLOG(0x10, "STOP 0");
        } break;
        case 0x11: {  // LD DE, d16
            load_r16_value(&registers.DE);
            OPLOG(0x11, "LD DE, d16");
        } break;
        case 0x12: {  // LD (DE), A
            load_into_addr_from_r8(&registers.DE, &registers.A);
            OPLOG(0x12, "LD (DE), A");
        } break;
        case 0x13: {  // INC DE
            increment_r16(&registers.DE);
            OPLOG(0x13, "INC DE");
        } break;
        case 0x14: {  // INC D
            increment_r8(&registers.D);
            OPLOG(0x14, "INC D");
        }  break;
        case 0x15: {  // DEC D
            decrement_r8(&registers.D);
            OPLOG(0x15, "DEC D");
        }   break;
        case 0x16: {  // LD D, d8
            load_r8_value(&registers.D);
            OPLOG(0x16, "LD D, d8");
        }  break;
        case 0x17: {  // RLA
            rotate_left(&registers.A);
            OPLOG(0x17, "RLA");
        } break;
        case 0x18: {  // JR r8
        } break;
        case 0x19: {  // ADD HL, DE
            add_r16(&registers.HL, &registers.DE);
        } break;
        case 0x1a: {  // LD A, (DE)
            load_into_r8_from_addr(&registers.A, &registers.DE);
            OPLOG(0x1a, "LD A, (DE)");
        } break;
        case 0x1b: {  // DEC DE
            decrement_r16(&registers.DE);
        } break;
        case 0x1c: {  // INC E
            increment_r8(&registers.E);
        } break;
        case 0x1d: {  // DEC E
            decrement_r8(&registers.E);
        } break;
        case 0x1e: {  // LD E, d8
            load_r8_value(&registers.E);
            OPLOG(0x06, "LD E, d8");
        }  break;
        case 0x1f: {  // RRA
            rotate_right(&registers.A);
        } break;
        case 0x20: {  // JR NZ, r8
        } break;
        case 0x21: {  // LD HL, d16
            load_r16_value(&registers.HL);
            OPLOG(0x21, "LD HL, D16");
        } break;
        case 0x22: {  // LD (HL+), A
            load_into_addr_from_r8(&registers.HL, &registers.A);
            registers.HL++;
            OPLOG(0x22, "LD (HL+), A");
        } break;
        case 0x23: {  // INC HL
            increment_r16(&registers.HL);
        } break;
        case 0x24: {  // INC H
            increment_r8(&registers.H);
        } break;
        case 0x25: {  // DEC H
            decrement_r8(&registers.H);
        } break;
        case 0x26: {  // LD H, d8
            load_r8_value(&registers.H);
            OPLOG(0x06, "LD H, d8");
        }  break;
        case 0x27: {  // DAA
        } break;
        case 0x28: {  // JR Z, r8
        } break;
        case 0x29: {  // ADD HL, HL
            add_r16(&registers.HL, &registers.HL);
        } break;
        case 0x2a: {  // LD A, (HL+)
        } break;
        case 0x2b: {  // DEC HL
            decrement_r16(&registers.HL);
        } break;
        case 0x2c: {  // INC L
            increment_r8(&registers.L);
        } break;
        case 0x2d: {  // DEC L
            decrement_r8(&registers.L);
        } break;
        case 0x2e: {  // LD L, d8
            load_r8_value(&registers.L);
            OPLOG(0x06, "LD L, d8");
        }  break;
        case 0x2f: {  // CPL
        } break;
        case 0x30: {  // JR NC, r8
        } break;
        case 0x31: {  // LD SP, d16
            load_r16_value(&registers.SP);
            OPLOG(0x31, "LD SP, d16");
        } break;
        case 0x32: {  // LD (HL-), A
            load_into_addr_from_r8(&registers.HL, &registers.A);
            registers.HL--;
            OPLOG(0x32, "LD (HL-), A");
        } break;
        case 0x33: {  // INC SP
            increment_r16(&registers.SP);
        } break;
        case 0x34: {  // INC (HL)
        } break;
        case 0x35: {  // DEC (HL)
        } break;
        case 0x36: {  // LD (HL), d8
        } break;
        case 0x37: {  // SCF
        } break;
        case 0x38: {  // JR C, r8
        } break;
        case 0x39: {  // ADD HL, SP
            add_r16(&registers.HL, &registers.SP);
        } break;
        case 0x3a: {  // LD A, (HL-)
        } break;
        case 0x3b: {  // DEC SP
            decrement_r16(&registers.SP);
        } break;
        case 0x3c: {  // INC A
            increment_r8(&registers.A);
        } break;
        case 0x3d: {  // DEC A
            decrement_r8(&registers.A);
        } break;
        case 0x3e: {  // LD A, d8
            load_r8_value(&registers.A);
            OPLOG(0x06, "LD A, d8");
        }  break;
        case 0x3f: {  // CCF
        } break;
        case 0x40: {  // LD B, B
            load_r8(&registers.B, &registers.B);
        } break;
        case 0x41: {  // LD B, C
            load_r8(&registers.B, &registers.C);
        } break;
        case 0x42: {  // LD B, D
            load_r8(&registers.B, &registers.D);
        } break;
        case 0x43: {  // LD B, E 
            load_r8(&registers.B, &registers.E);
        } break;
        case 0x44: {  // LD B, H  
            load_r8(&registers.B, &registers.H);
        } break;
        case 0x45: {  // LD B, L  
            load_r8(&registers.B, &registers.L);
        } break;
        case 0x46: {  // LD B, (HL)  
        } break;
        case 0x47: {  // LD B, A 
            load_r8(&registers.B, &registers.A);
        } break;
        case 0x48: {  // LD C, B
            load_r8(&registers.C, &registers.B);
        } break;
        case 0x49: {  // LD C, C
            load_r8(&registers.C, &registers.C);
        } break;
        case 0x4a: {  // LD C, D 
            load_r8(&registers.C, &registers.D);
        } break;
        case 0x4b: {  // LD C, E
            load_r8(&registers.C, &registers.E);
        } break;
        case 0x4c: {  // LD C, H
            load_r8(&registers.C, &registers.H);
        } break;
        case 0x4d: {  // LD C, L
            load_r8(&registers.C, &registers.L);
        } break;
        case 0x4e: {  // LD C, (HL)
        } break;
        case 0x4f: {  // LD C, A
            load_r8(&registers.C, &registers.A);
        } break;
        case 0x50: {  // LD D, B
            load_r8(&registers.D, &registers.B);
        } break;
        case 0x51: {  // LD D, C
            load_r8(&registers.D, &registers.C);
        } break;
        case 0x52: {  // LD D, D
            load_r8(&registers.D, &registers.D);
        } break;
        case 0x53: {  // LD D, E
            load_r8(&registers.D, &registers.E);
        } break;
        case 0x54: {  // LD D, H
            load_r8(&registers.D, &registers.H);
        } break;
        case 0x55: {  // LD D, L
            load_r8(&registers.D, &registers.L);
        } break;
        case 0x56: {  // LD D, (HL)
        } break;
        case 0x57: {  // LD D, A
            load_r8(&registers.D, &registers.A);
        } break;
        case 0x58: {  // LD E, B
            load_r8(&registers.E, &registers.B);
        } break;
        case 0x59: {  // LD E, C
            load_r8(&registers.E, &registers.C);
        } break;
        case 0x5a: {  // LD E, D
            load_r8(&registers.E, &registers.D);
        } break;
        case 0x5b: {  // LD E, E
            load_r8(&registers.E, &registers.E);
        } break;
        case 0x5c: {  // LD E, H
            load_r8(&registers.E, &registers.H);
        } break;
        case 0x5d: {  // LD E, L
            load_r8(&registers.E, &registers.L);
        } break;
        case 0x5e: {  // LD E, (HL)
        } break;
        case 0x5f: {  // LD E, A
            load_r8(&registers.E, &registers.A);
        } break;
        case 0x60: {  // LD H, B 
            load_r8(&registers.H, &registers.B);
        } break;
        case 0x61: {  // LD H, C
            load_r8(&registers.H, &registers.C);
        } break;
        case 0x62: {  // LD H, D 
            load_r8(&registers.H, &registers.D);
        } break;
        case 0x63: {  // LD H, E 
            load_r8(&registers.H, &registers.E);
        } break;
        case 0x64: {  // LD H, H 
            load_r8(&registers.H, &registers.H);
        } break;
        case 0x65: {  // LD H, L 
            load_r8(&registers.H, &registers.L);
        } break;
        case 0x66: {  // LD H, (HL)                                   
        } break;
        case 0x67: {  // LD H, A 
            load_r8(&registers.H, &registers.A);
        } break;
        case 0x68: {  // LD L, B 
            load_r8(&registers.L, &registers.B);
        } break;
        case 0x69: {  // LD L, C 
            load_r8(&registers.L, &registers.C);
        } break;
        case 0x6a: {  // LD L, D 
            load_r8(&registers.L, &registers.D);
        } break;
        case 0x6b: {  // LD L, E 
            load_r8(&registers.L, &registers.E);
        } break;
        case 0x6c: {  // LD L, H 
            load_r8(&registers.L, &registers.H);
        } break;
        case 0x6d: {  // LD L, L 
            load_r8(&registers.L, &registers.L);
        } break;
        case 0x6e: {  // LD L, (HL)                                   
        } break;
        case 0x6f: {  // LD L, A 
            load_r8(&registers.L, &registers.A);
        } break;
        case 0x70: {  // LD (HL), B
            load_into_addr_from_r8(&registers.HL, &registers.B);
            OPLOG(0x70, "LD (HL), B");
        }  break;
        case 0x71: {  // LD (HL), C 
            load_into_addr_from_r8(&registers.HL, &registers.C);
            OPLOG(0x71, "LD (HL), C");
        }  break;
        case 0x72: {  // LD (HL), D
            load_into_addr_from_r8(&registers.HL, &registers.D);
            OPLOG(0x72, "LD (HL), D");
        }  break;
        case 0x73: {  // LD (HL), E
            load_into_addr_from_r8(&registers.HL, &registers.E);
            OPLOG(0x73, "LD (HL), E");
        }  break;
        case 0x74: {  // LD (HL), H
            load_into_addr_from_r8(&registers.HL, &registers.H);
            OPLOG(0x74, "LD (HL), H");
        }  break;
        case 0x75: {  // LD (HL), L
            load_into_addr_from_r8(&registers.HL, &registers.L);
            OPLOG(0x75, "LD (HL), L");
        }  break;
        case 0x76: {  // HALT
            halt();
        } break;
        case 0x77: {  // LD (HL), A
            load_into_addr_from_r8(&registers.HL, &registers.A);
            OPLOG(0x77, "LD (HL), A");
        } break;
        case 0x78: {  // LD A, B  
            load_r8(&registers.A, &registers.B);
        } break;
        case 0x79: {  // LD A, C  
            load_r8(&registers.A, &registers.C);
        } break;
        case 0x7a: {  // LD A, D  
            load_r8(&registers.A, &registers.D);
        } break;
        case 0x7b: {  // LD A, E  
            load_r8(&registers.A, &registers.E);
        } break;
        case 0x7c: {  // LD A, H  
            load_r8(&registers.A, &registers.H);
        } break;
        case 0x7d: {  // LD A, L  
            load_r8(&registers.A, &registers.L);
        } break;
        case 0x7e: {  // LD A, (HL)  
        } break;
        case 0x7f: {  // LD A, A  
            load_r8(&registers.A, &registers.A);
        } break;
        case 0x80: {  // ADD A, B
            add_a_r8(&registers.B);
        } break;
        case 0x81: {  // ADD A, C
            add_a_r8(&registers.C);
        } break;
        case 0x82: {  // ADD A, D
            add_a_r8(&registers.D);
        } break;
        case 0x83: {  // ADD A, E
            add_a_r8(&registers.E);
        } break;
        case 0x84: {  // ADD A, H
            add_a_r8(&registers.H);
        } break;
        case 0x85: {  // ADD A, L
            add_a_r8(&registers.L);
        } break;
        case 0x86: {  // ADD A, (HL)
        } break;
        case 0x87: {  // ADD A, A
            add_a_r8(&registers.A);
        } break;
        case 0x88: {  // ADC A, B 
            adc_a_r8(&registers.B);
        } break;
        case 0x89: {  // ADC A, C 
            adc_a_r8(&registers.C);
        } break;
        case 0x8a: {  // ADC A, D 
            adc_a_r8(&registers.D);
        } break;
        case 0x8b: {  // ADC A, E 
            adc_a_r8(&registers.E);
        } break;
        case 0x8c: {  // ADC A, H 
            adc_a_r8(&registers.H);
        } break;
        case 0x8d: {  // ADC A, L 
            adc_a_r8(&registers.L);
        } break;
        case 0x8e: {  // ADC A, (HL)                      
        } break;
        case 0x8f: {  // ADC A, A 
            adc_a_r8(&registers.A);
        } break;
        case 0x90: {  // SUB B
            sub_a_r8(&registers.B);
        } break;
        case 0x91: {  // SUB C
            sub_a_r8(&registers.C);
        } break;
        case 0x92: {  // SUB D
            sub_a_r8(&registers.D);
        } break;
        case 0x93: {  // SUB E
            sub_a_r8(&registers.E);
        } break;
        case 0x94: {  // SUB H
            sub_a_r8(&registers.H);
        } break;
        case 0x95: {  // SUB L
            sub_a_r8(&registers.L);
        } break;
        case 0x96: {  // SUB (HL)
        } break;
        case 0x97: {  // SUB A
            sub_a_r8(&registers.A);
        } break;
        case 0x98: {  // SBC A, B
            subc_a_r8(&registers.B);
        } break;
        case 0x99: {  // SBC A, C
            subc_a_r8(&registers.C);
        } break;
        case 0x9a: {  // SBC A, D
            subc_a_r8(&registers.D);
        } break;
        case 0x9b: {  // SBC A, E
            subc_a_r8(&registers.E);
        } break;
        case 0x9c: {  // SBC A, H
            subc_a_r8(&registers.H);
        } break;
        case 0x9d: {  // SBC A, L
            subc_a_r8(&registers.L);
        } break;
        case 0x9e: {  // SBC A, (HL)
        } break;
        case 0x9f: {  // SBC A, A
            subc_a_r8(&registers.A);
        } break;
        case 0xa0: {  // AND B 
            and_a_r8(&registers.B);
        } break;
        case 0xa1: {  // AND C
            and_a_r8(&registers.C);
        } break;
        case 0xa2: {  // AND D
            and_a_r8(&registers.D);
        } break;
        case 0xa3: {  // AND E
            and_a_r8(&registers.E);
        } break;
        case 0xa4: {  // AND H
            and_a_r8(&registers.H);
        } break;
        case 0xa5: {  // AND L
            and_a_r8(&registers.L);
        } break;
        case 0xa6: {  // AND (HL)
        } break;
        case 0xa7: {  // AND A
            and_a_r8(&registers.A);
        } break;
        case 0xa8: {  // XOR B
            xor_a_r8(&registers.B);
        } break;
        case 0xa9: {  // XOR C
            xor_a_r8(&registers.C);
        } break;
        case 0xaa: {  // XOR D
            xor_a_r8(&registers.D);
        } break;
        case 0xab: {  // XOR E
            xor_a_r8(&registers.E);
        } break;
        case 0xac: {  // XOR H
            xor_a_r8(&registers.H);
        } break;
        case 0xad: {  // XOR L
            xor_a_r8(&registers.L);
        } break;
        case 0xae: {  // XOR (HL)
        } break;
        case 0xaf: {  // XOR A
            OPLOG(0xaf, "XOR A");
            xor_a_r8(&registers.A);
        } break;
        case 0xb0: {  // OR B
            or_a_r8(&registers.B);
        } break;
        case 0xb1: {  // OR C
            or_a_r8(&registers.C);
        } break;
        case 0xb2: {  // OR D
            or_a_r8(&registers.D);
        } break;
        case 0xb3: {  // OR E
            or_a_r8(&registers.E);
        } break;
        case 0xb4: {  // OR H
            or_a_r8(&registers.H);
        } break;
        case 0xb5: {  // OR L
            or_a_r8(&registers.L);
        } break;
        case 0xb6: {  // OR (HL)
        } break;
        case 0xb7: {  // OR A
            or_a_r8(&registers.A);
        } break;
        case 0xb8: {  // CP B
        } break;
        case 0xb9: {  // CP C
        } break;
        case 0xba: {  // CP D
        } break;
        case 0xbb: {  // CP E
        } break;
        case 0xbc: {  // CP H
        } break;
        case 0xbd: {  // CP L
        } break;
        case 0xbe: {  // CP (HL)
        } break;
        case 0xbf: {  // CP A
        } break;
        case 0xc0: {  // RET NZ
        } break;
        case 0xc1: {  // POP BC 
        } break;
        case 0xc2: {  // JP NZ, a16
        } break;
        case 0xc3: {  // JP a16
        } break;
        case 0xc4: {  // CALL NZ, a16
        } break;
        case 0xc5: {  // PUSH BC
        } break;
        case 0xc6: {  // ADD A, d8
        } break;
        case 0xc7: {  // RST 00H
        } break;
        case 0xc8: {  // RET Z
        } break;
        case 0xc9: {  // RET
        } break;
        case 0xca: {  // JP Z, a16
        } break;
        case 0xcb: {  // PREFIX CB
            do_cb_instruction();
        } break;
        case 0xcc: {  // CALL Z, a16
        } break;
        case 0xcd: {  // CALL a16
        } break;
        case 0xce: {  // ADC A, d8
        } break;
        case 0xcf: {  // RST 08H
        } break;
        case 0xd0: {  // RET NC
        } break;
        case 0xd1: {  // POP DE
        } break;
        case 0xd2: {  // JP NC, a16
        } break;
        case 0xd3: {  // NO INSTRUCTION
            undefined(0xd3);
        } break;
        case 0xd4: {  // CALL NC, a16 
        } break;
        case 0xd5: {  // PUSH DE
        } break;
        case 0xd6: {  // SUB d8
        } break;
        case 0xd7: {  // RST 10H
        } break;
        case 0xd8: {  // RET C
        } break;
        case 0xd9: {  // RETI
        } break;
        case 0xda: {  // JP C, a16
        } break;
        case 0xdb: {  // NO INSTRUCTION
            undefined(0xdb);
        } break;
        case 0xdc: {  // CALL C, a16
        } break;
        case 0xdd: {  // NO INSTRUCTION
            undefined(0xdd);
        } break;
        case 0xde: {  // SBC A, d8
        } break;
        case 0xdf: {  // RST 18H
        } break;
        case 0xe0: {  // LDH (a8), A
            load_a_into_offset();
            OPLOG(0xe0, "LDH (a8), A");
        } break;
        case 0xe1: {  // POP HL
        } break;
        case 0xe2: {  // LD (C), A
            load_a_into_c_offset();
            OPLOG(0xe2, "LD (C), A");
        } break;
        case 0xe3: {  // NO INSTRUCTION
            undefined(0xe3);
        } break;
        case 0xe4: {  // NO INSTRUCTION 
            undefined(0xe4);
        } break;
        case 0xe5: {  // PUSH HL
        } break;
        case 0xe6: {  // AND d8
        } break;
        case 0xe7: {  // RST 20H
        } break;
        case 0xe8: {  // ADD SP, r8
        } break;
        case 0xe9: {  // JP (HL)
        } break;
        case 0xea: {  // LD (a16), A
        } break;
        case 0xeb: {  // NO INSTRUCTION
            undefined(0xeb);
        } break;
        case 0xec: {  // NO INSTRUCTION 
            undefined(0xec);
        } break;
        case 0xed: {  // NO INSTRUCTION 
            undefined(0xed);
        } break;
        case 0xee: {  // XOR d8
        } break;
        case 0xef: {  // RST 28H
        } break;
        case 0xf0: {  // LDH A,(a8)  
        } break;
        case 0xf1: {  // POP AF
        } break;
        case 0xf2: {  // LD A, (C)
        } break;
        case 0xf3: {  // DI
        } break;
        case 0xf4: {  // NO INSTRUCTION
            undefined(0xf4);
        } break;
        case 0xf5: {  // PUSH AF
        } break;
        case 0xf6: {  // OR d8
        } break;
        case 0xf7: {  // RST 30H
        } break;
        case 0xf8: {  // LD HL, SP+r8
        } break;
        case 0xf9: {  // LD SP, HL
        } break;
        case 0xfa: {  // LD A, (a16)
        } break;
        case 0xfb: {  // EI
        } break;
        case 0xfc: {  // NO INSTRUCTION
            undefined(0xfc);
        } break;
        case 0xfd: {  // NO INSTRUCTION
            undefined(0xfd);
        } break;
        case 0xfe: {  // CP d8
        } break;
        case 0xff: {  // RST 38H
        } break;
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
    do_instruction(0x32);

    assert(memory[0x00ff] == 0x45);
    assert(registers.HL == 0x00fe);
    */

    while(1)
    {
        // TODO interrupts
        do_instruction(memory[registers.PC++]);
        total_clock.m += tick_clock.m;
        total_clock.t += tick_clock.t;

        if (registers.PC > 255) break;
    }

    printf("done...");

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

