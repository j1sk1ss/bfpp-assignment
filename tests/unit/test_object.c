#include <test_object.h>

static int destroyed;

static inline void destroy(bf_object* object) { (void)object; ++destroyed; }

static int inheritance() {
    const bf_type_info parent = { "parent", &bf_type_object };
    const bf_type_info child = { "child", &parent };
    const bf_type_info unrelated = { "unrelated", &bf_type_object };
    const bf_object_vtable vtable = { &child, destroy };
    bf_object object;
    int ok = 1;

    destroyed = 0;
    OBJ_init(&object, &vtable);
    TEST_CHECK(OBJ_is_a(&object, &child));
    TEST_CHECK(OBJ_is_a(&object, &parent));
    TEST_CHECK(OBJ_is_a(&object, &bf_type_object));
    TEST_CHECK(!OBJ_is_a(&object, &unrelated));
    TEST_CHECK(!OBJ_is_a(NULL, &child) && !OBJ_is_a(&object, NULL));
    TEST_CHECK(strcmp(OBJ_type_name(&object), child.name) == 0);
    OBJ_destroy(&object);
    TEST_CHECK(destroyed == 1);
    OBJ_destroy(NULL);

cleanup:
    return ok;
}

static int clear_error() {
    bf_error error;
    int ok = 1;

    memset(&error, 0xff, sizeof(error));
    COM_error_clear(&error);
    TEST_CHECK(error.code == bf_ok && error.source_offset == 0 && !error.message[0]);
    COM_error_clear(NULL);

cleanup:
    return ok;
}

int main() {
    const test_case cases[] = { TEST_CASE(inheritance), TEST_CASE(clear_error) };

    return TEST_run(cases, sizeof(cases) / sizeof(cases[0]));
}
