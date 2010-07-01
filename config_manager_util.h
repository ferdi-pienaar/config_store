
#ifndef CFG_MAN_UTIL_H
#define CFG_MAN_UTIL_H
#include "config_manager.h"

void cm_prt_int(const unsigned char *pItem, cm_item_len len);
bool cm_set_int(unsigned char *pItem, cm_item_len len, std::string val);

void cm_prt_str(const unsigned char *pItem, cm_item_len len);
bool cm_set_str(unsigned char *pItem, cm_item_len len, std::string val);


#endif // CFG_MAN_UTIL_H

