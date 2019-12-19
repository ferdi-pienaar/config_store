
#ifndef CFG_MAN_PRT_STR_H
#define CFG_MAN_PRT_STR_H
#include "config_manager_types.h"
#include <iostream>
#include <stdio.h> // FILE
#include <string.h>

namespace cfg_mgr
{

std::string cm_prt_str(const uint8_t *pItem, item_len_t len);

}

#endif // CFG_MAN_PRT_STR_H

