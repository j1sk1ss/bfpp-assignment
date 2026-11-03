#ifndef BF_PROGRAM_INTERNAL_H_
#define BF_PROGRAM_INTERNAL_H_

#include <program.h>
#include <stdlib.h>

struct bf_program {
    bf_instruction* code;
    unsigned long   len;
    unsigned long   cap;
};

bf_status PROG_emit(bf_program* program, unsigned char opcode, int operand);

#endif
