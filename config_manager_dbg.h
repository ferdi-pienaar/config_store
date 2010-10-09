
#ifndef CFG_MAN_DBG_H
#define CFG_MAN_DBG_H

#ifdef DEBUG_PRINT
#define DBG_PRT(fmt,...) printf(fmt, ##__VA_ARGS__)
#else
#define DBG_PRT(fmt,...)
#endif

#endif // CFG_MAN_DBG_H
