#include "coro_states.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include "libcorostacks.h"
#include "libcorostacks_int.h"
#include "utils.h"

struct file_impl
{
    FILE* input;
};

struct coredump_impl
{
    /* not implemented yet */
};

union source_impl
{
    struct file_impl file;
    struct coredump_impl coredump;
};

struct state_table
{
    st_source_t source_type;
    union source_impl impl;
};

st_result_t
open_state_table(state_table_t *handle, const char *filepath, st_source_t source)
{
    struct state_table *st = (struct state_table*) handle;
    st->source_type = source;

    if (source == ST_SOURCE_FILE)
    {
        st->impl.file.input = fopen(filepath, "rb");
        if (!st->impl.file.input)
            goto file_open_fail;

        return ST_SUCCESS;
    }
    else if (source == ST_SOURCE_COREDUMP)
    {
        abort();
        fprintf(stderr, "coredump source is not implemented yet\n");
    }

    cs_unreachable();

file_open_fail:
    error_report(CS_IO_ERROR, "Failed to open file \"%s\": %s",
                 filepath, strerror(errno));
    return ST_ERROR;
}

void
close_state_table(state_table_t *handle)
{
    struct state_table *st = (struct state_table*) handle;
    if (st->source_type == ST_SOURCE_FILE)
    {
        fclose(st->impl.file.input);
    }
}

int
st_next_entry(state_table_t *handle, ste_t *entry)
{
    struct state_table *st = (struct state_table *) handle;

    if (st->source_type == ST_SOURCE_FILE)
    {
        if (fread(entry, sizeof(ste_t), 1, st->impl.file.input) != 1)
            return ST_EOF;

        if (entry->sp == 0)
        {
            return ST_EOF;
        }

        return ST_ENTRY_PRESENT;
    }

    cs_unreachable();
}

void
st_reset_cursor(state_table_t *handle)
{
    struct state_table *st = (struct state_table *) handle;

    if (st->source_type == ST_SOURCE_FILE)
    {
        fseek(st->impl.file.input, 0, SEEK_SET);
    }

    cs_unreachable();
}

int
st_get_by_tid(state_table_t *handle, pid_t tid, ste_t *entry)
{
    struct state_table *st = (struct state_table *) handle;

    if (st->source_type == ST_SOURCE_FILE)
    {
        st_reset_cursor(handle);
        
        while (fread(entry, sizeof(ste_t), 1, st->impl.file.input) == 1)
        {
            if (entry->tid == tid)
                return ST_ENTRY_PRESENT;
        }

        return ST_EOF;
    }

    cs_unreachable();
}
