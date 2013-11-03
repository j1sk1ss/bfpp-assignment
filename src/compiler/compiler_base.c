#include <compiler_internal.h>

const bf_type_info bf_type_compiler = { "bf_compiler", &bf_type_object };

bf_status COMP_compile(
    bf_compiler* self, const char* source, unsigned long source_len, bf_program** out_program,
    bf_error* error
) {
    if (!self || !source || !out_program) return bf_err_bad_argument;

    return bf_compiler_vtable(self)->compile(
        self, source, source_len, out_program,
        error
    );
}
