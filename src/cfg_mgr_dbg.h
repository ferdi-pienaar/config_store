
#pragma once

#ifdef DEBUG_PRINT
#define DBG_PRT(fmt,...) printf(fmt, ##__VA_ARGS__)
#else
#define DBG_PRT(fmt,...)
#endif
