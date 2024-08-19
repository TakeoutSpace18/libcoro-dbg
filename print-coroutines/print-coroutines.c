#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>

#include "libcorostacks.h"

static csInstance_t *cs;

static void
help(void)
{
    printf("usage: print-coroutines <coredump_file>\n");
}

static void
print_backtrace(csCoroutine_t *coroutine)
{
    csFrame_t *frames = NULL;

    size_t frameCount;
    int ret = csEnumerateFrames(cs, coroutine, &frameCount, NULL);
    if (ret != CS_OK)
        goto enumerate_frames_fail;

    frames = malloc(sizeof(*frames) * frameCount);
    if (!frames)
        goto malloc_fail;

    ret = csEnumerateFrames(cs, coroutine, &frameCount, frames);
    if (ret != CS_OK)
        goto enumerate_frames_fail;

    printf("Coroutine (tid = %i)\n", coroutine->tid);
    for (size_t i = 0; i < frameCount; ++i)
    {
        printf("\t#%zu 0x%016lx in %s ()\n", i, frames[i].pc, frames[i].funcname);
    }

    free(frames);
    return;

enumerate_frames_fail:
    free(frames);
    fprintf(stderr, "Failed to get stack frames: %s\n", csErrorMessage());
    return;

malloc_fail:
    fprintf(stderr, "out of memory\n");
    abort();
}

int
main(int argc, char **argv) {
    const char *coredump_path;

    if (argc > 1 && strcmp(argv[1], "--help") == 0)
    {
        help();
        return EXIT_SUCCESS;
    }

    if (argc > 1)
        coredump_path = argv[1];
    else
    {
        help();
        return EXIT_FAILURE;
    }

    cs = csCoredumpAttach(coredump_path);
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
        print_backtrace(&coroutines[i]);
    }

    return EXIT_SUCCESS;


enumerate_coroutines_fail:
    csDetach(&cs);
coredump_attach_fail:
    fprintf(stderr, "%s", csErrorMessage());
    return EXIT_FAILURE;

malloc_fail:
    fprintf(stderr, "out of memory");
    csDetach(&cs);
    return EXIT_FAILURE;
}
