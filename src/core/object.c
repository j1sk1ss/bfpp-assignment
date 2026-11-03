#include <object_internal.h>

const bf_type_info bf_type_object = { "bf_object", NULL };

void OBJ_init(bf_object* self, const bf_object_vtable* vptr) {
    self->vptr = vptr;
}

void OBJ_destroy(bf_object* self) {
    if (self && self->vptr && self->vptr->destroy) {
        self->vptr->destroy(self);
    }
}

const char* OBJ_type_name(const bf_object* self) {
    if (!self || !self->vptr || !self->vptr->type) return "<invalid>";

    return self->vptr->type->name;
}

int OBJ_is_a(const bf_object* self, const bf_type_info* type) {
    const bf_type_info* it;

    if (!self || !self->vptr || !type) return 0;

    it = self->vptr->type;

    while (it) {
        if (it == type) return 1;

        it = it->parent;
    }

    return 0;
}
