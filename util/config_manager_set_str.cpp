/// This file contains an optional but useful extension
//  to the config manager: set and print functions for
//  basic data types.  The user can add similar implementations
//  for special types, such as IP or Ethernet addresses.

#include "config_manager_set_str.h"
#include "config_manager_printf.h"
#include <assert.h>
#include <limits.h>
#include <stdlib.h> // strto...

using namespace std;

namespace cfg_mgr
{

// C-style string
// pItem - pointer to memory to write the chars to.
// len - number of bytes the string consists of
// val - a string representing the new value
//
bool cm_set_str(uint8_t *pItem, item_len_t len, string val)
{
    snprintf((char *)pItem, len, val.c_str());
    return true;
}

}
