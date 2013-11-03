#include <tape_internal.h>

const bf_type_info bf_type_tape = { "bf_tape", &bf_type_object };

unsigned char TAPE_get(const bf_tape* self) {
    return bf_tape_vtable(self)->get(self);
}

void TAPE_set(bf_tape* self, unsigned char value) {
    bf_tape_vtable(self)->set(self, value);
}

void TAPE_add(bf_tape* self, int delta) {
    bf_tape_vtable(self)->add(self, delta);
}

bf_status TAPE_move(bf_tape* self, int delta) {
    return bf_tape_vtable(self)->move(self, delta);
}

unsigned long TAPE_position(const bf_tape* self) {
    return bf_tape_vtable(self)->position(self);
}

unsigned long TAPE_capacity(const bf_tape* self) {
    return bf_tape_vtable(self)->capacity(self);
}

bf_tape* TAPE_spawn(const bf_tape* prototype) {
    return bf_tape_vtable(prototype)->spawn(prototype);
}
