#include <program_internal.h>

bf_program* PROG_new() {
    return (bf_program*)calloc(1, sizeof(bf_program));
}

void PROG_free(bf_program* program) {
    if (!program) return;

    free(program->code);
    free(program);
}

unsigned long PROG_length(const bf_program* program) {
    return program ? program->len : 0u;
}

const bf_instruction* PROG_code(const bf_program* program) {
    return program ? program->code : NULL;
}

bf_status PROG_emit(bf_program* program, unsigned char opcode, int operand) {
    bf_instruction* next;

    if (program->len == program->cap) {
        unsigned long next_cap = program->cap ? program->cap * 2u : 128u;

        next = (bf_instruction*)realloc(program->code, next_cap * sizeof(*next));

        if (!next) return bf_err_oom;

        program->code = next;
        program->cap = next_cap;
    }

    program->code[program->len].opcode      = opcode;
    program->code[program->len].reserved[0] = 0;
    program->code[program->len].reserved[1] = 0;
    program->code[program->len].reserved[2] = 0;
    program->code[program->len].operand     = operand;
    ++program->len;

    return bf_ok;
}
