// The spy interface includes the basic interface, and additional methods.
#ifndef CM_PRINTF_SPY_H
#define CM_PRINTF_SPY_H

#include "config_manager_printf.h"

void cm_printf_spy_init(void);
const char * cm_printf_spy_get(void);
#endif // CM_PRINTF_SPY_H

