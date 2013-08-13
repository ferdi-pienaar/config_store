/// This is used for all output to the user.
//  Not using printf or cout directly facilitates replacing it with a spy for test purposes.
//
#include <stdarg.h>
#include <stdio.h>
#include "config_manager_printf.h"

int cm_printf(const char * format, ...)
{
    va_list args;
    va_start (args, format);
    int ret = vprintf(format, args);
    va_end (args);
    return ret;
}

