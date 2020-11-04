// Functions implemented by the application programmer, to extend
// the features provided by the Config_manager.
//

#include "my_cfg_fn.h"
#include <assert.h>
using namespace cfg_mgr;

//
void setdef_temp(uint8_t *pItem, item_len_t len)
{
    // Sanity check
    assert(len == sizeof(short));

    *((short *)pItem) = 39;
}


