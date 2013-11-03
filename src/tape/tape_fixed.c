#include <tape_internal.h>

typedef struct {
    bf_tape        super;
    unsigned char* cells;
    unsigned long  count;
    unsigned long  position;
} bf_fixed_tape;

const bf_type_info bf_type_fixed_tape = { "bf_fixed_tape", &bf_type_tape };

static inline unsigned char fixed_get(const bf_tape* base) {
    const bf_fixed_tape* self = (const bf_fixed_tape*)base;

    return self->cells[self->position];
}

static inline void fixed_set(bf_tape* base, unsigned char value) {
    bf_fixed_tape* self = (bf_fixed_tape*)base;

    self->cells[self->position] = value;
}

static inline void fixed_add(bf_tape* base, int delta) {
    bf_fixed_tape* self = (bf_fixed_tape*)base;

    self->cells[self->position] = (unsigned char)((unsigned int)self->cells[self->position] + (unsigned int)delta);
}

static inline bf_status fixed_move(bf_tape* base, int delta) {
    bf_fixed_tape* self = (bf_fixed_tape*)base;
    long long target = (long long)self->position + (long long)delta;

    if (target < 0 || (unsigned long long)target >= (unsigned long long)self->count) return bf_err_tape_bounds;

    self->position = (unsigned long)target;

    return bf_ok;
}

static inline unsigned long fixed_position(const bf_tape* base) {
    return ((const bf_fixed_tape*)base)->position;
}

static inline unsigned long fixed_capacity(const bf_tape* base) {
    return ((const bf_fixed_tape*)base)->count;
}

static inline bf_tape* fixed_spawn(const bf_tape* base) {
    return TAPE_fixed_new(((const bf_fixed_tape*)base)->count);
}

static inline void fixed_destroy(bf_object* object) {
    bf_fixed_tape* self = (bf_fixed_tape*)object;

    free(self->cells);
    free(self);
}

static const bf_tape_vtable_type fixed_vtable = {
    { &bf_type_fixed_tape, fixed_destroy },
    fixed_get,
    fixed_set,
    fixed_add,
    fixed_move,
    fixed_position,
    fixed_capacity,
    fixed_spawn
};

bf_tape* TAPE_fixed_new(unsigned long cells) {
    bf_fixed_tape* self;

    if (cells == 0) return NULL;

    self = (bf_fixed_tape*)calloc(1, sizeof(*self));

    if (!self) return NULL;

    self->cells = (unsigned char*)calloc(cells, 1);

    if (!self->cells) {
        free(self);

        return NULL;
    }

    OBJ_init(&self->super.super, &fixed_vtable.super);
    self->count = cells;

    return &self->super;
}
