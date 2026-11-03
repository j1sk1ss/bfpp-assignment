#include <compiler_internal.h>

#define bf_label_count 26

typedef struct {
    unsigned long* data;
    unsigned long  len;
    unsigned long  cap;
} int_stack;

typedef struct {
    unsigned long  instruction_index;
    int            label;
} patch;

typedef struct {
    patch*         data;
    unsigned long  len;
    unsigned long  cap;
} patch_vec;

static void set_error(
    bf_error* error, bf_status status, unsigned long offset, const char* fmt,
    ...
) {
    va_list args;

    if (!error) return;

    error->code = status;
    error->source_offset = offset;
    va_start(args, fmt);
    vsnprintf(error->message, sizeof(error->message), fmt, args);
    va_end(args);
}

static bf_status stack_push(int_stack* stack, unsigned long value) {
    unsigned long* next;

    if (stack->len == stack->cap) {
        unsigned long next_cap = stack->cap ? stack->cap * 2u : 64u;

        next = (unsigned long*)realloc(stack->data, next_cap * sizeof(*next));

        if (!next) return bf_err_oom;

        stack->data = next;
        stack->cap = next_cap;
    }

    stack->data[stack->len++] = value;

    return bf_ok;
}

static bf_status patch_push(patch_vec* patches, unsigned long index, int label) {
    patch* next;

    if (patches->len == patches->cap) {
        unsigned long next_cap = patches->cap ? patches->cap * 2u : 32u;

        next = (patch*)realloc(patches->data, next_cap * sizeof(*next));

        if (!next) return bf_err_oom;

        patches->data = next;
        patches->cap = next_cap;
    }

    patches->data[patches->len].instruction_index = index;
    patches->data[patches->len].label = label;
    ++patches->len;

    return bf_ok;
}

static inline int is_label(char c) {
    return c >= 'a' && c <= 'z';
}

static inline int is_digit(char c) {
    return c >= '0' && c <= '9';
}

static inline bf_status emit_checked(bf_program* program, unsigned char opcode, long long operand) {
    if (operand < INT_MIN || operand > INT_MAX) return bf_err_internal;

    return PROG_emit(program, opcode, (int)operand);
}

static bf_status emit_run(bf_program* program, unsigned char opcode, long long amount) {
    while (amount != 0) {
        long long chunk = amount;

        if (chunk > INT_MAX) chunk = INT_MAX;

        if (chunk < INT_MIN) chunk = INT_MIN;

        {
            bf_status status = emit_checked(program, opcode, chunk);

            if (status != bf_ok) return status;
        }

        amount -= chunk;
    }

    return bf_ok;
}

bf_status COMP_compile_core(
    const char* source, unsigned long source_len, const bf_compiler_options* options, bf_program** out_program,
    bf_error* error
) {
    bf_program* program = NULL;
    int_stack   loops = { 0 };
    patch_vec   patches = { 0 };
    int labels[bf_label_count];
    unsigned long i = 0;
    int in_comment = 0;
    bf_status   status = bf_ok;
    int k;

    if (!source || !options || !out_program) return bf_err_bad_argument;

    *out_program = NULL;
    COM_error_clear(error);

    for (k = 0; k < bf_label_count; ++k) labels[k] = -1;

    program = PROG_new();

    if (!program) return bf_err_oom;

    while (i < source_len) {
        char c = source[i];

        if (options->enable_extensions && c == '/') {
            in_comment = !in_comment;
            ++i;
            continue;
        }

        if (in_comment) {
            ++i;
            continue;
        }

        if (c == '+' || c == '-') {
            long long amount = 0;
            unsigned long j = i;

            while (j < source_len && (source[j] == '+' || source[j] == '-')) {
                amount += source[j] == '+' ? 1 : -1;
                ++j;
            }

            status = emit_run(program, bf_op_add, amount);

            if (status != bf_ok) goto _fail;

            i = j;
            continue;
        }

        if (c == '>' || c == '<') {
            long long amount = 0;
            unsigned long j = i;

            while (j < source_len && (source[j] == '>' || source[j] == '<')) {
                amount += source[j] == '>' ? 1 : -1;
                ++j;
            }

            status = emit_run(program, bf_op_move, amount);

            if (status != bf_ok) goto _fail;

            i = j;
            continue;
        }

        switch (c) {
            case ',': status = emit_checked(program, bf_op_input, 0);  break;
            case '.': status = emit_checked(program, bf_op_output, 0); break;
            case '[': {
                unsigned long index = program->len;

                status = emit_checked(program, bf_op_jump_if_zero, 0);

                if (status == bf_ok) status = stack_push(&loops, index);

                break;
            }
            case ']': {
                unsigned long open_index;

                if (loops.len == 0) {
                    set_error(
                        error, bf_err_syntax, i, "unmatched closing bracket at byte %lu",
                        (unsigned long)i
                    );
                    status = bf_err_syntax;
                    break;
                }

                open_index = loops.data[--loops.len];
                status = emit_checked(program, bf_op_jump_if_nonzero, (long long)open_index + 1);

                if (status == bf_ok) {
                    program->code[open_index].operand = (int)program->len;
                }

                break;
            }
            default:
                if (options->enable_extensions) {
                    if (c == '*')      status = emit_checked(program, bf_op_zero, 0);
                    else if (c == '!') status = emit_checked(program, bf_op_return_or_halt, 0);
                    else if (c == '$') status = emit_checked(program, bf_op_load_return, 0);
                    else if (c == '^' || c == '&') {
                        unsigned char opcode = c == '^' ? bf_op_jump : bf_op_call;

                        if (i + 1 >= source_len || !is_label(source[i + 1])) {
                            set_error(
                                error, bf_err_syntax, i, "'%c' must be followed by a label a-z",
                                c
                            );
                            status = bf_err_syntax;
                        }
                        else {
                            int label = source[i + 1] - 'a';
                            unsigned long index = program->len;

                            status = emit_checked(program, opcode, 0);

                            if (status == bf_ok) status = patch_push(&patches, index, label);

                            ++i;
                        }
                    }
                    else if (c == '#') {
                        if (i + 1 >= source_len || !is_digit(source[i + 1])) {
                            set_error(error, bf_err_syntax, i, "'#' must be followed by an argument index 0-9");
                            status = bf_err_syntax;
                        } else {
                            status = emit_checked(program, bf_op_load_arg, source[i + 1] - '0');
                            ++i;
                        }
                    }
                    else if (is_digit(c)) status = emit_checked(program, bf_op_store_arg, c - '0');
                    else if (is_label(c)) {
                        int label = c - 'a';

                        if (labels[label] < 0) {
                            if (program->len > (unsigned long)INT_MAX) status = bf_err_internal;
                            else labels[label] = (int)program->len;
                        }
                    }
                }

                break;
        }

        if (status != bf_ok) goto _fail;

        ++i;
    }

    if (in_comment) {
        set_error(error, bf_err_syntax, source_len, "unterminated '/' comment");
        status = bf_err_syntax;
        goto _fail;
    }

    if (loops.len != 0) {
        set_error(error, bf_err_syntax, source_len, "unmatched opening bracket");
        status = bf_err_syntax;
        goto _fail;
    }

    for (i = 0; i < patches.len; ++i) {
        patch patch_item = patches.data[i];
        int target = labels[patch_item.label];

        if (target < 0) {
            set_error(
                error, bf_err_syntax, 0, "undefined label '%c'",
                'a' + patch_item.label
            );
            status = bf_err_syntax;
            goto _fail;
        }

        program->code[patch_item.instruction_index].operand = target;
    }

    free(loops.data);
    free(patches.data);
    *out_program = program;

    return bf_ok;

_fail: {}

    if (error && error->code == bf_ok) {
        set_error(
            error, status, i, "%s",
            COM_status_name(status)
        );
    }

    free(loops.data);
    free(patches.data);
    PROG_free(program);

    return status;
}
