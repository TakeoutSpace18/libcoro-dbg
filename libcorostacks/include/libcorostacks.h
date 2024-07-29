#ifndef LIBCOROSTACKS_H
#define LIBCOROSTACKS_H

#include <libelf.h>
#include <stdlib.h>
#include <stdbool.h>

typedef unsigned long csAddr_t;

/* Handle that represents current libcorostacks
 * attachment to target address space */
typedef struct csInstance csInstance_t;

typedef struct csCoroutine
{
    pid_t tid;
}
csCoroutine_t;

#define CS_FUNCNAME_BUFSIZE 256

typedef struct csFrame
{
    csAddr_t pc;
    bool is_activation; /* See DWARF "activation" definition */

    /* can't get this from frame for some reason
    csAddr_t fp;
    csAddr_t sp;
    */

    char funcname[CS_FUNCNAME_BUFSIZE];
}
csFrame_t;

/* Initialize library by reading coroutine states from coredump file.
 * Returns NULL on failure. */
csInstance_t *csCoredumpAttach(const char *coredumpPath)
__nonnull_attribute__(1);

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


/* Retrieve list of frames of the given coroutine.
 * pFrames may be NULL, in which case only the number
 * of frames is obtained. 
 * On success returns CS_OK 
 * On failure returns CS_FAIL and sets csErrorMessage() */
int csEnumerateFrames(csInstance_t *pInstance, csCoroutine_t *pCoroutine,
                               size_t *pFrameCount, csFrame_t *pFrames)
__nonnull_attribute__(1, 2);


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
