#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <assert.h>
#include <errno.h>

#include <elfutils/libdwfl.h>
#include <elfutils/libdw.h>
#include <libelf.h>

#include "libcorostacks.h"
#include "libcorostacks_int.h"
#include "utils.h"


static int
getframe_callback(Dwfl_Frame *frame, void *arg)
{
    Dwarf_Addr pc;
    bool isactivation;
    dwfl_frame_pc(frame, &pc, &isactivation);
    printf("\tpc: %lx, isactivation: %i\n", pc, isactivation);

    return DWARF_CB_OK;
}

csInstance_t*
csCoredumpAttach(const char *coredumpPath, const char *stateTablePath)
{
    assert(coredumpPath);

    csInstance_t *pInstance = (csInstance_t *) malloc(sizeof(*pInstance));
    if (!pInstance)
        goto instance_malloc_fail;
        
    static Dwfl_Callbacks dwfl_callbacks = {
        .find_elf = dwfl_build_id_find_elf,
        .find_debuginfo = dwfl_standard_find_debuginfo,
        .debuginfo_path = NULL // TODO: provide this path in separate call
    };

    pInstance->dwfl = dwfl_begin(&dwfl_callbacks);

    if (!pInstance->dwfl)
        goto dwfl_begin_fail;


    pInstance->coredump_fd = open(coredumpPath, 0, O_RDONLY);
    if (pInstance->coredump_fd < 0)
        goto coredump_open_fail;

    pInstance->coredump_elf = elf_begin(pInstance->coredump_fd,
                                        ELF_C_READ_MMAP, NULL);
    if (!pInstance->coredump_elf)
        goto elf_begin_fail;

    pid_t pid = 1; /*TODO: write function to read pid from coredump */
    coredump_dwfl_callbacks_init(pInstance->dwfl, pInstance->coredump_elf, pid,
                                 stateTablePath);

    return pInstance;

instance_malloc_fail:
    error_report(CS_OUT_OF_MEMORY, NULL);
    return NULL;

dwfl_begin_fail:
    free(pInstance);
    error_report(CS_INTERNAL_ERROR, "%s", dwfl_errmsg(-1));
    return NULL;

coredump_open_fail:
    dwfl_end(pInstance->dwfl);
    free(pInstance);
    error_report(CS_IO_ERROR, "failed to open \"%s\": %s",
                 coredumpPath, strerror(errno));
    return NULL;

elf_begin_fail:
    close(pInstance->coredump_fd);
    dwfl_end(pInstance->dwfl);
    free(pInstance);
    error_report(CS_INTERNAL_ERROR, "%s", dwfl_errmsg(-1));
    return NULL;
}

void csDetach(csInstance_t **ppInstance)
{
    assert(ppInstance);
    if (!*ppInstance)
        return;
    
    csInstance_t *pInstance = *ppInstance;

    elf_end(pInstance->coredump_elf);
    close(pInstance->coredump_fd);
    dwfl_end(pInstance->dwfl);
    free(pInstance);

    *ppInstance = NULL;
}

static int
coroutine_count_cb(Dwfl_Thread *thread, void *arg)
{
    size_t *pCoroutinesCount = (size_t *) arg;
    (*pCoroutinesCount)++;
    return DWARF_CB_OK;
}

typedef struct getcoroutine_arg
{
    csCoroutine_t *pCoroutines;
    size_t *pCoroutinesCount;
}
getcoroutine_arg_t;

static int
getcoroutine_cb(Dwfl_Thread *thread, void *arg)
{
    size_t *pCoroutinesCount = ((getcoroutine_arg_t *) arg)->pCoroutinesCount; 
    csCoroutine_t *pCoroutines = ((getcoroutine_arg_t *) arg)->pCoroutines; 

    pCoroutines[*pCoroutinesCount] = (csCoroutine_t) {
        .tid = dwfl_thread_tid(thread)
    };

    if (pCoroutinesCount)
        (*pCoroutinesCount)++;

    return DWARF_CB_OK;
}

int csEnumerateCoroutines(csInstance_t *pInstance, size_t *pCoroutinesCount,
                          csCoroutine_t *pCoroutines)
{
    assert(pInstance);

    if (!pCoroutines)
    {
        assert(pCoroutinesCount);
        int ret = dwfl_getthreads(pInstance->dwfl,
                                  coroutine_count_cb, pCoroutinesCount);
        if (ret == -1)
            goto dwfl_getthreads_fail;

        return CS_OK;
    }

    if (pCoroutinesCount)
        *pCoroutinesCount = 0;
    getcoroutine_arg_t arg = { pCoroutines, pCoroutinesCount };

    int ret = dwfl_getthreads(pInstance->dwfl,
                              getcoroutine_cb, &arg);
    if (ret == -1)
        goto dwfl_getthreads_fail;

    return CS_OK;

dwfl_getthreads_fail:
    error_report(CS_INTERNAL_ERROR,
                 "dwfl_getthreads() failed: %s", dwfl_errmsg(-1));
    return CS_FAIL;

}

int csEnumerateFrames(csInstance_t *pInstance, csCoroutine_t *pCoroutine,
                      size_t *pFrameCount, csFrame_t *pFrames)
{
    NOT_IMPLEMENTED_FUNCTION;
}

