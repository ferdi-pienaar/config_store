#ifndef MY_CFG_FN_H
#define MY_CFG_FN_H

#include "config_manager.h"

void setdef_temp(uint8_t *pItem, cm_item_len_t len);
std::string prt_temp(const uint8_t *pItem, cm_item_len_t len);
bool set_temp(uint8_t *pItem, cm_item_len_t len, std::string val);

#endif // MY_CFG_FN_H


