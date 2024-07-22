#include <stdio.h>
#include <stdlib.h>

#include "coro_states.h"
#include "utils.h"

#define STATE_TABLE_DEFAULT_NAME "coro_states.bin"

static void
help(void)
{
    printf("usage: corostacks <coredump_file> [<state_table_file>]\n");
}

int
main(int argc, char **argv) {
    const char *coredump_path;
    const char *state_table_path;

    if (argc > 1)
        coredump_path = argv[1];
    else
    {
        help();
        return EXIT_FAILURE;
    }

    if (argc > 2)
        state_table_path = argv[2];
    else
        state_table_path = STATE_TABLE_DEFAULT_NAME;

    state_table_t st;
    if (open_state_table(&st, state_table_path, ST_SOURCE_FILE) != ST_SUCCESS)
    {
        fprintf(stderr, "open_state_table():\n\t%s\n", st_strerror(st_errno));
        return EXIT_FAILURE;
    }

    ste_t entry;
    while (st_next_entry(&st, &entry) == ST_ENTRY_PRESENT)
    {
        printf("sp: %lu, fp: %lu, pc: %lu\n", entry.sp, entry.fp, entry.pc);
    }

    close_state_table(&st);
    return EXIT_SUCCESS;
}
