#ifndef BF_IO_INTERNAL_H_
#define BF_IO_INTERNAL_H_

#include <io.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    bf_object_vtable super;
    int              (*read_byte)(bf_io*);
    bf_status        (*write_byte)(bf_io*, unsigned char);
    bf_status        (*flush)(bf_io*);
} bf_io_vtable_type;

struct bf_io {
    bf_object        super;
};

static inline const bf_io_vtable_type* bf_io_vtable(const bf_io* self) {
    return (const bf_io_vtable_type*)self->super.vptr;
}

#endif
