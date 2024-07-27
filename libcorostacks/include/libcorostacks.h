#ifndef LIBCOROSTACKS_H
#define LIBCOROSTACKS_H

#include <libelf.h>
#include <stdlib.h>

/* Handle that represents current libcorostacks
 * attachment to target address space */
typedef struct csInstance csInstance_t;

typedef struct csCoroutine
{
    int tid;
}
csCoroutine_t;

typedef struct csFrame csFrame_t;

/* Initialize library by reading coroutine states from coredump file.
 * Returns NULL on failure. */
csInstance_t *csCoredumpAttach(const char *coredumpPath,
                               const char *stateTablePath)
__nonnull_attribute__(1, 2);

/* Finish working with library and release resources */
void csDetach(csInstance_t **ppInstance)
__nonnull_attribute__(1);

/* Retrieve list of coroutines that are present in target address space.
 * pCoroutines may be NULL, in which case only the number
 * of coroutines is obtained. 
 * On success returns CS_OK 
 * On failure returns CS_FAIL and sets csErrorMessage() */
int csEnumerateCoroutines(csInstance_t *pInstance, size_t *pCoroutinesCount,
                          csCoroutine_t *pCoroutines)
__nonnull_attribute__(1);

int csEnumerateFrames(csInstance_t *pInstance, csCoroutine_t *pCoroutine,
                               size_t *pFrameCount, csFrame_t *pFrames);


/******************** Error handling ********************/

/* function return codes */
enum {
    CS_OK = 1,
    CS_FAIL = -1
};

typedef enum csErrorCode
{
    CS_OUT_OF_MEMORY,
    CS_INTERNAL_ERROR,
    CS_IO_ERROR
}
csErrorCode_t;

csErrorCode_t csErrorCode(void);
const char *csErrorMessage(void);


#endif // LIBCOROSTACKS_H
