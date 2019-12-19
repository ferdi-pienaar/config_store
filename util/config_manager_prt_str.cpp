/// This file contains an optional but useful extension
//  to the config manager: set and print functions for
//  basic data types.  The user can add similar implementations
//  for special types, such as IP or Ethernet addresses.

#include "config_manager_prt_str.h"
#include <sstream>

using namespace std;

namespace cfg_mgr
{

// C-style string
// pItem - pointer to memory containing a NULL-terminated string
// len - number of bytes the string consists of
//
string cm_prt_str(const uint8_t *pItem, item_len_t len)
{
    stringstream ss;
    ss << (char *)pItem;
    return ss.str();
}

}
