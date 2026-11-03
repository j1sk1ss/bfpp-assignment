#include <vm_internal.h>

typedef struct {
    bf_tape*           tape;
    unsigned long      return_pc;
    unsigned char      args[10];
    unsigned char      last_return;
} bf_frame;

typedef struct {
    bf_vm              super;
    const bf_program*  program;
    bf_tape*           tape_prototype;
    bf_io*             io;
    bf_vm_config       config;
    unsigned long long step_count;
} bf_switch_vm;

const bf_type_info bf_type_vm        = { "bf_vm",        &bf_type_object };
const bf_type_info bf_type_switch_vm = { "bf_switch_vm", &bf_type_vm     };

static void set_error(
    bf_error* error, bf_status status, unsigned long pc, const char* fmt,
    ...
) {
    va_list args;

    if (!error) return;

    error->code = status;
    error->source_offset = pc;
    va_start(args, fmt);
    vsnprintf(error->message, sizeof(error->message), fmt, args);
    va_end(args);
}

static void destroy_frames(bf_frame* frames, unsigned long depth) {
    unsigned long i;

    if (!frames) return;

    for (i = 0; i < depth; ++i) {
        OBJ_destroy(bf_as_object(frames[i].tape));
    }
}

static bf_status switch_run(bf_vm* base, bf_error* error) {
    bf_switch_vm* self = (bf_switch_vm*)base;
    const bf_instruction* code = PROG_code(self->program);
    unsigned long code_len = PROG_length(self->program);
    unsigned long max_depth = self->config.max_call_depth ? self->config.max_call_depth : 64u;
    bf_frame* frames = NULL;
    unsigned long depth = 1u;
    unsigned long pc = 0u;
    bf_status status = bf_ok;

    COM_error_clear(error);
    self->step_count = 0;
    frames = (bf_frame*)calloc(max_depth, sizeof(*frames));

    if (!frames) return bf_err_oom;

    frames[0].tape = TAPE_spawn(self->tape_prototype);

    if (!frames[0].tape) {
        free(frames);

        return bf_err_oom;
    }

    while (pc < code_len) {
        bf_frame* frame = &frames[depth - 1u];
        const bf_instruction* ins = &code[pc];
        unsigned char cell;

        if (self->config.max_steps && self->step_count >= self->config.max_steps) {
            set_error(error, bf_err_internal, pc, "step limit exceeded");
            status = bf_err_internal;
            goto _done;
        }

        ++self->step_count;

        switch ((bf_op_code)ins->opcode) {
            case bf_op_add: {
                TAPE_add(frame->tape, ins->operand);
                ++pc;
                break;
            }
            case bf_op_move: {
                status = TAPE_move(frame->tape, ins->operand);

                if (status != bf_ok) {
                    set_error(
                        error, status, pc, "tape move out of bounds at pc=%lu",
                        (unsigned long)pc
                    );
                    goto _done;
                }

                ++pc;
                break;
            }
            case bf_op_input: {
                int value = IO_read_byte(self->io);

                TAPE_set(frame->tape, value < 0 ? 0u : (unsigned char)value);
                ++pc;
                break;
            }
            case bf_op_output: {
                status = IO_write_byte(self->io, TAPE_get(frame->tape));

                if (status != bf_ok) {
                    set_error(
                        error, status, pc, "output error at pc=%lu",
                        (unsigned long)pc
                    );
                    goto _done;
                }

                ++pc;
                break;
            }
            case bf_op_jump_if_zero: {
                pc = TAPE_get(frame->tape) == 0 ? (unsigned long)ins->operand : pc + 1u;
                break;
            }
            case bf_op_jump_if_nonzero: {
                pc = TAPE_get(frame->tape) != 0 ? (unsigned long)ins->operand : pc + 1u;
                break;
            }
            case bf_op_zero: {
                TAPE_set(frame->tape, 0);
                ++pc;
                break;
            }
            case bf_op_jump: {
                pc = (unsigned long)ins->operand;
                break;
            }
            case bf_op_call: {
                if (depth >= max_depth) {
                    set_error(
                        error, bf_err_call_depth, pc, "maximum call depth (%lu) exceeded",
                        (unsigned long)max_depth
                    );
                    status = bf_err_call_depth;
                    goto _done;
                }

                frames[depth].tape = TAPE_spawn(self->tape_prototype);

                if (!frames[depth].tape) {
                    status = bf_err_oom;
                    goto _done;
                }

                frames[depth].return_pc = pc + 1u;
                ++depth;
                pc = (unsigned long)ins->operand;
                break;
            }
            case bf_op_return_or_halt: {
                if (depth == 1u) {
                    pc = code_len;
                    break;
                }

                cell = TAPE_get(frame->tape);
                pc = frame->return_pc;
                OBJ_destroy(bf_as_object(frame->tape));
                memset(frame, 0, sizeof(*frame));
                --depth;
                frames[depth - 1u].last_return = cell;
                break;
            }
            case bf_op_load_arg: {
                if (depth > 1u) {
                    TAPE_set(frame->tape, frames[depth - 2u].args[(unsigned)ins->operand]);
                }

                ++pc;
                break;
            }
            case bf_op_store_arg: {
                frame->args[(unsigned)ins->operand] = TAPE_get(frame->tape);
                ++pc;
                break;
            }
            case bf_op_load_return: {
                TAPE_set(frame->tape, frame->last_return);
                ++pc;
                break;
            }
            default: {
                set_error(
                    error, bf_err_internal, pc, "invalid opcode %u",
                    (unsigned)ins->opcode
                );
                status = bf_err_internal;
                goto _done;
            }
        }
    }

    status = IO_flush(self->io);

    if (status != bf_ok) set_error(error, status, pc, "flush failed");

_done: {}
    destroy_frames(frames, depth);
    free(frames);

    return status;
}

static inline unsigned long long switch_steps(const bf_vm* base) {
    return ((const bf_switch_vm*)base)->step_count;
}

static inline void switch_destroy(bf_object* object) {
    bf_switch_vm* self = (bf_switch_vm*)object;

    OBJ_destroy(bf_as_object(self->tape_prototype));
    OBJ_destroy(bf_as_object(self->io));
    free(self);
}

static const bf_vm_vtable_type switch_vtable = {
    { &bf_type_switch_vm, switch_destroy },
    switch_run,
    switch_steps
};

bf_vm* VM_switch_new(const bf_program* program, bf_tape* tape_prototype, bf_io* io, bf_vm_config config) {
    bf_switch_vm* self;

    if (!program || !tape_prototype || !io) return NULL;

    self = (bf_switch_vm*)calloc(1, sizeof(*self));

    if (!self) return NULL;

    OBJ_init(&self->super.super, &switch_vtable.super);
    self->program = program;
    self->tape_prototype = tape_prototype;
    self->io = io;
    self->config = config;

    return &self->super;
}

bf_status VM_run(bf_vm* self, bf_error* error) {
    if (!self) return bf_err_bad_argument;

    return bf_vm_vtable(self)->run(self, error);
}

unsigned long long VM_steps(const bf_vm* self) {
    return self ? bf_vm_vtable(self)->steps(self) : 0u;
}
