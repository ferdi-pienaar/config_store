
#ifndef CFG_MGR_SET_INT_H
#define CFG_MGR_SET_INT_H
#include "cfg_mgr_types.h"
#include <string>
#include <stdint.h> // uint8_t, etc

namespace cfg_mgr
{

bool cm_set_int(uint8_t *pItem, item_len_t len, std::string val);

}

#endif // CFG_MGR_SET_INT_H

