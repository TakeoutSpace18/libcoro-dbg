/* This file contains libdwfl callbacks to iterate
 * through coroutines in coredump file */

#include <elf.h>
#include <gelf.h>
#include <stdio.h>
#include <stdlib.h>

#include <libelf.h>
#include <elfutils/libdwfl.h>
#include <assert.h>

#include "coro_states.h"
#include "libcorostacks.h"
#include "libcorostacks_int.h"
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
    else
    {
        /* no more coroutines */
        free(thread_arg);
        return 0;
    }

    cs_unreachable();
}

static bool
core_memory_read(Dwfl* dwfl, Dwarf_Addr addr, Dwarf_Word* result,
                 void* dwfl_arg)
{
    core_arg_t *core_arg = (core_arg_t*) dwfl_arg;
    Elf *coredump_elf = core_arg->coredump_elf;
    assert(coredump_elf);

    size_t phdr_num;
    if (elf_getphdrnum(coredump_elf, &phdr_num) < 0)
        goto elf_getphdrnum_fail;

    /* Find program header that contains desired addr,
     * and read the data from coredump */
    for (size_t i = 0; i < phdr_num; ++i)
    {
        GElf_Phdr phdr;
        GElf_Phdr *phdr_p = gelf_getphdr(coredump_elf, i, &phdr);
        if (!phdr_p || phdr.p_type != PT_LOAD)
            continue;

        GElf_Addr start = phdr.p_vaddr;
        GElf_Addr end = phdr.p_vaddr + phdr.p_memsz;
        
        if (addr < start || end <= addr)
            continue;

        if (phdr.p_align > 1)
            fprintf(stderr, "core_memory_read() warning:"
            "segment that contains addr %lx has positive alignment\n", addr);

        int64_t offset = phdr.p_offset + addr - start;
        const size_t nbytes = 8; /* only x86_64 is currently supported */
        Elf_Data *data = elf_getdata_rawchunk(coredump_elf, offset, nbytes, ELF_T_ADDR);
        if (data == NULL)
            goto elf_getdata_rawchunk_fail;

        assert(data->d_size == nbytes);

        *result = *((const Dwarf_Word*) data->d_buf);

        return true;
    }

    error_report(CS_INTERNAL_ERROR,
                 "program header containing desired address not found");
    return false;

elf_getphdrnum_fail:
    error_report(CS_INTERNAL_ERROR, "failed to get number of program headers");
    return false;

elf_getdata_rawchunk_fail:
    error_report(CS_INTERNAL_ERROR, "failed to read data from coredump file");
    return false;
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
    close_state_table(&core_arg->state_table);
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
coredump_dwfl_callbacks_init(Dwfl *dwfl, Elf *core, pid_t pid, const char* state_table_path)
{
    core_arg_t *core_arg = (core_arg_t*) malloc(sizeof(core_arg_t));
    if (!core_arg)
        goto core_arg_malloc_fail;

    core_arg->coredump_elf = core;

    st_result_t ret = open_state_table(&core_arg->state_table,
                                       state_table_path, ST_SOURCE_FILE);
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
