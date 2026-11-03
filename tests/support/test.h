#ifndef BF_TEST_H_
#define BF_TEST_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef TEST_FIXTURE_DIR
#define TEST_FIXTURE_DIR "tests/fixtures"
#endif

typedef struct {
    const char* name;
    int         (*run)();
} test_case;

#define TEST_CASE(fn) { #fn, fn }
#define TEST_CHECK(expr) do { \
    if (!(expr)) { \
        fprintf( \
            stderr, "%s:%d: %s\n", __FILE__, __LINE__, \
            #expr \
        ); \
        ok = 0; \
        goto cleanup; \
    } \
} while (0)

int            TEST_run(const test_case* cases, unsigned long count);
unsigned char* TEST_read_fixture(const char* name, unsigned long* length);
FILE*          TEST_open_fixture(const char* name);

#endif
