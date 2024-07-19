#include <stddef.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <unistd.h>
#include <stdbool.h>

#include "coro.h"

#define NUM_CORO_DEFAULT 16

struct coro_context main_context;

typedef struct {
    struct coro_context ctx;
    struct coro_stack stack;
    int number;
} coroutine_t;

static void
coroutine_func(coroutine_t *self, useconds_t sleep_time)
{
    printf("coro #%i: coroutine_func()\n", self->number);
    usleep(sleep_time);
    coro_transfer(&self->ctx, &main_context);
}

static void
coroutine_main(void *arg)
{
    coroutine_t *self = (coroutine_t*) arg; 
    useconds_t sleep_time = 400000;
    int iter = 0;
    for (;;) {
        printf("coro #%i; iter #%i: begin\n", self->number, iter);
        usleep(sleep_time);
        coro_transfer(&self->ctx, &main_context);

        coroutine_func(self, sleep_time);

        printf("coro #%i; iter #%i: end\n", self->number, iter);
        usleep(sleep_time);
        coro_transfer(&self->ctx, &main_context);

        ++iter;
    }
}

static void
create_coroutine(coroutine_t *c, coro_func func, void *arg)
{
    if (coro_stack_alloc(&c->stack, 4096) == 0) {
        fprintf(stderr, "coro_stack_alloc(): failed\n");
        abort();
    }
    coro_create(&c->ctx, func, arg, c->stack.sptr, c->stack.ssze);
}

static void
destroy_coroutine(coroutine_t *c)
{
    coro_destroy(&c->ctx);
    coro_stack_free(&c->stack);
}

static coroutine_t*
create_coro_array(int num_coro)
{
    coroutine_t *coros = malloc(num_coro * sizeof(coroutine_t));
    if (!coros) {
        fprintf(stderr, "out of memory\n");
        abort();
    }

    for (int i = 0; i < num_coro; ++i) {
        coros[i].number = i;
        create_coroutine(&coros[i], &coroutine_main, &coros[i]);
    }

    return coros;
}

static void
destroy_coro_array(coroutine_t **array, int num_coro)
{
    for (int i = 0; i < num_coro; ++i){
        destroy_coroutine(&(*array)[i]);
    }

    free(*array);
    *array = NULL;
}

int
main(int argc, char **argv)
{
    printf("pid: %i\n", getpid());

    int num_coro;
    if (argc == 2) {
        num_coro = atoi(argv[1]);
    }
    else if (argc == 1) {
        num_coro = NUM_CORO_DEFAULT;
    }
    else {
        printf("usage: corodemo <number of coroutines>\n");
    }

    if (num_coro <= 0) {
        fprintf(stderr, "num_coro should be positive\n");
    }

    coro_create(&main_context, NULL, NULL, NULL, 0);

    coroutine_t *coros = create_coro_array(num_coro);


    int cur_coro = 0;
    while (true) {

        coro_transfer(&main_context, &coros[cur_coro].ctx);
        cur_coro = (cur_coro + 1) % num_coro;
    }

    destroy_coro_array(&coros, num_coro);
    return EXIT_SUCCESS;
}
