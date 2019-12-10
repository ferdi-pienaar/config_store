/// This file contains an optional but useful extension
//  to the config manager: a setDefault function that
//  just 0's the block of memory.  For many applications,
//  this is the desired setdef action.

#include "config_manager_setdef_null.h"
#include <assert.h>
#include <string.h> // memset

using namespace std;

namespace cfg_mgr
{

// Write 0 to the item
// @param pItem - pointer to memory to set to default.
// @param len - number of bytes the item consists of
//
void cm_setdef_null(uint8_t *pItem, item_len_t len)
{
    assert(pItem != nullptr);
    memset(pItem, 0, len);
}

}