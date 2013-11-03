#ifndef BF_IO_H_
#define BF_IO_H_

#include <common.h>
#include <object.h>

#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct bf_io bf_io;

extern const bf_type_info bf_type_io;
extern const bf_type_info bf_type_stdio_io;
extern const bf_type_info bf_type_buffer_io;

bf_io*               IO_stdio_new(FILE* input, FILE* output, int close_on_destroy);
bf_io*               IO_buffer_new(const unsigned char* input, unsigned long input_len);
int                  IO_read_byte(bf_io* self);
bf_status            IO_write_byte(bf_io* self, unsigned char byte);
bf_status            IO_flush(bf_io* self);
const unsigned char* IO_buffer_output(const bf_io* self, unsigned long* len);
void                 IO_buffer_reset(bf_io* self, const unsigned char* input, unsigned long input_len);

#ifdef __cplusplus
}
#endif

#endif
