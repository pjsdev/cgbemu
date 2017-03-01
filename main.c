#include <stdio.h>
#include <string.h>

// masks for inspecting Register.F
#define FLAG_MASK_Z (0x80) // zero flag gets set when result of an operation has been zero
#define FLAG_MASK_N (0x40) // add/sub flag set when previous op was an add or sub
#define FLAG_MASK_H (0x20) // indicates carry for lower 4 bits of the result
#define FLAG_MASK_C (0x10) // indicate carry for upper 8 bits

#define MEMORY_SIZE (256*256) // 64k memory

typedef struct {
    union {
        struct {
            char A;
            char F;
        };

        short AF;
    };
    union {
        struct {
            char B;
            char C;
        };

        short BC;
    };
    union {
        struct {
            char D;
            char E;
        };

        short DE;
    };
    union {
        struct {
            char H;
            char L;
        };

        short HL;
    };

    short SP;
    short PC;
} Registers;

Registers registers;
char memory[MEMORY_SIZE];

void do_instruction(char instruction)
{
    switch(instruction)
    {
        case 0x47: // LD B,A
            registers.B = registers.A; 
            break;
            // TODO: set clock values
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
    printf("A: 0x%x, B: 0x%x\n", registers.A & 0xff, registers.B & 0xff);
    do_instruction(0x47);
    printf("A: 0x%x, B: 0x%x\n", registers.A & 0xff, registers.B & 0xff);
    return 0;
}

