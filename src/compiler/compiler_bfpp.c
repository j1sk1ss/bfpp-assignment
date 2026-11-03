#include <compiler_internal.h>

typedef struct {
    bf_compiler super;
} bf_plus_plus_compiler;

const bf_type_info bf_type_bfpp_compiler = { "bf_plus_plus_compiler", &bf_type_compiler };

static inline bf_status bfpp_compile(
    bf_compiler* self, const char* source, unsigned long source_len, bf_program** out_program,
    bf_error* error
) {
    const bf_compiler_options options = { 1 };

    (void)self;

    return COMP_compile_core(
        source, source_len, &options, out_program,
        error
    );
}

static inline void bfpp_destroy(bf_object* object) {
    free(object);
}

static const bf_compiler_vtable_type bfpp_vtable = {
    { &bf_type_bfpp_compiler, bfpp_destroy },
    bfpp_compile
};

bf_compiler* COMP_bfpp_new() {
    bf_plus_plus_compiler* self = (bf_plus_plus_compiler*)calloc(1, sizeof(*self));

    if (!self) return NULL;

    OBJ_init(&self->super.super, &bfpp_vtable.super);

    return &self->super;
}
