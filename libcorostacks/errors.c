#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include "libcorostacks.h"
#include "utils.h"

#define ERROR_MSG_BUF_SIZE 256

int cs_errno;
char cs_errmsg[ERROR_MSG_BUF_SIZE];


csErrorCode_t
csErrorCode(void)
{
    return cs_errno;
}

const char*
csErrorMessage(void)
{
    return cs_errmsg;
}

const char*
errcode_to_string(csErrorCode_t errcode)
{
    switch (errcode)
    {
        case CS_OUT_OF_MEMORY:
            return "out of memory";
        case CS_INTERNAL_ERROR:
            return "internal error";
        case CS_IO_ERROR:
            return "input/output error";
    }

    return "unknown error";
}

void
error_report_impl(const char *funcname, csErrorCode_t errcode,
              const char *format, ...)
{
    cs_errno = errcode;

    snprintf(cs_errmsg, ERROR_MSG_BUF_SIZE, "%s(): %s",
             funcname, errcode_to_string(errcode));

    if (format)
    {
        va_list args;
        va_start(args, format);

        static char descr[ERROR_MSG_BUF_SIZE];
        vsnprintf(descr, ERROR_MSG_BUF_SIZE, format, args);
        
        strcat(cs_errmsg, "\n\t");
        strcat(cs_errmsg, descr);

        va_end(args);
    }
}

