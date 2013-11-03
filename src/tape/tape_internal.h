#ifndef BF_TAPE_INTERNAL_H_
#define BF_TAPE_INTERNAL_H_

#include <tape.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    bf_object_vtable super;
    unsigned char    (*get)(const bf_tape*);
    void             (*set)(bf_tape*, unsigned char);
    void             (*add)(bf_tape*, int);
    bf_status        (*move)(bf_tape*, int);
    unsigned long    (*position)(const bf_tape*);
    unsigned long    (*capacity)(const bf_tape*);
    bf_tape*         (*spawn)(const bf_tape*);
} bf_tape_vtable_type;

struct bf_tape {
    bf_object        super;
};

static inline const bf_tape_vtable_type* bf_tape_vtable(const bf_tape* self) {
    return (const bf_tape_vtable_type*)self->super.vptr;
}

#endif
