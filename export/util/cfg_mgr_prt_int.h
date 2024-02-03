
#pragma once

#include "cfg_mgr_types.h"
#include <iostream>
#include <stdint.h> // uint8_t, etc

namespace cfg_mgr
{

std::string cm_prt_int(const uint8_t *pItem, item_len_t len);

}
