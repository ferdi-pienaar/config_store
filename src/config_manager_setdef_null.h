
#ifndef CFG_MAN_SETDEF_NULL_H
#define CFG_MAN_SETDEF_NULL_H

#include "config_manager_types.h"
#include <stdint.h> // uint8_t, etc

namespace cfg_mgr
{
void cm_setdef_null(uint8_t *pItem, item_len_t len);
}

#endif // CFG_MAN_SETDEF_NULL_H
