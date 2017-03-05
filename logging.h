
void log_with_file_line(const char* file_name, const int line_number, const char* msg, ...);
void OPLOG(unsigned short opcode, const char* memonic);
#define LOG(...) log_with_file_line(__FILE__, __LINE__, __VA_ARGS__)

