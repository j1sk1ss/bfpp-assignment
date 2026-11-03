#include <io_internal.h>

typedef struct {
    bf_io                super;
    const unsigned char* input;
    unsigned long        input_len;
    unsigned long        input_pos;
    unsigned char*       output;
    unsigned long        output_len;
    unsigned long        output_cap;
} bf_buffer_io;

const bf_type_info bf_type_buffer_io = { "bf_buffer_io", &bf_type_io };

static inline int buffer_read(bf_io* base) {
    bf_buffer_io* self = (bf_buffer_io*)base;

    if (self->input_pos >= self->input_len) return -1;

    return self->input[self->input_pos++];
}

static bf_status buffer_write(bf_io* base, unsigned char byte) {
    bf_buffer_io* self = (bf_buffer_io*)base;

    if (self->output_len == self->output_cap) {
        unsigned long next_cap = self->output_cap ? self->output_cap * 2u : 64u;
        unsigned char* next = (unsigned char*)realloc(self->output, next_cap);

        if (!next) return bf_err_oom;

        self->output = next;
        self->output_cap = next_cap;
    }

    self->output[self->output_len++] = byte;

    return bf_ok;
}

static inline bf_status buffer_flush(bf_io* base) {
    (void)base;

    return bf_ok;
}

static inline void buffer_destroy(bf_object* object) {
    bf_buffer_io* self = (bf_buffer_io*)object;

    free(self->output);
    free(self);
}

static const bf_io_vtable_type buffer_vtable = {
    { &bf_type_buffer_io, buffer_destroy },
    buffer_read,
    buffer_write,
    buffer_flush
};

bf_io* IO_buffer_new(const unsigned char* input, unsigned long input_len) {
    bf_buffer_io* self = (bf_buffer_io*)calloc(1, sizeof(*self));

    if (!self) return NULL;

    OBJ_init(&self->super.super, &buffer_vtable.super);
    self->input = input;
    self->input_len = input_len;

    return &self->super;
}

const unsigned char* IO_buffer_output(const bf_io* base, unsigned long* len) {
    const bf_buffer_io* self;

    if (!base || !OBJ_is_a(bf_as_const_object(base), &bf_type_buffer_io)) return NULL;

    self = (const bf_buffer_io*)base;

    if (len) *len = self->output_len;

    return self->output;
}

void IO_buffer_reset(bf_io* base, const unsigned char* input, unsigned long input_len) {
    bf_buffer_io* self;

    if (!base || !OBJ_is_a(bf_as_object(base), &bf_type_buffer_io)) return;

    self = (bf_buffer_io*)base;
    self->input      = input;
    self->input_len  = input_len;
    self->input_pos  = 0;
    self->output_len = 0;
}
