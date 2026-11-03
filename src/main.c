#include <main.h>

static char* read_file(const char* path, unsigned long* size_out) {
    FILE*  fp = fopen(path, "rb");
    long   size;
    char*  buffer;
    unsigned long nread;

    if (!fp) return NULL;

    if (fseek(fp, 0, SEEK_END) != 0) {
        fclose(fp);

        return NULL;
    }

    size = ftell(fp);

    if (size < 0) {
        fclose(fp);

        return NULL;
    }

    rewind(fp);
    buffer = (char*)malloc((unsigned long)size + 1u);

    if (!buffer) {
        fclose(fp);

        return NULL;
    }

    nread = fread(buffer, 1, (unsigned long)size, fp);

    if (nread != (unsigned long)size && ferror(fp)) {
        free(buffer);
        fclose(fp);

        return NULL;
    }

    buffer[nread] = '\0';
    fclose(fp);
    *size_out = nread;

    return buffer;
}

static void usage(const char* argv0) {
    fprintf(
        stderr, "Usage: %s [--dialect=bf|bfpp] [--tape=fixed|grow] <file>\n"
                "  fixed: 30000 cells\n"
                "  grow : starts at 1024 cells, grows up to 1 MiB\n", argv0
    );
}

int main(int argc, char** argv) {
    const char*  path       = NULL;
    const char*  dialect    = "bfpp";
    const char*  tape_kind  = "grow";
    char*        source     = NULL;
    unsigned long source_len = 0;
    bf_compiler* compiler   = NULL;
    bf_program*  program    = NULL;
    bf_tape*     tape       = NULL;
    bf_io*       io         = NULL;
    bf_vm*       vm         = NULL;
    bf_error     error;
    bf_status    status;
    int i;
    int rc = 1;

    for (i = 1; i < argc; ++i) {
        if (strncmp(argv[i], "--dialect=", 10) == 0)  dialect = argv[i] + 10;
        else if (strncmp(argv[i], "--tape=", 7) == 0) tape_kind = argv[i] + 7;
        else if (argv[i][0] == '-') {
            usage(argv[0]);

            return 2;
        }
        else if (path) {
            usage(argv[0]);

            return 2;
        }
        else path = argv[i];
    }

    if (!path) {
        usage(argv[0]);

        return 2;
    }

    source = read_file(path, &source_len);

    if (!source) {
        fprintf(stderr, "cannot read '%s': %s\n", path, strerror(errno));
        goto _cleanup;
    }

    if (strcmp(dialect, "bf") == 0)        compiler = COMP_classic_new();
    else if (strcmp(dialect, "bfpp") == 0) compiler = COMP_bfpp_new();
    else {
        fprintf(stderr, "unknown dialect: %s\n", dialect);
        goto _cleanup;
    }

    if (!compiler) goto _cleanup;

    status = COMP_compile(
        compiler, source, source_len, &program,
        &error
    );

    if (status != bf_ok) {
        fprintf(
            stderr, "compile error [%s] at %lu: %s\n", COM_status_name(status), (unsigned long)error.source_offset,
            error.message
        );
        goto _cleanup;
    }

    if (strcmp(tape_kind, "fixed") == 0)     tape = TAPE_fixed_new(30000);
    else if (strcmp(tape_kind, "grow") == 0) tape = TAPE_grow_new(1024, 1024u * 1024u);
    else {
        fprintf(stderr, "unknown tape: %s\n", tape_kind);
        goto _cleanup;
    }

    if (!tape) goto _cleanup;

    io = IO_stdio_new(stdin, stdout, 0);

    if (!io) goto _cleanup;

    vm = VM_switch_new(program, tape, io, (bf_vm_config){ 128u, 0u });

    if (!vm) goto _cleanup;

    tape = NULL;
    io = NULL;

    status = VM_run(vm, &error);

    if (status != bf_ok) {
        fprintf(
            stderr, "runtime error [%s] at pc=%lu: %s\n", COM_status_name(status), (unsigned long)error.source_offset,
            error.message
        );
        goto _cleanup;
    }

    rc = 0;

_cleanup: {}
    OBJ_destroy(bf_as_object(vm));
    OBJ_destroy(bf_as_object(io));
    OBJ_destroy(bf_as_object(tape));
    PROG_free(program);
    OBJ_destroy(bf_as_object(compiler));
    free(source);

    return rc;
}
