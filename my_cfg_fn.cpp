// Functions implemented by the application programmer, to extend
// the features provided by the config_manager.
//

#include <iostream>
#include "config_manager.h"
#include "my_cfg_fn.h"
using namespace std;


//
void setdef_temp(uint8_t *pItem, cm_item_len_t len)
{
    // Sanity check
    assert(len == sizeof(short));

    *((short *)pItem) = 39;
}


