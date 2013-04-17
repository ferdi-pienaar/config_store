/// This is used for all output to the user.
//  Not using printf or cout directly facilitates replacing it with a spy for test purposes.
//
#ifndef CFG_MAN_PRINTF_H
#define CFG_MAN_PRINTF_H
#include <stdarg.h>
int cm_printf(const char * format, ...);
#endif

