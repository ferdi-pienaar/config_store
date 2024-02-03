/// This is used for all output to the user.
//  Not using printf or cout directly facilitates replacing it with a spy for test purposes.
//
#pragma once

#include <stdarg.h>
namespace cfg_mgr
{
int cm_printf(const char * format, ...);
}
