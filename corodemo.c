#include <stddef.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <unistd.h>

#include "coro.h"
#include "corodemo.h"


coro_state_t g_coro_state;

struct coro_stack coroA_stack;
struct coro_context coroA_context;

struct coro_stack coroB_stack;
struct coro_context coroB_context;

static inline void dump_coro_state(coro_state_t *state);
static void coroutineA_main(void *arg);
static void coroutineB_main(void *arg);
static int coroutineA_func(int a, int b);
static int coroutineB_func(int a, int b);
int main(void);


static inline void 
dump_coro_state(coro_state_t *state)
{
    static FILE *coro_states_file = 0;
    if (!coro_states_file) {
        coro_states_file = fopen("coro_states.txt", "w");
    }

    fprintf(coro_states_file, "%lx %lx %lx\n",
            state->pc, state->sp, state->fp);
    fflush(coro_states_file);
}

static void
coroutineA_main(void *arg)
{
    int var;
    printf("coroutineA stack address: %p\n", &var);

    for (;;) {
        printf("Hello from coroutineA\n");
        printf("result: %i\n", coroutineA_func(123, 7));
        coro_transfer(&coroA_context, &coroB_context);
        dump_coro_state(&g_coro_state);
    }
}

static void
coroutineB_main(void *arg)
{
    int var;
    printf("coroutineB stack address: %p\n", &var);

    for (;;) {
        printf("Hello from coroutineB\n");
        printf("result: %i\n", coroutineB_func(45, 23));
        coro_transfer(&coroB_context, &coroA_context);
        dump_coro_state(&g_coro_state);
    }
}

static int
coroutineA_func(int a, int b)
{
    printf("coroutineA_func()\n");
    sleep(3);
    return a * 100 + b;
}

static int
coroutineB_func(int a, int b)
{
    printf("coroutineB_func()\n");
    sleep(3);
    return a * 256 + b;
}

int main(void)
{
    printf("pid: %i\n", getpid());

    struct coro_context main_context;
    coro_create(&main_context, NULL, NULL, NULL, 0);

    coro_stack_alloc(&coroA_stack, 4096);
    coro_create(&coroA_context, coroutineA_main, NULL, coroA_stack.sptr, coroA_stack.ssze);

    coro_stack_alloc(&coroB_stack, 4096);
    coro_create(&coroB_context, coroutineB_main, NULL, coroB_stack.sptr, coroB_stack.ssze);

    coro_transfer(&main_context, &coroA_context);
    dump_coro_state(&g_coro_state);
    return EXIT_SUCCESS;
}
