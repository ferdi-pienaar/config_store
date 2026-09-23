
#pragma once

#include "cfg_mgr_types.h"
#include <string>
#include <stdint.h> // uint8_t, etc

namespace cfg_mgr
{

bool cm_set_str(uint8_t *pItem, item_len_t len, std::string val);

}
