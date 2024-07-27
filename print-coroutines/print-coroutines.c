#include <stdio.h>
#include <stdlib.h>

#include "libcorostacks.h"

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

    csInstance_t *cs = csCoredumpAttach(coredump_path, state_table_path);
    if (!cs)
        goto coredump_attach_fail;

    size_t coroutineCount;
    int ret = csEnumerateCoroutines(cs, &coroutineCount, NULL);
    if (ret != CS_OK)
        goto enumerate_coroutines_fail;

    csCoroutine_t *coroutines = malloc(sizeof(csCoroutine_t) * coroutineCount);
    if (!coroutines)
        goto malloc_fail;

    ret = csEnumerateCoroutines(cs, &coroutineCount, coroutines);
    if (ret != CS_OK)
        goto enumerate_coroutines_fail;

    printf("Found %zu coroutines\n", coroutineCount);
    for (size_t i = 0; i < coroutineCount; ++i)
    {
        printf("Coroutine #%i\n", coroutines[i].tid);
    }

    return EXIT_SUCCESS;

coredump_attach_fail:
    fprintf(stderr, "csCoredumpAttach(): %s\n", csErrorMessage());
    return EXIT_FAILURE;

enumerate_coroutines_fail:
    fprintf(stderr, "csEnumerateCoroutines(): %s\n", csErrorMessage());
    csDetach(&cs);
    return EXIT_FAILURE;

malloc_fail:
    fprintf(stderr, "out of memory");
    csDetach(&cs);
    return EXIT_FAILURE;
}
