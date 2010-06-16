#include <iostream>
#include "config_manager.h"
#include "my_cfg_fn.h"
using namespace std;

// functions implemented by the application programmer, to extend
// the features provided by the config_manager.
//


//
void setdef_temp(unsigned char *pItem, cm_item_len len)
{
    // Sanity check
    assert(len == sizeof(short));

    *((short *)pItem) = 39;
}


