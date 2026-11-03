#include <test_tape.h>

static int check_tape(int grow) {
    bf_tape* tape = grow ? TAPE_grow_new(2, 8) : TAPE_fixed_new(2);
    bf_tape* child = NULL;
    int ok = 1;

    TEST_CHECK(tape && TAPE_get(tape) == 0 && TAPE_position(tape) == 0);
    TAPE_add(tape, -1);
    TEST_CHECK(TAPE_get(tape) == 255);
    TAPE_add(tape, 2);
    TEST_CHECK(TAPE_get(tape) == 1);
    TAPE_add(tape, INT_MAX);
    TEST_CHECK(TAPE_get(tape) == 0);
    TAPE_set(tape, 17);
    TEST_CHECK(TAPE_move(tape, -1) == bf_err_tape_bounds);
    TEST_CHECK(TAPE_position(tape) == 0 && TAPE_get(tape) == 17);
    TEST_CHECK(TAPE_move(tape, 1) == bf_ok && TAPE_get(tape) == 0);
    TEST_CHECK(TAPE_move(tape, 6) == (grow ? bf_ok : bf_err_tape_bounds));
    TEST_CHECK(TAPE_capacity(tape) == (grow ? 8u : 2u));
    TEST_CHECK(TAPE_position(tape) == (grow ? 7u : 1u));
    TEST_CHECK(TAPE_move(tape, 1) == bf_err_tape_bounds);
    child = TAPE_spawn(tape);
    TEST_CHECK(child && TAPE_position(child) == 0 && TAPE_get(child) == 0);
    TEST_CHECK(TAPE_capacity(child) == TAPE_capacity(tape));
    TEST_CHECK(TAPE_move(tape, grow ? -7 : -1) == bf_ok);
    TEST_CHECK(TAPE_get(tape) == 17);

cleanup:
    OBJ_destroy(bf_as_object(child));
    OBJ_destroy(bf_as_object(tape));

    return ok;
}

static inline int fixed() { return check_tape(0); }
static inline int grow() { return check_tape(1); }
static int invalid_sizes() {
    int ok = 1;

    TEST_CHECK(!TAPE_fixed_new(0));
    TEST_CHECK(!TAPE_grow_new(0, 8));
    TEST_CHECK(!TAPE_grow_new(8, 2));

cleanup:
    return ok;
}

int main() {
    const test_case cases[] = { TEST_CASE(fixed), TEST_CASE(grow), TEST_CASE(invalid_sizes) };

    return TEST_run(cases, sizeof(cases) / sizeof(cases[0]));
}
