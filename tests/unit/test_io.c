#include <test_io.h>

static int buffer() {
    unsigned long length = 0, output_length = 0, i;
    unsigned char* input = TEST_read_fixture("io/bytes.bin", &length);
    bf_io* io = IO_buffer_new(input, length);
    int ok = 1;

    TEST_CHECK(input && io && length > 64);

    for (i = 0; i < length; ++i) {
        TEST_CHECK(IO_read_byte(io) == input[i]);
        TEST_CHECK(IO_write_byte(io, input[i]) == bf_ok);
    }

    TEST_CHECK(IO_read_byte(io) == -1 && IO_flush(io) == bf_ok);
    TEST_CHECK(IO_buffer_output(io, &output_length));
    TEST_CHECK(output_length == length);
    TEST_CHECK(memcmp(IO_buffer_output(io, NULL), input, length) == 0);
    IO_buffer_reset(io, input, length);
    (void)IO_buffer_output(io, &output_length);
    TEST_CHECK(output_length == 0 && IO_read_byte(io) == input[0]);

cleanup:
    OBJ_destroy(bf_as_object(io));
    free(input);

    return ok;
}

static int stdio_roundtrip() {
    FILE* input = TEST_open_fixture("io/bytes.bin");
    FILE* output = tmpfile();
    bf_io* io = NULL;
    unsigned long length = 0, i;
    unsigned char* bytes = TEST_read_fixture("io/bytes.bin", &length);
    int ok = 1;

    TEST_CHECK(input && output && bytes);
    io = IO_stdio_new(input, output, 0);
    TEST_CHECK(io);

    for (i = 0; i < length; ++i) {
        TEST_CHECK(IO_read_byte(io) == bytes[i]);
        TEST_CHECK(IO_write_byte(io, bytes[i]) == bf_ok);
    }

    TEST_CHECK(IO_read_byte(io) == -1 && IO_flush(io) == bf_ok);
    rewind(output);

    for (i = 0; i < length; ++i) TEST_CHECK(fgetc(output) == bytes[i]);

    TEST_CHECK(fgetc(output) == EOF);
    /* Destroying with close_on_destroy=0 leaves streams usable. */
    OBJ_destroy(bf_as_object(io)); io = NULL;
    TEST_CHECK(fseek(input, 0, SEEK_SET) == 0 && fgetc(input) == bytes[0]);

cleanup:
    OBJ_destroy(bf_as_object(io));

    if (input) fclose(input);

    if (output) fclose(output);

    free(bytes);

    return ok;
}

static int invalid_io() {
    int ok = 1;

    TEST_CHECK(IO_read_byte(NULL) == -1);
    TEST_CHECK(IO_write_byte(NULL, 0) == bf_err_bad_argument);
    TEST_CHECK(IO_flush(NULL) == bf_err_bad_argument);
    TEST_CHECK(!IO_stdio_new(NULL, stdout, 0));
    TEST_CHECK(!IO_buffer_output(NULL, NULL));

cleanup:
    return ok;
}

int main() {
    const test_case cases[] = { TEST_CASE(buffer), TEST_CASE(stdio_roundtrip), TEST_CASE(invalid_io) };

    return TEST_run(cases, sizeof(cases) / sizeof(cases[0]));
}
