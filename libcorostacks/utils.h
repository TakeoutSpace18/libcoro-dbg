#ifndef UTILS_H
#define UTILS_H

#include <stdlib.h>
#include <stdio.h>

#define cs_unreachable() __builtin_unreachable

#define FORCEINLINE __attribute__ ((always_inline))

#define NOT_IMPLEMENTED_FUNCTION do { \
    fprintf(stderr, "%s:%i %s is not implemented yet\n",\
            __FILE__, __LINE__, __func__); \
    abort(); \
} while (0)

#define NOT_IMPLEMENTED do { \
    fprintf(stderr, "%s:%i this is not implemented yet\n",\
            __FILE__, __LINE__); \
    abort(); \
} while (0)

#endif /* UTILS_H */
