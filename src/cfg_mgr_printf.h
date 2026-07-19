/// This is used for all output to the user.
//  Client supplies its own implementation, and tests can replace it with a spy.
//
#pragma once
#include "cfg_mgr_types.h"
#include <stdarg.h>

namespace cfg_mgr
{

// Function pointer cm_printf points to the implementation: supplied by client (or test).
extern PRINTF_FN_TYPE cm_printf;
}
