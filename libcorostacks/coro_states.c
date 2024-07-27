#include "coro_states.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include "utils.h"

int st_errno;
static int saved_errno;

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
        {
            st_errno = ST_ERROR_OPEN_FILE; 
            saved_errno = errno;
            return ST_ERROR;
        }

        return ST_SUCCESS;
    }
    else if (source == ST_SOURCE_COREDUMP)
    {
        abort();
        fprintf(stderr, "coredump source is not implemented yet\n");
    }
    else
    {
        return ST_ERROR_INVALID_ARGUMENTS;
    }

    cs_unreachable();
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

const char*
st_strerror(int st_errno)
{
    static char strbuf[256];

    switch (st_errno)
    {
        case ST_ERROR_OPEN_FILE:
            sprintf(strbuf, "failed to open file: %s", strerror(saved_errno));
            break;
        case ST_ERROR_INVALID_ARGUMENTS:
            sprintf(strbuf, "invalid arguments");
            break;
    }

    return strbuf;
}

