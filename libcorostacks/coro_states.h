#ifndef CORO_STATES_H
#define CORO_STATES_H

#include <elfutils/libdwfl.h>
#include "libcorostacks.h"


typedef struct state_table_entry
{
    csAddr_t sp;
    csAddr_t pc;
    csAddr_t fp;
    pid_t tid;
} ste_t;

#define ST_HANDLE_SIZE 32
typedef struct
{
    char opaque[ST_HANDLE_SIZE];
} state_table_t;

typedef enum
{
    ST_ENTRY_PRESENT = 1,
    ST_SUCCESS = 0,
    ST_ERROR = -1,
    ST_EOF = -2
} st_result_t;

/* Open coroutine states table from separate file,
 * generated by modified libcoro */
st_result_t st_open_from_file(state_table_t *handle, const char* filepath);

/* Open coroutine states table directly from address space in the core file */
st_result_t st_open_from_coredump(state_table_t *handle, Dwfl *dwfl, Elf *coredump_elf);

void st_close(state_table_t *handle);

/* Begin reading state table from first entry */
void st_reset_cursor(state_table_t *handle);

/* Get next entry from the state table.
 * Returns positive value if entry is found, 0 otherwise */
int st_next_entry(state_table_t *handle, ste_t *entry);

/* Find entry by TID
 * Return ST_ENTRY_PRESENT on success, ST_EOF on failure */
int st_get_by_tid(state_table_t *handle, pid_t tid, ste_t *entry);

#endif /* CORO_STATES_H */
