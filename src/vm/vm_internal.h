#ifndef BF_VM_INTERNAL_H_
#define BF_VM_INTERNAL_H_

#include <vm.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    bf_object_vtable   super;
    bf_status          (*run)(bf_vm*, bf_error*);
    unsigned long long (*steps)(const bf_vm*);
} bf_vm_vtable_type;

struct bf_vm {
    bf_object          super;
};

static inline const bf_vm_vtable_type* bf_vm_vtable(const bf_vm* self) {
    return (const bf_vm_vtable_type*)self->super.vptr;
}

#endif
