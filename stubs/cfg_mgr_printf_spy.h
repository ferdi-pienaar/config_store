// The spy interface includes the basic interface, and additional methods.
#ifndef CFG_MGR_PRINTF_SPY_H
#define CFG_MGR_PRINTF_SPY_H

#include "cfg_mgr_printf.h"

void cm_printf_spy_init(void);
const char * cm_printf_spy_get(void);

#endif // CFG_MGR_PRINTF_SPY_H
