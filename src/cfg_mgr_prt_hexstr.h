
#ifndef CFG_MGR_PRT_HEXSTR_H
#define CFG_MGR_PRT_HEXSTR_H
#include "cfg_mgr_types.h"
#include <string>
#include <stdint.h> // uint8_t, etc

namespace cfg_mgr
{

std::string cm_prt_hexstr(const uint8_t *pItem, item_len_t len);

}

#endif // CFG_MGR_PRT_HEXSTR_H

