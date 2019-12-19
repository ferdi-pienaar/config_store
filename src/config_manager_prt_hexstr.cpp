/// This file contains an optional but useful extension
//  to the config manager: set and print functions for
//  basic data types.  The user can add similar implementations
//  for special types, such as IP or Ethernet addresses.

#include "config_manager_prt_hexstr.h"
#include <sstream>
#include <iomanip> // setw

using namespace std;

namespace cfg_mgr
{

// Print memory as array of hex bytes
// pItem - pointer to memory
// len - number of bytes
//
string cm_prt_hexstr(const uint8_t *pItem, item_len_t len)
{
    stringstream ss;

    for (item_len_t i = 0; i < len; i++)
    {
        ss << setfill('0') << setw(2) << hex << (int)pItem[i];
    }
    return ss.str();
}

}
