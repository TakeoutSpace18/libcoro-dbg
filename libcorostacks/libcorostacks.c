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

Dwfl_Callbacks dwfl_callbacks = {
    .find_elf = dwfl_build_id_find_elf,
    .find_debuginfo = dwfl_standard_find_debuginfo,
    .debuginfo_path = NULL // TODO: provide this path in separate call
};

csInstance_t*
csCoredumpAttach(const char *coredumpPath)
{
    assert(coredumpPath);

    csInstance_t *pInstance = (csInstance_t *) malloc(sizeof(*pInstance));
    if (!pInstance)
        goto instance_malloc_fail;
        

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

    int ret = dwfl_core_file_report(pInstance->dwfl,
                                    pInstance->coredump_elf, NULL);
    if (ret == -1)
        goto report_fail;

    dwfl_report_end(pInstance->dwfl, NULL, NULL);

    pid_t pid = 1; /*TODO: write function to read pid from coredump */
    coredump_dwfl_callbacks_init(pInstance->dwfl, pInstance->coredump_elf, pid);

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

report_fail:
    elf_end(pInstance->coredump_elf);
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

    if (pCoroutinesCount)
        *pCoroutinesCount = 0;

    if (!pCoroutines)
    {
        assert(pCoroutinesCount);
        int ret = dwfl_getthreads(pInstance->dwfl,
                                  coroutine_count_cb, pCoroutinesCount);
        if (ret == -1)
            goto dwfl_getthreads_fail;

        return CS_OK;
    }

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

static int
frame_count_cb(Dwfl_Frame *state, void *arg)
{
    size_t *pFrameCount = (size_t *) arg;
    (*pFrameCount)++;
    return DWARF_CB_OK;
}

typedef struct getframe_arg
{
    csFrame_t *pFrames;
    size_t *pFrameCount;
    Dwfl *dwfl;
}
getframe_arg_t;

static int
getframe_cb(Dwfl_Frame *state, void *arg)
{
    size_t *pFrameCount = ((getframe_arg_t *) arg)->pFrameCount; 
    csFrame_t *pFrames = ((getframe_arg_t *) arg)->pFrames; 
    Dwfl *dwfl = ((getframe_arg_t *) arg)->dwfl; 

    csFrame_t *frame = &pFrames[*pFrameCount];

    if (!dwfl_frame_pc(state, &frame->pc, &frame->is_activation))
        goto dwfl_fail;

    /* This doesn't work for some reason */
    /* 6 - DWARF rbp register number */
    // if (!dwfl_frame_reg(state, 6, &target_frame->fp))
        // goto dwfl_fail;

    /* 7 - DWARF rsp register number */
    // if (!dwfl_frame_reg(state, 7, &target_frame->sp))
        // goto dwfl_fail;

    /* Get function name */
    Dwarf_Addr pc_adjusted = frame->pc - (frame->is_activation ? 1 : 0);
    Dwfl_Module *module = dwfl_addrmodule(dwfl, pc_adjusted);
    const char *name = dwfl_module_addrname(module, pc_adjusted);
    if (name)
        strncpy(frame->funcname, name, CS_FUNCNAME_BUFSIZE);
    else 
        snprintf(frame->funcname, CS_FUNCNAME_BUFSIZE, "??");


    if (pFrameCount)
        (*pFrameCount)++;

    return DWARF_CB_OK;

dwfl_fail:
    error_report(CS_INTERNAL_ERROR, "dwfl_frame_pc(): %s", dwfl_errmsg(-1));
    return DWARF_CB_ABORT;
}

int csEnumerateFrames(csInstance_t *pInstance, csCoroutine_t *pCoroutine,
                      size_t *pFrameCount, csFrame_t *pFrames)
{
    assert(pInstance);
    assert(pCoroutine);

    if (pFrameCount)
        *pFrameCount = 0;

    if (!pFrames)
    {
        assert(pFrameCount);
        int ret = dwfl_getthread_frames(pInstance->dwfl, pCoroutine->tid,
                                        frame_count_cb, pFrameCount);
        if (ret == -1)
            goto dwfl_getthread_frames_fail;

        return CS_OK;
    }

    getframe_arg_t arg = { pFrames, pFrameCount, pInstance->dwfl };

    int ret = dwfl_getthread_frames(pInstance->dwfl, pCoroutine->tid,
                                    getframe_cb, &arg);
    if (ret == -1)
        goto dwfl_getthread_frames_fail;

    return CS_OK;

dwfl_getthread_frames_fail:
    error_report(CS_INTERNAL_ERROR,
                 "dwfl_getthread_frames() failed: %s", dwfl_errmsg(-1));
    return CS_FAIL;
}

