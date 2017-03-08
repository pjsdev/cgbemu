#include <stdio.h>
#include <assert.h>

#include "memory.h"
#include "common.h"
#include "cpu.h"
#include "logging.h"

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

char interrupt_master_enable = 0;

/*
#define FLAGS_ISSET(x) (cpu_registers.F & (x))
#define FLAGS_SET(x) (cpu_registers.F |= (x))
// clear by inverting the bit position then ANDing it.
#define FLAGS_CLEAR(x) (cpu_registers.F &= ~(x))
*/

void cpu_run_tests(){
    // test rotate left carry
    cpu_registers.A = 0xf0; // 11110000
    cpu_do_instruction(0x07);
    assert(cpu_registers.A == 0xe1); // 11100001
    assert(cpu_registers.F & FLAGS_CARRY);
    
    cpu_registers.HL = 0x0001;
    cpu_do_instruction(0x29);
    assert(cpu_registers.HL == 0x0002);

    /* test mem_read_u16
    assert(memory[cpu_registers.PC++] == 0x31);
    assert(mem_read_u16(cpu_registers.PC) == 0xfffe);
    */

    /* test load_into_addr_from_r8 && hl--   (0x32)
    cpu_registers.HL = 0x00ff; // addr
    cpu_registers.A = 0x45;
    cpu_do_instruction(0x32);

    assert(memory[0x00ff] == 0x45);
    assert(cpu_registers.HL == 0x00fe);
    */

   
}

void set_ticks(int t){
    cpu_tick_clock.t = t;
    cpu_tick_clock.m = t/4;
}

void increment_r16(u16* operand){
    // no flags
    (*operand)++;
    set_ticks(8);
}

void increment_r8(u8* operand){
    cpu_registers.F = 0;
   
    if (*operand == 0xff)
    {
        // we are about to produce a 0
        // we are about to carry
        cpu_registers.F |= FLAGS_ZERO;
        cpu_registers.F |= FLAGS_CARRY;
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
    cpu_registers.F = 0;

    if (*operand == 0x00)
    {
        cpu_registers.F |= FLAGS_ZERO;
        cpu_registers.F |= FLAGS_CARRY;
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
    cpu_registers.F = 0;
    // TODO flags
    cpu_registers.A += *rhs;
    set_ticks(4);
} 

void adc_a_r8(u8* rhs){
    cpu_registers.F = 0;
    // TODO flags
    set_ticks(4);
} 

void sub_a_r8(u8* rhs){
    cpu_registers.F = 0;
    cpu_registers.A -= *rhs;
    set_ticks(4);
}

void subc_a_r8(u8* rhs){
    cpu_registers.F = 0;
    set_ticks(4);
}

void xor_a_r8(u8* rhs){
    cpu_registers.F = 0;
    cpu_registers.A ^= *rhs;

    if(cpu_registers.A == 0x00) cpu_registers.F |= FLAGS_ZERO;
    set_ticks(4);
}

void or_a_r8(u8* rhs){
    cpu_registers.F = 0;
    cpu_registers.A |= *rhs;

    if(cpu_registers.A == 0x00) cpu_registers.F |= FLAGS_ZERO;
    set_ticks(4);
}

void and_a_r8(u8* rhs){
    cpu_registers.F = 0;
    cpu_registers.F |= FLAGS_HALFCARRY;

    cpu_registers.A &= *rhs;

    if(cpu_registers.A == 0x00) cpu_registers.F |= FLAGS_ZERO;
    set_ticks(4);
}

void call(){
    // grab the addr we want to move to
    u16 call_addr = mem_read_u16(cpu_registers.PC);

    // consume the PC for the read
    cpu_registers.PC += 2;

    // move the stack pointer down
    cpu_registers.SP -= 2;

    // record our current address
    mem_write_u16(cpu_registers.SP, cpu_registers.PC);

    // set the new address
    cpu_registers.PC = call_addr;

    set_ticks(24);
}

void nop(){
    set_ticks(4);
}

void rotate_right(u8* operand, int ticks){
    u8 old_carry = cpu_registers.F & FLAGS_CARRY;
    cpu_registers.F = 0; 

    u8 carry = ((*operand) & 0x01);
    cpu_registers.F |= (carry << 4);

    (*operand) = (*operand) >> 1;
    (*operand) |= old_carry << 3;
    set_ticks(ticks);
}

void rotate_left(u8* operand, int ticks){
    u8 old_carry = cpu_registers.F & FLAGS_CARRY;
    cpu_registers.F = 0;

    // grab my carry and put it in carry flag
    u8 carry = ((*operand) & 0x80);
    cpu_registers.F |= (carry >> 3);

    // shift left and put the old carry into the first bit
    (*operand) = (*operand) << 1;
    (*operand) |= old_carry >> 4;
    set_ticks(ticks);
}

void rotate_right_carry(u8* operand, int ticks){
    cpu_registers.F = 0;
   
    // store lowest bit
    u8 carry = ((*operand) & 0x01);

    // shift lowest bit out
    (*operand) = (*operand) >> 1;

    // carry old highest bit to lowest
    (*operand) |= (carry << 7);

    // assign carry flag
    cpu_registers.F |= (carry << 4);
    set_ticks(ticks);
}

void rotate_left_carry(u8* operand, int ticks){
    cpu_registers.F = 0;
   
    // store highest bit
    u8 carry = ((*operand) & 0x80);

    // shift highest bit out
    (*operand) = (*operand) << 1;

    // carry old highest bit to lowest
    (*operand) |= (carry >> 7);

    // assign carry flag
    cpu_registers.F |= (carry >> 3);

    set_ticks(ticks);
}

void halt(){
}

void stop(){
}

void undefined(u8 opcode){
    OPLOG(opcode, "Undefined instruction");
}

void load_r8(u8* lhs, u8* rhs){
    *lhs = *rhs;
    set_ticks(4);
}

void load_r8_value(u8* lhs){
    *lhs = mem_read_u8(cpu_registers.PC);
    cpu_registers.PC++;
    set_ticks(8);
}

void load_into_r8_from_addr(u8* lhs, u16* addr){
    *lhs = mem_read_u8(*addr);
    set_ticks(8);
}

void load_into_addr_from_r8(const u16* addr, u8* value){
    mem_write_u8(*addr, *value);
    set_ticks(8);
}

void load_r16_value(u16* lhs){
    *lhs = mem_read_u16(cpu_registers.PC);
    // printf("Loading 0x%04x ", *lhs);
    cpu_registers.PC += 2;
    set_ticks(12);
}

void load_a_into_offset(){
    mem_write_u8(0xff00 + memory[cpu_registers.PC++], cpu_registers.A);
    set_ticks(12);
}

void load_a_into_c_offset(){
    mem_write_u8(0xff00 + cpu_registers.C, cpu_registers.A);
    set_ticks(8);
}

void bit_compare_r8(int bitpos, u8* operand){
    if(*operand & (1 << bitpos)){
        // set to 0
        cpu_registers.F &= ~(FLAGS_ZERO);
    }
    else{
        // zero flag. set to 1
        cpu_registers.F |= FLAGS_ZERO;
    }

    cpu_registers.F &= ~(FLAGS_NEGATIVE);
    cpu_registers.F |= FLAGS_CARRY;
    set_ticks(8);
}

void bit_compare_at_addr(int bitpos, u16* addr){
    u8 value = mem_read_u8(*addr);
    bit_compare_r8(bitpos, &value);
    set_ticks(16);
}

void do_cb_instruction(){
    u8 opcode = memory[cpu_registers.PC++];
    switch(opcode){
        case 0x7c: { // BIT 7, H
            bit_compare_r8(7, &cpu_registers.H);
            OPLOG(0x7c, "BIT 7, H");
        } break;
        default:
            OPLOG(opcode, "Opcode not implemented CB");
    }
}

void jump_nz(){
    signed char relative_addr = memory[cpu_registers.PC++];
    // 0 means that we had a non-zero value
    if ((cpu_registers.F & FLAGS_ZERO) == 0){ 
        cpu_registers.PC += relative_addr; 
    } 

    set_ticks(8);
}

void cpu_do_instruction(u8 instruction){
    switch(instruction){
        case 0x00: {  // NOP
            nop();
            OPLOG(0x00, "NOP");
        } break;
        case 0x01: {  // LD BC, d16
            load_r16_value(&cpu_registers.BC);
            OPLOG(0x01, "LD BC, d16");
        } break;
        case 0x02: {  // LD (BC), A
            load_into_addr_from_r8(&cpu_registers.BC, &cpu_registers.A);
            OPLOG(0x02, "LD (BC), A");
        } break;
        case 0x03: {  // INC BC
            increment_r16(&cpu_registers.BC);
            OPLOG(0x03, "INC BC");
        } break;
        case 0x04: {  // INC B
            increment_r8(&cpu_registers.B);
            OPLOG(0x04, "INC B");
        } break;
        case 0x05: {  // DEC B
            decrement_r8(&cpu_registers.B);
            OPLOG(0x05, "DEC B");
        } break;
        case 0x06: {  // LD B, d8
            load_r8_value(&cpu_registers.B);
            OPLOG(0x06, "LD B, d8");
        } break;
        case 0x07: {  // RLCA
            rotate_left_carry(&cpu_registers.A, 4);
            OPLOG(0x07, "RLCA");
        } break;
        case 0x08: {  // LD (a16), SP
            OPLOG(0x08, "LD (a16), SP");
        } break;
        case 0x09: {  // ADD HL, BC
            add_r16(&cpu_registers.HL, &cpu_registers.BC);
            OPLOG(0x09, "ADD HL, BC");
        } break;
        case 0x0a: {  // LD A, (BC)
            load_into_r8_from_addr(&cpu_registers.A, &cpu_registers.BC);
            OPLOG(0x0a, "LD A, (BC)");
        }  break;
        case 0x0b: {  // DEC BC
            decrement_r16(&cpu_registers.BC);
            OPLOG(0x0b, "DEC BC");
        } break;
        case 0x0c: {  // INC C
            increment_r8(&cpu_registers.C);
            OPLOG(0x0c, "INC C");
        }  break;
        case 0x0d: {  // DEC C
            decrement_r8(&cpu_registers.C);
            OPLOG(0x0d, "DEC C");
        } break;
        case 0x0e: {  // LD C, d8
            load_r8_value(&cpu_registers.C);
            OPLOG(0x0e, "LD C, d8");
        }  break;
        case 0x0f: {  // RRCA
            rotate_right_carry(&cpu_registers.A, 4);
            OPLOG(0x0f, "RRCA");
        } break;
        case 0x10: {  // STOP 0
            stop();
            OPLOG(0x10, "STOP 0");
        } break;
        case 0x11: {  // LD DE, d16
            load_r16_value(&cpu_registers.DE);
            OPLOG(0x11, "LD DE, d16");
        } break;
        case 0x12: {  // LD (DE), A
            load_into_addr_from_r8(&cpu_registers.DE, &cpu_registers.A);
            OPLOG(0x12, "LD (DE), A");
        } break;
        case 0x13: {  // INC DE
            increment_r16(&cpu_registers.DE);
            OPLOG(0x13, "INC DE");
        } break;
        case 0x14: {  // INC D
            increment_r8(&cpu_registers.D);
            OPLOG(0x14, "INC D");
        }  break;
        case 0x15: {  // DEC D
            decrement_r8(&cpu_registers.D);
            OPLOG(0x15, "DEC D");
        }   break;
        case 0x16: {  // LD D, d8
            load_r8_value(&cpu_registers.D);
            OPLOG(0x16, "LD D, d8");
        }  break;
        case 0x17: {  // RLA
            rotate_left(&cpu_registers.A, 4);
            OPLOG(0x17, "RLA");
        } break;
        case 0x18: {  // JR r8
            OPLOG(0x18, "JR r8");
        } break;
        case 0x19: {  // ADD HL, DE
            add_r16(&cpu_registers.HL, &cpu_registers.DE);
            OPLOG(0x19, "ADD HL, DE");
        } break;
        case 0x1a: {  // LD A, (DE)
            load_into_r8_from_addr(&cpu_registers.A, &cpu_registers.DE);
            OPLOG(0x1a, "LD A, (DE)");
        } break;
        case 0x1b: {  // DEC DE
            decrement_r16(&cpu_registers.DE);
            OPLOG(0x1b, "DEC DE");
        } break;
        case 0x1c: {  // INC E
            increment_r8(&cpu_registers.E);
            OPLOG(0x1c, "INC E");
        } break;
        case 0x1d: {  // DEC E
            decrement_r8(&cpu_registers.E);
            OPLOG(0x1d, "DEC E");
        } break;
        case 0x1e: {  // LD E, d8
            load_r8_value(&cpu_registers.E);
            OPLOG(0x1e, "LD E, d8");
        }  break;
        case 0x1f: {  // RRA
            rotate_right(&cpu_registers.A, 4);
            OPLOG(0x1f, "RRA");
        } break;
        case 0x20: {  // JR NZ, r8
            jump_nz();
            OPLOG(0x20, "JR NZ, r8");
        } break;
        case 0x21: {  // LD HL, d16
            load_r16_value(&cpu_registers.HL);
            OPLOG(0x21, "LD HL, d16");
        } break;
        case 0x22: {  // LD (HL+), A
            load_into_addr_from_r8(&cpu_registers.HL, &cpu_registers.A);
            cpu_registers.HL++;
            OPLOG(0x22, "LD (HL+), A");
        } break;
        case 0x23: {  // INC HL
            increment_r16(&cpu_registers.HL);
            OPLOG(0x23, "INC HL");
        } break;
        case 0x24: {  // INC H
            increment_r8(&cpu_registers.H);
            OPLOG(0x24, "INC H");
        } break;
        case 0x25: {  // DEC H
            decrement_r8(&cpu_registers.H);
            OPLOG(0x25, "DEC H");
        } break;
        case 0x26: {  // LD H, d8
            load_r8_value(&cpu_registers.H);
            OPLOG(0x26, "LD H, d8");
        }  break;
        case 0x27: {  // DAA
            OPLOG(0x27, "DAA");
        } break;
        case 0x28: {  // JR Z, r8
            OPLOG(0x28, "JR Z, r8");
        } break;
        case 0x29: {  // ADD HL, HL
            add_r16(&cpu_registers.HL, &cpu_registers.HL);
            OPLOG(0x29, "ADD HL, HL");
        } break;
        case 0x2a: {  // LD A, (HL+)
            OPLOG(0x2a, "LD A, (HL+)");
        } break;
        case 0x2b: {  // DEC HL
            decrement_r16(&cpu_registers.HL);
            OPLOG(0x2b, "DEC HL");
        } break;
        case 0x2c: {  // INC L
            increment_r8(&cpu_registers.L);
            OPLOG(0x2c, "INC L");
        } break;
        case 0x2d: {  // DEC L
            decrement_r8(&cpu_registers.L);
            OPLOG(0x2d, "DEC L");
        } break;
        case 0x2e: {  // LD L, d8
            load_r8_value(&cpu_registers.L);
            OPLOG(0x2e, "LD L, d8");
        }  break;
        case 0x2f: {  // CPL
            OPLOG(0x2f, "CPL");
        } break;
        case 0x30: {  // JR NC, r8
            OPLOG(0x30, "JR NC, r8");
        } break;
        case 0x31: {  // LD SP, d16
            load_r16_value(&cpu_registers.SP);
            OPLOG(0x31, "LD SP, d16");
        } break;
        case 0x32: {  // LD (HL-), A
            load_into_addr_from_r8(&cpu_registers.HL, &cpu_registers.A);
            cpu_registers.HL--;
            OPLOG(0x32, "LD (HL-), A");
        } break;
        case 0x33: {  // INC SP
            increment_r16(&cpu_registers.SP);
            OPLOG(0x33, "INC SP");
        } break;
        case 0x34: {  // INC (HL)
            OPLOG(0x34, "INC (HL)");
        } break;
        case 0x35: {  // DEC (HL)
            OPLOG(0x35, "DEC (HL)");
        } break;
        case 0x36: {  // LD (HL), d8
            OPLOG(0x36, "LD (HL), d8");
        } break;
        case 0x37: {  // SCF
            OPLOG(0x37, "SCF");
        } break;
        case 0x38: {  // JR C, r8
            OPLOG(0x38, "JR C, r8");
        } break;
        case 0x39: {  // ADD HL, SP
            add_r16(&cpu_registers.HL, &cpu_registers.SP);
            OPLOG(0x39, "ADD HL, SP");
        } break;
        case 0x3a: {  // LD A, (HL-)
            OPLOG(0x3a, "LD A, (HL-)");
        } break;
        case 0x3b: {  // DEC SP
            decrement_r16(&cpu_registers.SP);
            OPLOG(0x3b, "DEC SP");
        } break;
        case 0x3c: {  // INC A
            increment_r8(&cpu_registers.A);
            OPLOG(0x3c, "INC A");
        } break;
        case 0x3d: {  // DEC A
            decrement_r8(&cpu_registers.A);
            OPLOG(0x3d, "DEC A");
        } break;
        case 0x3e: {  // LD A, d8
            load_r8_value(&cpu_registers.A);
            OPLOG(0x3e, "LD A, d8");
        }  break;
        case 0x3f: {  // CCF
            OPLOG(0x3f, "CCF");
        } break;
        case 0x40: {  // LD B, B
            load_r8(&cpu_registers.B, &cpu_registers.B);
            OPLOG(0x40, "LD B, B");
        } break;
        case 0x41: {  // LD B, C
            load_r8(&cpu_registers.B, &cpu_registers.C);
            OPLOG(0x41, "LD B, C");
        } break;
        case 0x42: {  // LD B, D
            load_r8(&cpu_registers.B, &cpu_registers.D);
            OPLOG(0x42, "LD B, D");
        } break;
        case 0x43: {  // LD B, E 
            load_r8(&cpu_registers.B, &cpu_registers.E);
            OPLOG(0x43, "LD B, E");
        } break;
        case 0x44: {  // LD B, H  
            load_r8(&cpu_registers.B, &cpu_registers.H);
            OPLOG(0x44, "LD B, H");
        } break;
        case 0x45: {  // LD B, L  
            load_r8(&cpu_registers.B, &cpu_registers.L);
            OPLOG(0x45, "LD B, L");
        } break;
        case 0x46: {  // LD B, (HL)  
            OPLOG(0x46, "LD B, (HL)");
        } break;
        case 0x47: {  // LD B, A 
            load_r8(&cpu_registers.B, &cpu_registers.A);
            OPLOG(0x47, "LD B, A");
        } break;
        case 0x48: {  // LD C, B
            load_r8(&cpu_registers.C, &cpu_registers.B);
            OPLOG(0x48, "LD C, B");
        } break;
        case 0x49: {  // LD C, C
            load_r8(&cpu_registers.C, &cpu_registers.C);
            OPLOG(0x49, "LD C, C");
        } break;
        case 0x4a: {  // LD C, D 
            load_r8(&cpu_registers.C, &cpu_registers.D);
            OPLOG(0x4a, "LD C, D");
        } break;
        case 0x4b: {  // LD C, E
            load_r8(&cpu_registers.C, &cpu_registers.E);
            OPLOG(0x4b, "LD C, E");
        } break;
        case 0x4c: {  // LD C, H
            load_r8(&cpu_registers.C, &cpu_registers.H);
            OPLOG(0x4c, "LD C, H");
        } break;
        case 0x4d: {  // LD C, L
            load_r8(&cpu_registers.C, &cpu_registers.L);
            OPLOG(0x4d, "LD C, L");
        } break;
        case 0x4e: {  // LD C, (HL)
            OPLOG(0x4e, "LD C, (HL)");
        } break;
        case 0x4f: {  // LD C, A
            load_r8(&cpu_registers.C, &cpu_registers.A);
            OPLOG(0x4f, "LD C, A");
        } break;
        case 0x50: {  // LD D, B
            load_r8(&cpu_registers.D, &cpu_registers.B);
            OPLOG(0x50, "LD D, B");
        } break;
        case 0x51: {  // LD D, C
            load_r8(&cpu_registers.D, &cpu_registers.C);
            OPLOG(0x51, "LD D, C");
        } break;
        case 0x52: {  // LD D, D
            load_r8(&cpu_registers.D, &cpu_registers.D);
            OPLOG(0x52, "LD D, D");
        } break;
        case 0x53: {  // LD D, E
            load_r8(&cpu_registers.D, &cpu_registers.E);
            OPLOG(0x53, "LD D, E");
        } break;
        case 0x54: {  // LD D, H
            load_r8(&cpu_registers.D, &cpu_registers.H);
            OPLOG(0x54, "LD D, H");
        } break;
        case 0x55: {  // LD D, L
            load_r8(&cpu_registers.D, &cpu_registers.L);
            OPLOG(0x55, "LD D, L");
        } break;
        case 0x56: {  // LD D, (HL)
            OPLOG(0x56, "LD D, (HL)");
        } break;
        case 0x57: {  // LD D, A
            load_r8(&cpu_registers.D, &cpu_registers.A);
            OPLOG(0x57, "LD D, A");
        } break;
        case 0x58: {  // LD E, B
            load_r8(&cpu_registers.E, &cpu_registers.B);
            OPLOG(0x58, "LD E, B");
        } break;
        case 0x59: {  // LD E, C
            load_r8(&cpu_registers.E, &cpu_registers.C);
            OPLOG(0x59, "LD E, C");
        } break;
        case 0x5a: {  // LD E, D
            load_r8(&cpu_registers.E, &cpu_registers.D);
            OPLOG(0x5a, "LD E, D");
        } break;
        case 0x5b: {  // LD E, E
            load_r8(&cpu_registers.E, &cpu_registers.E);
            OPLOG(0x5b, "LD E, E");
        } break;
        case 0x5c: {  // LD E, H
            load_r8(&cpu_registers.E, &cpu_registers.H);
            OPLOG(0x5c, "LD E, H");
        } break;
        case 0x5d: {  // LD E, L
            load_r8(&cpu_registers.E, &cpu_registers.L);
            OPLOG(0x5d, "LD E, L");
        } break;
        case 0x5e: {  // LD E, (HL)
            OPLOG(0x5e, "LD E, (HL)");
        } break;
        case 0x5f: {  // LD E, A
            load_r8(&cpu_registers.E, &cpu_registers.A);
            OPLOG(0x5f, "LD E, A");
        } break;
        case 0x60: {  // LD H, B 
            load_r8(&cpu_registers.H, &cpu_registers.B);
            OPLOG(0x60, "LD H, B");
        } break;
        case 0x61: {  // LD H, C
            load_r8(&cpu_registers.H, &cpu_registers.C);
            OPLOG(0x61, "LD H, C");
        } break;
        case 0x62: {  // LD H, D 
            load_r8(&cpu_registers.H, &cpu_registers.D);
            OPLOG(0x62, "LD H, D");
        } break;
        case 0x63: {  // LD H, E 
            load_r8(&cpu_registers.H, &cpu_registers.E);
            OPLOG(0x63, "LD H, E");
        } break;
        case 0x64: {  // LD H, H 
            load_r8(&cpu_registers.H, &cpu_registers.H);
            OPLOG(0x64, "LD H, H");
        } break;
        case 0x65: {  // LD H, L 
            load_r8(&cpu_registers.H, &cpu_registers.L);
            OPLOG(0x65, "LD H, L");
        } break;
        case 0x66: {  // LD H, (HL)                                   
            OPLOG(0x66, "LD H, (HL)");
        } break;
        case 0x67: {  // LD H, A 
            load_r8(&cpu_registers.H, &cpu_registers.A);
            OPLOG(0x67, "LD H, A");
        } break;
        case 0x68: {  // LD L, B 
            load_r8(&cpu_registers.L, &cpu_registers.B);
            OPLOG(0x68, "LD L, B");
        } break;
        case 0x69: {  // LD L, C 
            load_r8(&cpu_registers.L, &cpu_registers.C);
            OPLOG(0x69, "LD L, C");
        } break;
        case 0x6a: {  // LD L, D 
            load_r8(&cpu_registers.L, &cpu_registers.D);
            OPLOG(0x6a, "LD L, D");
        } break;
        case 0x6b: {  // LD L, E 
            load_r8(&cpu_registers.L, &cpu_registers.E);
            OPLOG(0x6b, "LD L, E");
        } break;
        case 0x6c: {  // LD L, H 
            load_r8(&cpu_registers.L, &cpu_registers.H);
            OPLOG(0x6c, "LD L, H");
        } break;
        case 0x6d: {  // LD L, L 
            load_r8(&cpu_registers.L, &cpu_registers.L);
            OPLOG(0x6d, "LD L, L");
        } break;
        case 0x6e: {  // LD L, (HL)                                   
            OPLOG(0x6e, "LD L, (HL)");
        } break;
        case 0x6f: {  // LD L, A 
            load_r8(&cpu_registers.L, &cpu_registers.A);
            OPLOG(0x6f, "LD L, A");
        } break;
        case 0x70: {  // LD (HL), B
            load_into_addr_from_r8(&cpu_registers.HL, &cpu_registers.B);
            OPLOG(0x70, "LD (HL), B");
        }  break;
        case 0x71: {  // LD (HL), C 
            load_into_addr_from_r8(&cpu_registers.HL, &cpu_registers.C);
            OPLOG(0x71, "LD (HL), C");
        }  break;
        case 0x72: {  // LD (HL), D
            load_into_addr_from_r8(&cpu_registers.HL, &cpu_registers.D);
            OPLOG(0x72, "LD (HL), D");
        }  break;
        case 0x73: {  // LD (HL), E
            load_into_addr_from_r8(&cpu_registers.HL, &cpu_registers.E);
            OPLOG(0x73, "LD (HL), E");
        }  break;
        case 0x74: {  // LD (HL), H
            load_into_addr_from_r8(&cpu_registers.HL, &cpu_registers.H);
            OPLOG(0x74, "LD (HL), H");
        }  break;
        case 0x75: {  // LD (HL), L
            load_into_addr_from_r8(&cpu_registers.HL, &cpu_registers.L);
            OPLOG(0x75, "LD (HL), L");
        }  break;
        case 0x76: {  // HALT
            halt();
            OPLOG(0x76, "HALT");
        } break;
        case 0x77: {  // LD (HL), A
            load_into_addr_from_r8(&cpu_registers.HL, &cpu_registers.A);
            OPLOG(0x77, "LD (HL), A");
        } break;
        case 0x78: {  // LD A, B  
            load_r8(&cpu_registers.A, &cpu_registers.B);
            OPLOG(0x78, "LD A, B");
        } break;
        case 0x79: {  // LD A, C  
            load_r8(&cpu_registers.A, &cpu_registers.C);
            OPLOG(0x79, "LD A, C");
        } break;
        case 0x7a: {  // LD A, D  
            load_r8(&cpu_registers.A, &cpu_registers.D);
            OPLOG(0x7a, "LD A, D");
        } break;
        case 0x7b: {  // LD A, E  
            load_r8(&cpu_registers.A, &cpu_registers.E);
            OPLOG(0x7b, "LD A, E");
        } break;
        case 0x7c: {  // LD A, H  
            load_r8(&cpu_registers.A, &cpu_registers.H);
            OPLOG(0x7c, "LD A, H");
        } break;
        case 0x7d: {  // LD A, L  
            load_r8(&cpu_registers.A, &cpu_registers.L);
            OPLOG(0x7d, "LD A, L");
        } break;
        case 0x7e: {  // LD A, (HL)  
            OPLOG(0x7e, "LD A, (HL)");
        } break;
        case 0x7f: {  // LD A, A  
            load_r8(&cpu_registers.A, &cpu_registers.A);
            OPLOG(0x7f, "LD A, A");
        } break;
        case 0x80: {  // ADD A, B
            add_a_r8(&cpu_registers.B);
            OPLOG(0x80, "ADD A, B");
        } break;
        case 0x81: {  // ADD A, C
            add_a_r8(&cpu_registers.C);
            OPLOG(0x81, "ADD A, C");
        } break;
        case 0x82: {  // ADD A, D
            add_a_r8(&cpu_registers.D);
            OPLOG(0x82, "ADD A, D");
        } break;
        case 0x83: {  // ADD A, E
            add_a_r8(&cpu_registers.E);
            OPLOG(0x83, "ADD A, E");
        } break;
        case 0x84: {  // ADD A, H
            add_a_r8(&cpu_registers.H);
            OPLOG(0x84, "ADD A, H");
        } break;
        case 0x85: {  // ADD A, L
            add_a_r8(&cpu_registers.L);
            OPLOG(0x85, "ADD A, L");
        } break;
        case 0x86: {  // ADD A, (HL)
            OPLOG(0x86, "ADD A, (HL)");
        } break;
        case 0x87: {  // ADD A, A
            add_a_r8(&cpu_registers.A);
            OPLOG(0x87, "ADD A, A");
        } break;
        case 0x88: {  // ADC A, B 
            adc_a_r8(&cpu_registers.B);
            OPLOG(0x88, "ADC A, B");
        } break;
        case 0x89: {  // ADC A, C 
            adc_a_r8(&cpu_registers.C);
            OPLOG(0x89, "ADC A, C");
        } break;
        case 0x8a: {  // ADC A, D 
            adc_a_r8(&cpu_registers.D);
            OPLOG(0x8a, "ADC A, D");
        } break;
        case 0x8b: {  // ADC A, E 
            adc_a_r8(&cpu_registers.E);
            OPLOG(0x8b, "ADC A, E");
        } break;
        case 0x8c: {  // ADC A, H 
            adc_a_r8(&cpu_registers.H);
            OPLOG(0x8c, "ADC A, H");
        } break;
        case 0x8d: {  // ADC A, L 
            adc_a_r8(&cpu_registers.L);
            OPLOG(0x8d, "ADC A, L");
        } break;
        case 0x8e: {  // ADC A, (HL)                      
            OPLOG(0x8e, "ADC A, (HL)");
        } break;
        case 0x8f: {  // ADC A, A 
            adc_a_r8(&cpu_registers.A);
            OPLOG(0x8f, "ADC A, A");
        } break;
        case 0x90: {  // SUB B
            sub_a_r8(&cpu_registers.B);
            OPLOG(0x90, "SUB B");
        } break;
        case 0x91: {  // SUB C
            sub_a_r8(&cpu_registers.C);
            OPLOG(0x91, "SUB C");
        } break;
        case 0x92: {  // SUB D
            sub_a_r8(&cpu_registers.D);
            OPLOG(0x92, "SUB D");
        } break;
        case 0x93: {  // SUB E
            sub_a_r8(&cpu_registers.E);
            OPLOG(0x93, "SUB E");
        } break;
        case 0x94: {  // SUB H
            sub_a_r8(&cpu_registers.H);
            OPLOG(0x94, "SUB H");
        } break;
        case 0x95: {  // SUB L
            sub_a_r8(&cpu_registers.L);
            OPLOG(0x95, "SUB L");
        } break;
        case 0x96: {  // SUB (HL)
            OPLOG(0x96, "SUB (HL)");
        } break;
        case 0x97: {  // SUB A
            sub_a_r8(&cpu_registers.A);
            OPLOG(0x97, "SUB A");
        } break;
        case 0x98: {  // SBC A, B
            subc_a_r8(&cpu_registers.B);
            OPLOG(0x98, "SBC A, B");
        } break;
        case 0x99: {  // SBC A, C
            subc_a_r8(&cpu_registers.C);
            OPLOG(0x99, "SBC A, C");
        } break;
        case 0x9a: {  // SBC A, D
            subc_a_r8(&cpu_registers.D);
            OPLOG(0x9a, "SBC A, D");
        } break;
        case 0x9b: {  // SBC A, E
            subc_a_r8(&cpu_registers.E);
            OPLOG(0x9b, "SBC A, E");
        } break;
        case 0x9c: {  // SBC A, H
            subc_a_r8(&cpu_registers.H);
            OPLOG(0x9c, "SBC A, H");
        } break;
        case 0x9d: {  // SBC A, L
            subc_a_r8(&cpu_registers.L);
            OPLOG(0x9d, "SBC A, L");
        } break;
        case 0x9e: {  // SBC A, (HL)
            OPLOG(0x9e, "SBC A, (HL)");
        } break;
        case 0x9f: {  // SBC A, A
            subc_a_r8(&cpu_registers.A);
            OPLOG(0x9f, "SBC A, A");
        } break;
        case 0xa0: {  // AND B 
            and_a_r8(&cpu_registers.B);
            OPLOG(0xa0, "AND B");
        } break;
        case 0xa1: {  // AND C
            and_a_r8(&cpu_registers.C);
            OPLOG(0xa1, "AND C");
        } break;
        case 0xa2: {  // AND D
            and_a_r8(&cpu_registers.D);
            OPLOG(0xa2, "AND D");
        } break;
        case 0xa3: {  // AND E
            and_a_r8(&cpu_registers.E);
            OPLOG(0xa3, "AND E");
        } break;
        case 0xa4: {  // AND H
            and_a_r8(&cpu_registers.H);
            OPLOG(0xa4, "AND H");
        } break;
        case 0xa5: {  // AND L
            and_a_r8(&cpu_registers.L);
            OPLOG(0xa5, "AND L");
        } break;
        case 0xa6: {  // AND (HL)
            OPLOG(0xa6, "AND (HL)");
        } break;
        case 0xa7: {  // AND A
            and_a_r8(&cpu_registers.A);
            OPLOG(0xa7, "AND A");
        } break;
        case 0xa8: {  // XOR B
            xor_a_r8(&cpu_registers.B);
            OPLOG(0xa8, "XOR B");
        } break;
        case 0xa9: {  // XOR C
            xor_a_r8(&cpu_registers.C);
            OPLOG(0xa9, "XOR C");
        } break;
        case 0xaa: {  // XOR D
            xor_a_r8(&cpu_registers.D);
            OPLOG(0xaa, "XOR D");
        } break;
        case 0xab: {  // XOR E
            xor_a_r8(&cpu_registers.E);
            OPLOG(0xab, "XOR E");
        } break;
        case 0xac: {  // XOR H
            xor_a_r8(&cpu_registers.H);
            OPLOG(0xac, "XOR H");
        } break;
        case 0xad: {  // XOR L
            xor_a_r8(&cpu_registers.L);
            OPLOG(0xad, "XOR L");
        } break;
        case 0xae: {  // XOR (HL)
            OPLOG(0xae, "XOR (HL)");
        } break;
        case 0xaf: {  // XOR A
            xor_a_r8(&cpu_registers.A);
            OPLOG(0xaf, "XOR A");
        } break;
        case 0xb0: {  // OR B
            or_a_r8(&cpu_registers.B);
            OPLOG(0xb0, "OR B");
        } break;
        case 0xb1: {  // OR C
            or_a_r8(&cpu_registers.C);
            OPLOG(0xb1, "OR C");
        } break;
        case 0xb2: {  // OR D
            or_a_r8(&cpu_registers.D);
            OPLOG(0xb2, "OR D");
        } break;
        case 0xb3: {  // OR E
            or_a_r8(&cpu_registers.E);
            OPLOG(0xb3, "OR E");
        } break;
        case 0xb4: {  // OR H
            or_a_r8(&cpu_registers.H);
            OPLOG(0xb4, "OR H");
        } break;
        case 0xb5: {  // OR L
            or_a_r8(&cpu_registers.L);
            OPLOG(0xb5, "OR L");
        } break;
        case 0xb6: {  // OR (HL)
            OPLOG(0xb6, "OR (HL)");
        } break;
        case 0xb7: {  // OR A
            or_a_r8(&cpu_registers.A);
            OPLOG(0xb7, "OR A");
        } break;
        case 0xb8: {  // CP B
            OPLOG(0xb8, "CP B");
        } break;
        case 0xb9: {  // CP C
            OPLOG(0xb9, "CP C");
        } break;
        case 0xba: {  // CP D
            OPLOG(0xba, "CP D");
        } break;
        case 0xbb: {  // CP E
            OPLOG(0xbb, "CP E");
        } break;
        case 0xbc: {  // CP H
            OPLOG(0xbc, "CP H");
        } break;
        case 0xbd: {  // CP L
            OPLOG(0xbd, "CP L");
        } break;
        case 0xbe: {  // CP (HL)
            OPLOG(0xbe, "CP (HL)");
        } break;
        case 0xbf: {  // CP A
            OPLOG(0xbf, "CP A");
        } break;
        case 0xc0: {  // RET NZ
            OPLOG(0xc0, "RET NZ");
        } break;
        case 0xc1: {  // POP BC 
            OPLOG(0xc1, "POP BC");
        } break;
        case 0xc2: {  // JP NZ, a16
            OPLOG(0xc2, "JP NZ, a16");
        } break;
        case 0xc3: {  // JP a16
            OPLOG(0xc3, "JP a16");
        } break;
        case 0xc4: {  // CALL NZ, a16
            OPLOG(0xc4, "CALL NZ, a16");
        } break;
        case 0xc5: {  // PUSH BC
            OPLOG(0xc5, "PUSH BC");
        } break;
        case 0xc6: {  // ADD A, d8
            OPLOG(0xc6, "ADD A, d8");
        } break;
        case 0xc7: {  // RST 00H
            OPLOG(0xc7, "RST 00H");
        } break;
        case 0xc8: {  // RET Z
            OPLOG(0xc8, "RET Z");
        } break;
        case 0xc9: {  // RET
            OPLOG(0xc9, "RET");
        } break;
        case 0xca: {  // JP Z, a16
            OPLOG(0xca, "JP Z, a16");
        } break;
        case 0xcb: {  // PREFIX CB
            do_cb_instruction();
        } break;
        case 0xcc: {  // CALL Z, a16
            OPLOG(0xcc, "CALL Z, a16");
        } break;
        case 0xcd: {  // CALL a16
            call();
            OPLOG(0xcd, "CALL a16");
        } break;
        case 0xce: {  // ADC A, d8
            OPLOG(0xce, "ADC A, d8");
        } break;
        case 0xcf: {  // RST 08H
            OPLOG(0xcf, "RST 08H");
        } break;
        case 0xd0: {  // RET NC
            OPLOG(0xd0, "RET NC");
        } break;
        case 0xd1: {  // POP DE
            OPLOG(0xd1, "POP DE");
        } break;
        case 0xd2: {  // JP NC, a16
            OPLOG(0xd2, "JP NC, a16");
        } break;
        case 0xd3: {  // NO INSTRUCTION
            undefined(0xd3);
        } break;
        case 0xd4: {  // CALL NC, a16 
            OPLOG(0xd4, "CALL NC, a16");
        } break;
        case 0xd5: {  // PUSH DE
            OPLOG(0xd5, "PUSH DE");
        } break;
        case 0xd6: {  // SUB d8
            OPLOG(0xd6, "SUB d8");
        } break;
        case 0xd7: {  // RST 10H
            OPLOG(0xd7, "RST 10H");
        } break;
        case 0xd8: {  // RET C
            OPLOG(0xd8, "RET C");
        } break;
        case 0xd9: {  // RETI
            OPLOG(0xd9, "RETI");
        } break;
        case 0xda: {  // JP C, a16
            OPLOG(0xda, "JP C, a16");
        } break;
        case 0xdb: {  // NO INSTRUCTION
            undefined(0xdb);
        } break;
        case 0xdc: {  // CALL C, a16
            OPLOG(0xdc, "CALL C, a16");
        } break;
        case 0xdd: {  // NO INSTRUCTION
            undefined(0xdd);
        } break;
        case 0xde: {  // SBC A, d8
            OPLOG(0xde, "SBC A, d8");
        } break;
        case 0xdf: {  // RST 18H
            OPLOG(0xdf, "RST 18H");
        } break;
        case 0xe0: {  // LDH (a8), A
            load_a_into_offset();
            OPLOG(0xe0, "LDH (a8), A");
        } break;
        case 0xe1: {  // POP HL
            OPLOG(0xe1, "POP HL");
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
            OPLOG(0xe5, "PUSH HL");
        } break;
        case 0xe6: {  // AND d8
            OPLOG(0xe6, "AND d8");
        } break;
        case 0xe7: {  // RST 20H
            OPLOG(0xe7, "RST 20H");
        } break;
        case 0xe8: {  // ADD SP, r8
            OPLOG(0xe8, "ADD SP, r8");
        } break;
        case 0xe9: {  // JP (HL)
            OPLOG(0xe9, "JP (HL)");
        } break;
        case 0xea: {  // LD (a16), A
            OPLOG(0xea, "LD (a16), A");
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
            OPLOG(0xee, "XOR d8");
        } break;
        case 0xef: {  // RST 28H
            OPLOG(0xef, "RST 28H");
        } break;
        case 0xf0: {  // LDH A,(a8)  
            OPLOG(0xf0, "LDH A,(a8)");
        } break;
        case 0xf1: {  // POP AF
            OPLOG(0xf1, "POP AF");
        } break;
        case 0xf2: {  // LD A, (C)
            OPLOG(0xf2, "LD A, (C)");
        } break;
        case 0xf3: {  // DI
            OPLOG(0xf3, "DI");
        } break;
        case 0xf4: {  // NO INSTRUCTION
            undefined(0xf4);
        } break;
        case 0xf5: {  // PUSH AF
            OPLOG(0xf5, "PUSH AF");
        } break;
        case 0xf6: {  // OR d8
            OPLOG(0xf6, "OR d8");
        } break;
        case 0xf7: {  // RST 30H
            OPLOG(0xf7, "RST 30H");
        } break;
        case 0xf8: {  // LD HL, SP+r8
            OPLOG(0xf8, "LD HL, SP+r8");
        } break;
        case 0xf9: {  // LD SP, HL
            OPLOG(0xf9, "LD SP, HL");
        } break;
        case 0xfa: {  // LD A, (a16)
            OPLOG(0xfa, "LD A, (a16)");
        } break;
        case 0xfb: {  // EI
            OPLOG(0xfb, "EI");
        } break;
        case 0xfc: {  // NO INSTRUCTION
            undefined(0xfc);
        } break;
        case 0xfd: {  // NO INSTRUCTION
            undefined(0xfd);
        } break;
        case 0xfe: {  // CP d8
            OPLOG(0xfe, "CP d8");
        } break;
        case 0xff: {  // RST 38H
            OPLOG(0xff, "RST 38H");
        } break;
        default:
            printf("Unknown instruction\n");
            break;
    }
}

