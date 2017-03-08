#ifndef LOGGING_H
#define LOGGING_H

void log_with_file_line(const char* file_name, const int line_number, const char* msg, ...);
void OPLOG(unsigned short opcode, const char* memonic);
#define LOG(...) log_with_file_line(__FILE__, __LINE__, __VA_ARGS__)

void debug_print_mem();
void debug_print_cartridge_header();

void debug_tick();
void debug_break(const char* file_name, const int line_number, const char* function_name);
#define BREAK debug_break(__FILE__, __LINE__, __FUNCTION__)

#endif

