#include <stdio.h>
#include <stdarg.h>

#define LOG_BUFFER_MAX (256)
char _log_u8_buffer[LOG_BUFFER_MAX];

void log_with_file_line(const char* file_name, const int line_number, const char* msg, ...)
{
   va_list args;
   va_start(args, msg);
   vsnprintf(_log_u8_buffer, LOG_BUFFER_MAX, msg, args);
   va_end(args);
   printf("%s:%03d %s\n", file_name, line_number, _log_u8_buffer);

   // TODO file logging
}

void OPLOG(unsigned short opcode, const char* memonic){
    printf("0x%02x [%s]\n", opcode, memonic);
}
