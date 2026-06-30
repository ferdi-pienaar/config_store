/// This is used for all output to the user.
//  Not using printf or cout directly facilitates replacing it with a spy for test purposes,
//  or allowing client to supply its own implementation.
//
#pragma once

#include <stdarg.h>

namespace cfg_mgr
{

// Function pointer cm_printf points to the implementation: local or set by client or test.
// xxx currently, the client API does not allow it to choose its own function.
using print_fn_t = int(*)(const char* fmt, ...);
extern print_fn_t cm_printf;

int cm_printf_local(const char * format, ...);
}
