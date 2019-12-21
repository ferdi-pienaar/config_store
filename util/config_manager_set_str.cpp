/// This file contains an optional but useful extension
//  to the config manager: set and print functions for
//  basic data types.  The user can add similar implementations
//  for special types, such as IP or Ethernet addresses.

#include "config_manager_set_str.h"
#include "config_manager_printf.h"
#include "config_manager_dbg.h"
#include <stdio.h> // snprintf

using namespace std;

namespace cfg_mgr
{

// Save C-style string in memory from string input with opening and closing quotes.
// pItem - pointer to memory to write the chars to.
// len - number of bytes reserved in memory (including terminator).
// val - a string representing the new value
//
// Note: truncate silently if input string is too long.
//
bool cm_set_str(uint8_t *pItem, item_len_t len, string val)
{
    if (val[0] != '\"')
    {
        cm_printf("String has no opening quote.\n");
        return false;
    }
    if (val[val.length()-1] != '\"')
    {
        cm_printf("String has no closing quote.\n");
        return false;
    }
    item_len_t write_bytes = (val.length() - 1 > len) ? len : val.length() - 1;
    DBG_PRT("%s: write_bytes=%u\n", __PRETTY_FUNCTION__, write_bytes);

    // Write excluding the opening quote in the string.
    snprintf((char *)pItem, write_bytes, val.c_str() + 1);
    return true;
}

}
