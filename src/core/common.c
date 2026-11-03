#include <common_internal.h>

const char* COM_status_name(bf_status status) {
    switch (status) {
        case bf_ok:               return "bf_ok";
        case bf_err_oom:          return "bf_err_oom";
        case bf_err_io:           return "bf_err_io";
        case bf_err_syntax:       return "bf_err_syntax";
        case bf_err_tape_bounds:  return "bf_err_tape_bounds";
        case bf_err_call_depth:   return "bf_err_call_depth";
        case bf_err_bad_argument: return "bf_err_bad_argument";
        case bf_err_internal:     return "bf_err_internal";
        default:                  return "bf_err_unknown";
    }
}

void COM_error_clear(bf_error* error) {
    if (!error) return;

    memset(error, 0, sizeof(*error));
    error->code = bf_ok;
}
