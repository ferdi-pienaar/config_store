
#ifndef CFG_MAN_SET_INT_H
#define CFG_MAN_SET_INT_H
#include "config_manager_types.h"
#include <iostream>
#include <stdint.h> // uint8_t, etc

namespace cfg_mgr
{

bool cm_set_int(uint8_t *pItem, item_len_t len, std::string val);

}

#endif // CFG_MAN_SET_INT_H

