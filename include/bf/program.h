#ifndef BF_PROGRAM_H_
#define BF_PROGRAM_H_

#include <common.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum bf_op_code {
    bf_op_add = 0,
    bf_op_move,
    bf_op_input,
    bf_op_output,
    bf_op_jump_if_zero,
    bf_op_jump_if_nonzero,
    bf_op_zero,
    bf_op_jump,
    bf_op_call,
    bf_op_return_or_halt,
    bf_op_load_arg,
    bf_op_store_arg,
    bf_op_load_return
} bf_op_code;

typedef struct {
    unsigned char opcode;
    unsigned char reserved[3];
    int           operand;
} bf_instruction;

typedef struct bf_program bf_program;

bf_program*           PROG_new();
void                  PROG_free(bf_program* program);
unsigned long         PROG_length(const bf_program* program);
const bf_instruction* PROG_code(const bf_program* program);

#ifdef __cplusplus
}
#endif

#endif
