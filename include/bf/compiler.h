#ifndef BF_COMPILER_H_
#define BF_COMPILER_H_

#include <common.h>
#include <object.h>
#include <program.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct bf_compiler bf_compiler;

extern const bf_type_info bf_type_compiler;
extern const bf_type_info bf_type_classic_compiler;
extern const bf_type_info bf_type_bfpp_compiler;

bf_compiler* COMP_classic_new();
bf_compiler* COMP_bfpp_new();

bf_status COMP_compile(
    bf_compiler* self, const char* source, unsigned long source_len, bf_program** out_program,
    bf_error* error
);

#ifdef __cplusplus
}
#endif

#endif
