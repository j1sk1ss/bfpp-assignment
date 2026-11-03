#include <io_internal.h>

typedef struct {
    bf_io super;
    FILE* input;
    FILE* output;
    int   close_on_destroy;
} bf_stdio_io;

const bf_type_info bf_type_stdio_io = { "bf_stdio_io", &bf_type_io };

static inline int stdio_read(bf_io* base) {
    bf_stdio_io* self = (bf_stdio_io*)base;
    int ch = fgetc(self->input);

    return ch == EOF ? -1 : ch;
}

static inline bf_status stdio_write(bf_io* base, unsigned char byte) {
    bf_stdio_io* self = (bf_stdio_io*)base;

    return fputc((int)byte, self->output) == EOF ? bf_err_io : bf_ok;
}

static inline bf_status stdio_flush(bf_io* base) {
    bf_stdio_io* self = (bf_stdio_io*)base;

    return fflush(self->output) == 0 ? bf_ok : bf_err_io;
}

static void stdio_destroy(bf_object* object) {
    bf_stdio_io* self = (bf_stdio_io*)object;

    if (self->close_on_destroy) {
        if (self->input) fclose(self->input);

        if (self->output && self->output != self->input) fclose(self->output);
    }

    free(self);
}

static const bf_io_vtable_type stdio_vtable = {
    { &bf_type_stdio_io, stdio_destroy },
    stdio_read,
    stdio_write,
    stdio_flush
};

bf_io* IO_stdio_new(FILE* input, FILE* output, int close_on_destroy) {
    bf_stdio_io* self;

    if (!input || !output) return NULL;

    self = (bf_stdio_io*)calloc(1, sizeof(*self));

    if (!self) return NULL;

    OBJ_init(&self->super.super, &stdio_vtable.super);
    self->input            = input;
    self->output           = output;
    self->close_on_destroy = close_on_destroy;

    return &self->super;
}
