#include <test.h>

int TEST_run(const test_case* cases, unsigned long count) {
    unsigned long i;
    int failures = 0;

    for (i = 0; i < count; ++i) {
        int ok = cases[i].run();

        printf("[%s] %s\n", ok ? " OK " : "FAIL", cases[i].name);
        failures += !ok;
    }

    return failures ? 1 : 0;
}

FILE* TEST_open_fixture(const char* name) {
    char path[1024];
    FILE* file;

    int length = snprintf(
        path, sizeof(path), "%s/%s", TEST_FIXTURE_DIR,
        name
    );

    if (length < 0 || (unsigned long)length >= sizeof(path)) return NULL;

    file = fopen(path, "rb");

    if (!file) fprintf(stderr, "cannot open fixture %s\n", path);

    return file;
}

unsigned char* TEST_read_fixture(const char* name, unsigned long* length) {
    FILE* file = TEST_open_fixture(name);
    unsigned char* data = NULL;
    long size;

    if (!file) return NULL;

    if (fseek(file, 0, SEEK_END) != 0 || (size = ftell(file)) < 0) goto cleanup;

    rewind(file);
    data = malloc((unsigned long)size + 1u);

    if (!data) goto cleanup;

    if (fread(data, 1, (unsigned long)size, file) != (unsigned long)size) {
        free(data);
        data = NULL;
        goto cleanup;
    }

    data[size] = 0;
    *length = (unsigned long)size;

cleanup:
    fclose(file);

    return data;
}
