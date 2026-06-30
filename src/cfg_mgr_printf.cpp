/// This is used for all output to the user.
//  Not using printf or cout directly facilitates replacing it with a spy for test purposes,
//  or allowing client to supply its own implementation.
//
#include <stdarg.h>
#include <stdio.h>
#include "cfg_mgr_printf.h"

namespace cfg_mgr
{

int cm_printf_local(const char * format, ...)
{
    va_list args;
    va_start (args, format);
    int ret = vprintf(format, args);
    va_end (args);
    return ret;
}

}
