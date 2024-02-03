#pragma once

#include "cfg_mgr_types.h" // cfg_mgr::item_len_t

void setdef_temp(uint8_t *pItem, cfg_mgr::item_len_t len);
std::string prt_temp(const uint8_t *pItem, cfg_mgr::item_len_t len);
bool set_temp(uint8_t *pItem, cfg_mgr::item_len_t len, std::string val);
