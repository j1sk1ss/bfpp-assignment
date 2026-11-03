#include <test_vm.h>

/* Supply bytecode directly: these tests do not depend on the compiler. */
static int bytecode_fixtures() {
    FILE* cases = TEST_open_fixture("vm/cases.txt");
    FILE* code = NULL;
    bf_program* program = NULL;
    bf_tape* tape = NULL;
    bf_io* io = NULL;
    bf_vm* vm = NULL;
    unsigned char* input = NULL;
    unsigned char* expected = NULL;
    char code_path[128], input_path[128], output_path[128];
    unsigned long max_depth, max_steps, steps, offset, input_len, expected_len, actual_len;
    int status, ok = 1;

    TEST_CHECK(cases);

    while (
        fscanf(
            cases, "%127s %127s %127s %lu %lu %d %lu %lu", code_path, input_path,
            output_path, &max_depth, &max_steps, &status,
            &offset, &steps
        ) == 8
    ) {
        unsigned int opcode;
        int operand;
        bf_error error;
        bf_io* view;

        input = TEST_read_fixture(input_path, &input_len);
        expected = TEST_read_fixture(output_path, &expected_len);
        code = TEST_open_fixture(code_path);
        program = PROG_new();
        tape = TAPE_fixed_new(8);
        io = IO_buffer_new(input, input_len);
        TEST_CHECK(input && expected && code && program && tape && io);

        while (fscanf(code, "%u %d", &opcode, &operand) == 2) {
            TEST_CHECK(opcode <= 255);
            TEST_CHECK(PROG_emit(program, (unsigned char)opcode, operand) == bf_ok);
        }

        TEST_CHECK(feof(code));
        fclose(code); code = NULL;
        view = io;
        vm = VM_switch_new(program, tape, io, (bf_vm_config){max_depth, max_steps});
        TEST_CHECK(vm);
        tape = NULL; io = NULL;
        TEST_CHECK(VM_steps(vm) == 0);
        TEST_CHECK(VM_run(vm, &error) == (bf_status)status);
        TEST_CHECK(error.code == (bf_status)status && error.source_offset == offset);
        TEST_CHECK(VM_steps(vm) == steps);
        (void)IO_buffer_output(view, &actual_len);
        TEST_CHECK(actual_len == expected_len);
        TEST_CHECK(!actual_len || memcmp(IO_buffer_output(view, NULL), expected, actual_len) == 0);
        OBJ_destroy(bf_as_object(vm)); vm = NULL;
        PROG_free(program); program = NULL;
        free(input); input = NULL;
        free(expected); expected = NULL;
    }

    TEST_CHECK(feof(cases));

cleanup:
    if (cases) fclose(cases);

    if (code) fclose(code);

    OBJ_destroy(bf_as_object(vm));
    OBJ_destroy(bf_as_object(tape));
    OBJ_destroy(bf_as_object(io));
    PROG_free(program);
    free(input);
    free(expected);

    return ok;
}

int main() {
    const test_case cases[] = { TEST_CASE(bytecode_fixtures) };

    return TEST_run(cases, sizeof(cases) / sizeof(cases[0]));
}
