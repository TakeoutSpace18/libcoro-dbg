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

int coredump_attach(Dwfl *dwfl, Elf *core, const char* state_table_path);

#define error_report(errcode, format, ...) \
    error_report_impl(__func__, errcode, format, ##__VA_ARGS__)

/* Set error message that can be accessed through csErrorMessage().
 * FORMAT may be NULL, in which case only error code will be printed. */
void error_report_impl(const char *funcname, csErrorCode_t errcode,
                  const char *format, ...);



#endif /* LIBCOROSTACKS_INT_H */
