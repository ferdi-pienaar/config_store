// The spy interface includes the basic interface, and additional methods.
#pragma once

#include "cfg_mgr_printf.h"

int cm_printf_spy(const char * format, ...);
void cm_printf_spy_init(void);
const char * cm_printf_spy_get(void);
