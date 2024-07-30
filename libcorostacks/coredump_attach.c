/* This file contains libdwfl callbacks to iterate
 * through coroutines in coredump file */

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include <libelf.h>
#include <elfutils/libdwfl.h>
#include <elf.h>
#include <gelf.h>

#include "libcorostacks.h"
#include "libcorostacks_int.h"
#include "coro_states.h"
#include "errors.h"
#include "utils.h"

typedef struct core_arg
{
    state_table_t state_table;
    Elf *coredump_elf;
}
core_arg_t;

typedef struct coroutine_arg
{
    ste_t st_entry;
}
coroutine_arg_t;

static pid_t
core_next_thread(Dwfl* dwfl, void* dwfl_arg, void** thread_argp)
{
    core_arg_t *core_arg = (core_arg_t*) dwfl_arg;

    coroutine_arg_t *thread_arg;
    if (*thread_argp == NULL)
    {
        thread_arg = (coroutine_arg_t *) malloc(sizeof(*thread_arg));
        if (!thread_arg) {
            error_report(CS_OUT_OF_MEMORY, NULL);
            return -1;
        }

        st_reset_cursor(&core_arg->state_table);

        *thread_argp = thread_arg;
    }
    else
    {
        thread_arg = *thread_argp;
    }

    int ret = st_next_entry(&core_arg->state_table, &thread_arg->st_entry);
    if (ret == ST_ENTRY_PRESENT)
    {
        return thread_arg->st_entry.tid;
    }
    else if (ret == ST_EOF)
    {
        /* no more coroutines */
        free(thread_arg);
        return 0;
    }

    /* Error */
    return -1;
}

static bool
core_memory_read(Dwfl* dwfl, Dwarf_Addr addr, Dwarf_Word* result,
                 void* dwfl_arg)
{
    core_arg_t *core_arg = (core_arg_t*) dwfl_arg;
    Elf *coredump_elf = core_arg->coredump_elf;

    const size_t nbytes = 8; /* only x86_64 is currently supported */
    int ret = coredump_vmem_read(coredump_elf, addr, nbytes, (char *) result);

    return ret == CS_OK;
}

static bool
core_set_initial_registers(Dwfl_Thread* thread, void* thread_arg)
{
    coroutine_arg_t *coro_arg = (coroutine_arg_t*) thread_arg;

    dwfl_thread_state_register_pc(thread, coro_arg->st_entry.pc);
    if (!dwfl_thread_state_registers(thread, 6, 1, &coro_arg->st_entry.fp))
    {
        error_report(CS_INTERNAL_ERROR, "Failed to set fp register");
        return false;
    }


    if (!dwfl_thread_state_registers(thread, 7, 1, &coro_arg->st_entry.sp))
    {
        error_report(CS_INTERNAL_ERROR, "Failed to set sp register");
        return false;
    }

    return true;
}

static void
core_detach(Dwfl* dwfl, void* dwfl_arg)
{
    core_arg_t *core_arg = (core_arg_t*) dwfl_arg;
    st_close(&core_arg->state_table);
    free(core_arg);
}

static Dwfl_Thread_Callbacks callbacks = {
    .next_thread = core_next_thread,
    .get_thread = NULL, /* this method will be emulated by
                           next_thread callback (linear search) */
    .memory_read = core_memory_read,
    .set_initial_registers = core_set_initial_registers,
    .detach = core_detach,
    .thread_detach = NULL
};

int
coredump_dwfl_callbacks_init(Dwfl *dwfl, Elf *core, pid_t pid)
{
    core_arg_t *core_arg = (core_arg_t*) malloc(sizeof(core_arg_t));
    if (!core_arg)
        goto core_arg_malloc_fail;

    core_arg->coredump_elf = core;

    st_result_t ret = st_open_from_coredump(&core_arg->state_table,
                                        dwfl, core);
    if (ret != ST_SUCCESS)
        goto open_state_table_fail;

    if (dwfl_attach_state(dwfl, core, pid, &callbacks, core_arg) == false)
        goto dwfl_attach_state_fail;

    return CS_OK;

core_arg_malloc_fail:
    error_report(CS_OUT_OF_MEMORY, NULL);
    return CS_FAIL;

open_state_table_fail:
    return CS_FAIL;

dwfl_attach_state_fail:
    error_report(CS_INTERNAL_ERROR,
                 "dwfl_attach_state():\n\t%s", dwfl_errmsg(-1));
    return CS_FAIL;
}
