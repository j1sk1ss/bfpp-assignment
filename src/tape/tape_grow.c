#include <tape_internal.h>

typedef struct {
    bf_tape        super;
    unsigned char* cells;
    unsigned long  count;
    unsigned long  position;
    unsigned long  max_cells;
} bf_grow_tape;

const bf_type_info bf_type_grow_tape = { "bf_grow_tape", &bf_type_tape };

static inline unsigned char grow_get(const bf_tape* base) {
    const bf_grow_tape* self = (const bf_grow_tape*)base;

    return self->cells[self->position];
}

static inline void grow_set(bf_tape* base, unsigned char value) {
    bf_grow_tape* self = (bf_grow_tape*)base;

    self->cells[self->position] = value;
}

static inline void grow_add(bf_tape* base, int delta) {
    bf_grow_tape* self = (bf_grow_tape*)base;

    self->cells[self->position] = (unsigned char)((unsigned int)self->cells[self->position] + (unsigned int)delta);
}

static bf_status grow_reserve(bf_grow_tape* self, unsigned long needed) {
    unsigned long next_count;
    unsigned char* next;

    if (needed <= self->count) return bf_ok;

    if (needed > self->max_cells) return bf_err_tape_bounds;

    next_count = self->count;

    while (next_count < needed) {
        unsigned long doubled = next_count > self->max_cells / 2u ? self->max_cells : next_count * 2u;

        if (doubled <= next_count) return bf_err_tape_bounds;

        next_count = doubled;
    }

    next = (unsigned char*)realloc(self->cells, next_count);

    if (!next) return bf_err_oom;

    memset(next + self->count, 0, next_count - self->count);
    self->cells = next;
    self->count = next_count;

    return bf_ok;
}

static bf_status grow_move(bf_tape* base, int delta) {
    bf_grow_tape* self = (bf_grow_tape*)base;
    long long target = (long long)self->position + (long long)delta;
    bf_status status;

    if (target < 0) return bf_err_tape_bounds;

    status = grow_reserve(self, (unsigned long)target + 1u);

    if (status != bf_ok) return status;

    self->position = (unsigned long)target;

    return bf_ok;
}

static inline unsigned long grow_position(const bf_tape* base) {
    return ((const bf_grow_tape*)base)->position;
}

static inline unsigned long grow_capacity(const bf_tape* base) {
    return ((const bf_grow_tape*)base)->count;
}

static inline bf_tape* grow_spawn(const bf_tape* base) {
    const bf_grow_tape* self = (const bf_grow_tape*)base;

    return TAPE_grow_new(self->count, self->max_cells);
}

static inline void grow_destroy(bf_object* object) {
    bf_grow_tape* self = (bf_grow_tape*)object;

    free(self->cells);
    free(self);
}

static const bf_tape_vtable_type grow_vtable = {
    { &bf_type_grow_tape, grow_destroy },
    grow_get,
    grow_set,
    grow_add,
    grow_move,
    grow_position,
    grow_capacity,
    grow_spawn
};

bf_tape* TAPE_grow_new(unsigned long initial_cells, unsigned long max_cells) {
    bf_grow_tape* self;

    if (initial_cells == 0 || max_cells < initial_cells) return NULL;

    self = (bf_grow_tape*)calloc(1, sizeof(*self));

    if (!self) return NULL;

    self->cells = (unsigned char*)calloc(initial_cells, 1);

    if (!self->cells) {
        free(self);

        return NULL;
    }

    OBJ_init(&self->super.super, &grow_vtable.super);
    self->count = initial_cells;
    self->max_cells = max_cells;

    return &self->super;
}
