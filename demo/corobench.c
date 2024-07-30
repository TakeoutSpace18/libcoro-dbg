#include <bits/time.h>
#include <stddef.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <time.h>
#include <unistd.h>
#include <stdbool.h>

#include "coro.h"

struct coro_context main_context;

typedef struct {
    struct coro_context ctx;
    struct coro_stack stack;
    int number;
    size_t lifecount;
    bool finished;
} coroutine_t;

static void
bench_coro_main(void *arg)
{
    coroutine_t *self = (coroutine_t *) arg;
    
    size_t iter = 0;
    while (iter < self->lifecount)
    {
        ++iter;
        coro_transfer(&self->ctx, &main_context);
    }

    self->finished = true;
    // printf("coro #%i finished\n", self->number);
    coro_transfer(&self->ctx, &main_context);
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
create_coro_array(int num_coro, int coro_lifecount)
{
    coroutine_t *coros = malloc(num_coro * sizeof(coroutine_t));
    if (!coros) {
        fprintf(stderr, "out of memory\n");
        abort();
    }

    for (int i = 0; i < num_coro; ++i) {
        coros[i].number = i;
        coros[i].lifecount = coro_lifecount;
        coros[i].finished = false;
        create_coroutine(&coros[i], bench_coro_main, &coros[i]);
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

static int
benchmark(int max_coros, int step, int coro_lifecount)
{
    #ifdef CORO_DEBUG
        const char *filename = "corobench-st.dat";
    #else
        const char *filename = "corobench-no-st.dat";
    #endif /* ifdef CORO_DEBUG */

    FILE *measure_data = fopen(filename, "w");

    coro_create(&main_context, NULL, NULL, NULL, 0);
    for (int n_coros = step; n_coros <= max_coros; n_coros += step)
    {
        printf("Launching %i coroutines\n", n_coros);
        coroutine_t *coros = create_coro_array(n_coros, coro_lifecount);

        struct timespec begin_time;
        if (clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &begin_time) == -1) {
            perror("clock_gettime()");
            abort();
        }

        bool has_active_coro = true;
        while (has_active_coro) {
            has_active_coro = false;
            for (int i = 0; i < n_coros; ++i)
            {
                if (!coros[i].finished)
                {
                    has_active_coro = true;
                    coro_transfer(&main_context, &coros[i].ctx);
                }

            }
        }
        
        struct timespec end_time;
        if (clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &end_time) == -1) {
            perror("clock_gettime()");
            abort();
        }
        long int begin_ns = begin_time.tv_sec * 1e9 + begin_time.tv_nsec;
        long int end_ns = end_time.tv_sec * 1e9 + end_time.tv_nsec;
        long int time_elapsed = end_ns - begin_ns;

        fprintf(measure_data, "%i %li\n", n_coros, time_elapsed);

        destroy_coro_array(&coros, n_coros);
    }

    fclose(measure_data);

    return EXIT_SUCCESS;
}

int
main(int argc, char **argv)
{
    printf("pid: %i\n", getpid());

    int max_coros, step, coro_lifecount;
    if (argc == 4) {
        max_coros = atoi(argv[1]);
        step = atoi(argv[2]);
        coro_lifecount = atoi(argv[3]);
    }
    else {
        printf("usage: corobench <max_coros> <step> <coro_lifecount>\n");
        return EXIT_FAILURE;
    }

    if (max_coros <= 0 || step <= 0 || coro_lifecount <= 0) {
        fprintf(stderr, "params should be positive");
        return EXIT_FAILURE;
    }

    return benchmark(max_coros, step, coro_lifecount);
}

