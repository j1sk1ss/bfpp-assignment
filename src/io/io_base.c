#include <io_internal.h>

const bf_type_info bf_type_io = { "bf_io", &bf_type_object };

int IO_read_byte(bf_io* self) {
    if (!self) return -1;

    return bf_io_vtable(self)->read_byte(self);
}

bf_status IO_write_byte(bf_io* self, unsigned char byte) {
    if (!self) return bf_err_bad_argument;

    return bf_io_vtable(self)->write_byte(self, byte);
}

bf_status IO_flush(bf_io* self) {
    if (!self) return bf_err_bad_argument;

    return bf_io_vtable(self)->flush(self);
}
