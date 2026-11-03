#ifndef BF_TAPE_H_
#define BF_TAPE_H_

#include <common.h>
#include <object.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct bf_tape bf_tape;

extern const bf_type_info bf_type_tape;
extern const bf_type_info bf_type_fixed_tape;
extern const bf_type_info bf_type_grow_tape;

bf_tape*      TAPE_fixed_new(unsigned long cells);
bf_tape*      TAPE_grow_new(unsigned long initial_cells, unsigned long max_cells);
unsigned char TAPE_get(const bf_tape* self);
void          TAPE_set(bf_tape* self, unsigned char value);
void          TAPE_add(bf_tape* self, int delta);
bf_status     TAPE_move(bf_tape* self, int delta);
unsigned long TAPE_position(const bf_tape* self);
unsigned long TAPE_capacity(const bf_tape* self);
bf_tape*      TAPE_spawn(const bf_tape* prototype);

#ifdef __cplusplus
}
#endif

#endif
