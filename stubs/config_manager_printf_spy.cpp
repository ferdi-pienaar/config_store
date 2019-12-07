/// This spy implementation prints not to stdout, but to
//   a buffer that can later be compared to the expected CUT output.
//
#include <stdarg.h>
#include <stdio.h>
#include "config_manager_printf_spy.h"

static const unsigned int BUF_SIZE = 4096;
static char buf[BUF_SIZE];
static int offset; // offset in buffer at which next cm_printf will write

int cfg_mgr::cm_printf(const char * format, ...)
{
    va_list args;
    va_start (args, format);
    int bytes = vsprintf(buf + offset, format, args);
    va_end (args);
    offset += bytes;
    return bytes;
}

////////////////////////////////////////////////////////////////////////////////
//
// Methods outside of config_manager_printf's interface, for test purposes
//
////////////////////////////////////////////////////////////////////////////////
void cm_printf_spy_init(void)
{
    offset = 0;
}


const char * cm_printf_spy_get(void)
{
    return buf;
}

