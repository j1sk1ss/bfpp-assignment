#include <compiler_internal.h>

typedef struct {
    bf_compiler super;
} bf_classic_compiler;

const bf_type_info bf_type_classic_compiler = { "bf_classic_compiler", &bf_type_compiler };

static inline bf_status classic_compile(
    bf_compiler* self, const char* source, unsigned long source_len, bf_program** out_program,
    bf_error* error
) {
    const bf_compiler_options options = { 0 };

    (void)self;

    return COMP_compile_core(
        source, source_len, &options, out_program,
        error
    );
}

static inline void classic_destroy(bf_object* object) {
    free(object);
}

static const bf_compiler_vtable_type classic_vtable = {
    { &bf_type_classic_compiler, classic_destroy },
    classic_compile
};

bf_compiler* COMP_classic_new() {
    bf_classic_compiler* self = (bf_classic_compiler*)calloc(1, sizeof(*self));

    if (!self) return NULL;

    OBJ_init(&self->super.super, &classic_vtable.super);

    return &self->super;
}
