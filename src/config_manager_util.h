
#ifndef CFG_MAN_UTIL_H
#define CFG_MAN_UTIL_H
#include "config_manager_types.h"
#include <iostream>
#include <stdio.h> // FILE
#include <string.h>

std::string cm_prt_int(const uint8_t *pItem, cm_item_len_t len);
bool cm_set_int(uint8_t *pItem, cm_item_len_t len, std::string val);

std::string cm_prt_str(const uint8_t *pItem, cm_item_len_t len);
bool cm_set_str(uint8_t *pItem, cm_item_len_t len, std::string val);

std::string cm_prt_hexstr(const uint8_t *pItem, cm_item_len_t len);

#endif // CFG_MAN_UTIL_H

