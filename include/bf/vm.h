#ifndef BF_VM_H_
#define BF_VM_H_

#include <common.h>
#include <io.h>
#include <object.h>
#include <program.h>
#include <tape.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct bf_vm bf_vm;

typedef struct {
    unsigned long      max_call_depth;
    unsigned long long max_steps; /* 0 = unlimited */
} bf_vm_config;

extern const bf_type_info bf_type_vm;
extern const bf_type_info bf_type_switch_vm;

bf_vm*             VM_switch_new(const bf_program* program, bf_tape* tape_prototype, bf_io* io, bf_vm_config config);
bf_status          VM_run(bf_vm* self, bf_error* error);
unsigned long long VM_steps(const bf_vm* self);

#ifdef __cplusplus
}
#endif

#endif
