#include <test_program.h>

static int instruction_storage() {
    bf_program* program = PROG_new();
    unsigned long i;
    int ok = 1;

    TEST_CHECK(program && PROG_length(program) == 0);

    for (i = 0; i < 513; ++i)
        TEST_CHECK(PROG_emit(program, bf_op_add, (int)i - 256) == bf_ok);
    TEST_CHECK(PROG_length(program) == 513);

    for (i = 0; i < 513; ++i) {
        const bf_instruction* ins = &PROG_code(program)[i];

        TEST_CHECK(ins->opcode == bf_op_add && ins->operand == (int)i - 256);
        TEST_CHECK(!ins->reserved[0] && !ins->reserved[1] && !ins->reserved[2]);
    }

    TEST_CHECK(PROG_length(NULL) == 0 && PROG_code(NULL) == NULL);

cleanup:
    PROG_free(program);

    return ok;
}

int main() {
    const test_case cases[] = { TEST_CASE(instruction_storage) };

    return TEST_run(cases, sizeof(cases) / sizeof(cases[0]));
}
