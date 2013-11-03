#include <test_compiler.h>

static int compile_fixtures() {
    FILE* cases = TEST_open_fixture("compiler/cases.txt");
    FILE* expected = NULL;
    bf_compiler* compiler = NULL;
    bf_program* program = NULL;
    unsigned char* source = NULL;
    char source_path[128], expected_path[128];
    int dialect, status;
    unsigned long offset, length;
    int ok = 1;

    TEST_CHECK(cases);

    while (
        fscanf(
            cases, "%127s %127s %d %d %lu", source_path, expected_path,
            &dialect, &status, &offset
        ) == 5
    ) {
        bf_error error;
        unsigned long i = 0;
        unsigned int opcode;
        int operand;

        compiler = dialect ? COMP_bfpp_new() : COMP_classic_new();
        source = TEST_read_fixture(source_path, &length);
        TEST_CHECK(compiler && source);
        TEST_CHECK(
            COMP_compile(
                compiler, (const char*)source, length, &program,
                &error
            ) == (bf_status)status
        );
        TEST_CHECK(error.code == (bf_status)status);

        if (status == bf_ok) {
            expected = TEST_open_fixture(expected_path);
            TEST_CHECK(expected && program);

            while (fscanf(expected, "%u %d", &opcode, &operand) == 2) {
                const bf_instruction* code = PROG_code(program);

                TEST_CHECK(i < PROG_length(program));
                TEST_CHECK(code[i].opcode == opcode && code[i].operand == operand);
                ++i;
            }

            TEST_CHECK(feof(expected) && i == PROG_length(program));
            fclose(expected);
            expected = NULL;
        } else {
            TEST_CHECK(!program && error.source_offset == offset && error.message[0]);
        }

        PROG_free(program); program = NULL;
        OBJ_destroy(bf_as_object(compiler)); compiler = NULL;
        free(source); source = NULL;
    }

    TEST_CHECK(feof(cases));

cleanup:
    if (cases) fclose(cases);

    if (expected) fclose(expected);

    PROG_free(program);
    OBJ_destroy(bf_as_object(compiler));
    free(source);

    return ok;
}

static int bad_arguments() {
    bf_program* program = NULL;
    bf_error error;
    int ok = 1;

    TEST_CHECK(
        COMP_compile(
            NULL, NULL, 0, &program,
            &error
        ) == bf_err_bad_argument
    );

cleanup:
    return ok;
}

int main() {
    const test_case cases[] = { TEST_CASE(compile_fixtures), TEST_CASE(bad_arguments) };

    return TEST_run(cases, sizeof(cases) / sizeof(cases[0]));
}
