#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>

#include "cpu.h"
#include "memory.h"
#include "logging.h"
#include "common.h"
#include "display.h"

#define LOG_BUFFER_MAX (256)
char _log_u8_buffer[LOG_BUFFER_MAX];

int debug_tick_enabled = 0;

void log_with_file_line(const char* file_name, const int line_number, const char* msg, ...)
{
   va_list args;
   va_start(args, msg);
   vsnprintf(_log_u8_buffer, LOG_BUFFER_MAX, msg, args);
   va_end(args);
   printf("%s:%03d %s\n", file_name, line_number, _log_u8_buffer);

   // TODO file logging
}

void PCLOG(){
    if(debug_tick_enabled)
        printf("(0x%02x)\t", cpu_registers.PC);
}

void OPLOG(unsigned short opcode, const char* memonic){
    if(debug_tick_enabled)
        printf("0x%02x\t%s\n", opcode, memonic);
}

void debug_print_registers() {
    printf(" --- Registers ---\n");
    printf(" A: 0x%02X ", cpu_registers.A);
    printf(" F: 0x%02X\n", cpu_registers.F);
    printf(" B: 0x%02X ", cpu_registers.B);
    printf(" C: 0x%02X\n", cpu_registers.C);
    printf(" D: 0x%02X ", cpu_registers.D);
    printf(" E: 0x%02X\n", cpu_registers.E);
    printf(" H: 0x%02X ", cpu_registers.H);
    printf(" L: 0x%02X\n", cpu_registers.L);
    printf("SP: 0x%04X\n", cpu_registers.SP);
    printf("PC: 0x%04X\n", cpu_registers.PC);
    printf(" ----------------\n");
}


void debug_print_mem(){
    char s = getchar(); // eat space
    if (s != ' '){
        printf("Bad format: 'm ffe1'\n");
        return;
    }
    
    char opcode_str[4];
    fgets(opcode_str, 5, stdin);
    unsigned short addr = (unsigned short)strtol(opcode_str, NULL, 16);

    if(!addr){
        printf("Bad format: 'm ffe1'\n");
        return;
    }

    printf("memory[0x%04x] = 0x%02x\n", addr, memory[addr]);
}

void debug_print_cartridge_header(){
    // TODO the rest of the header (tetris has 00s)
    // http://bgb.bircd.org/pandocs.htm#thecartridgeheader
    // - psmith march 9 2017
    u8 title[16];
    memcpy(title, &memory[0x0134], 16);
    
    u8 cgb_flag = mem_read_u8(0x0143);

    printf("Cartridge header...\n\n");
    printf("Title: %s\n", title);

    switch(cgb_flag){
        case 0x80:
            printf("CGB supported, not mandatory\n");
            break;
        case 0xc0:
            printf("CGB mandatory\n");
            break;
        default:
            printf("Unknown CGB Flag 0x%02x\n", cgb_flag);
            break;
    }
}


void debug_break(const char* file_name, const int line_number, const char* function_name){
   int cont = 1;
   while(cont){
       printf("\n%s:%d %s\n> ", file_name, line_number, function_name);
       char c = getchar();
       if (c == '\n') continue;
       switch(c){
           case 'd':
               debug_display();
               break;
           case 'q':
               running = 0;
               cont = 0;
               break;
           case 'h':
               debug_print_cartridge_header();
               break;
           case 'm':
               debug_print_mem();
               break;
           case 'r':
               debug_print_registers();
               break;
           case 't':
               debug_tick_enabled = 1;
               cont = 0;
               break;
           case 'c':
               printf("Continue...\n");
               cont = 0;
               break;
           default:
               printf("Unknown command '%c'\n", c);
               break;
       }

       int tmp = getchar();
       while(tmp != '\n') tmp = getchar(); // consume the rest...
   }

}

void debug_tick(){
    if(debug_tick_enabled){
        debug_tick_enabled = 0;
        BREAK;
    }
}
