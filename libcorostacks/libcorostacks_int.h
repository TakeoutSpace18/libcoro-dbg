#ifndef LIBCOROSTACKS_INT_H
#define LIBCOROSTACKS_INT_H

#include <elfutils/libdwfl.h>
#include "libcorostacks.h"

extern int cs_errno;

struct csInstance
{
    Dwfl *dwfl;
    int coredump_fd;
    Elf *coredump_elf;
};

/* Calls dwfl_attach_state with Dwfl_Thread_Callbacks setup for extracting
   coroutine state from the ELF core file. */

int coredump_dwfl_callbacks_init(Dwfl *dwfl, Elf *core, pid_t pid);

/* Read contents of coredump by virtual address */
int coredump_vmem_read(Elf *coredump_elf, csAddr_t addr,
                        size_t nbytes, char *result);


#endif /* LIBCOROSTACKS_INT_H */
