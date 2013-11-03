#ifndef BF_COMPILER_INTERNAL_H_
#define BF_COMPILER_INTERNAL_H_

#include <compiler.h>
#include <program_internal.h>
#include <limits.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    bf_object_vtable super;
    bf_status        (*compile)(
        bf_compiler*, const char*, unsigned long, bf_program**,
        bf_error*
    );
} bf_compiler_vtable_type;

struct bf_compiler {
    bf_object        super;
};

typedef struct {
    int              enable_extensions;
} bf_compiler_options;

bf_status COMP_compile_core(
    const char* source, unsigned long source_len, const bf_compiler_options* options, bf_program** out_program,
    bf_error* error
);

static inline const bf_compiler_vtable_type* bf_compiler_vtable(const bf_compiler* self) {
    return (const bf_compiler_vtable_type*)self->super.vptr;
}

#endif
