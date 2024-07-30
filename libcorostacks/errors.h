#ifndef ERRORS_H
#define ERRORS_H

#include <libcorostacks.h>

/* Set error message that can be accessed through csErrorMessage().
 * FORMAT may be NULL, in which case only error code will be printed. */
#define error_report(errcode, format, ...) \
    error_report_impl(__func__, errcode, format, ##__VA_ARGS__)

void error_report_impl(const char *funcname, csErrorCode_t errcode,
                  const char *format, ...)
__attribute__ ((format (printf, 3, 4)));


#endif
