
#ifndef CFG_MAN_UTIL_H
#define CFG_MAN_UTIL_H
#include "config_manager_types.h"
#include <iostream>

void cm_prt_int(FILE * f, const uint8_t *pItem, cm_item_len_t len);
bool cm_set_int(uint8_t *pItem, cm_item_len_t len, std::string val);

void cm_prt_str(FILE * f, const uint8_t *pItem, cm_item_len_t len);
bool cm_set_str(uint8_t *pItem, cm_item_len_t len, std::string val);

void cm_prt_hexstr(FILE * f, const uint8_t *pItem, cm_item_len_t len);

#endif // CFG_MAN_UTIL_H

