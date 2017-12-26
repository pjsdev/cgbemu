#include <stdio.h>
#include <stdlib.h>

#include "logging.h"
#include "common.h"


void print_u16_chunks(u8_buffer* buf){
    for(int i = 1; i <= buf->size; i += 2){
        if(i < buf->size){
            // we have access to two bytes
            printf("%04x ", (buf->data[i-1] << 8) + buf->data[i]);
            if((i + 1) % 16 == 0) printf("\n");
        }
        else{
            printf("%02x ", buf->data[i-1]);
        }
    }
    printf("\n");
}

u8_buffer* malloc_u8_buffer(int size){
    u8_buffer* buf = malloc(sizeof(u8_buffer));
    buf->size = size;
    buf->data = malloc(size);
    return buf;
}

void free_u8_buffer(u8_buffer* buf){
    free(buf->data);
    free(buf);
}

u8_buffer* read_binary_file(const char* filename){
    FILE* src = fopen(filename, "rb");
    if (src == NULL){
        LOG("Could not open file %s", filename);
        return NULL;
    }

    // go to end of file
    fseek(src, 0, SEEK_END); 

    // ftell returns our current position in file
    u8_buffer* buf = malloc_u8_buffer(ftell(src));

    // SEEK_SET means beginning of file
    fseek(src, 0, SEEK_SET);

    int read = fread(buf->data, 1, buf->size, src);

    if (read != buf->size){
        LOG("Could not read all of file %s", filename);
        fclose(src);
        return NULL;
    }

    fclose(src);
    return buf; 
}


