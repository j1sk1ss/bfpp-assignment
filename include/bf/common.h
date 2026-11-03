#ifndef BF_COMMON_H_
#define BF_COMMON_H_

#ifdef __cplusplus
extern "C" {
#endif

typedef enum bf_status {
    bf_ok = 0,
    bf_err_oom,
    bf_err_io,
    bf_err_syntax,
    bf_err_tape_bounds,
    bf_err_call_depth,
    bf_err_bad_argument,
    bf_err_internal
} bf_status;

typedef struct {
    bf_status     code;
    unsigned long source_offset;
    char          message[160];
} bf_error;

const char* COM_status_name(bf_status status);
void        COM_error_clear(bf_error* error);

#ifdef __cplusplus
}
#endif

#endif
