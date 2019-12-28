/// This file contains an optional but useful extension
//  to the config manager: set and print functions for
//  basic data types.  The user can add similar implementations
//  for special types, such as IP or Ethernet addresses.

#include "cfg_mgr_prt_int.h"
#include <assert.h>
#include <sstream>

using namespace std;

namespace cfg_mgr
{

// signed int
// pItem - pointer to memory containing an integer.
// len - number of bytes the integer consists of
//
// Since integers are kept in the order prescribed by the given
// system (little-endian or big-endian), we just typecast it
// it correctly and print.
//
string cm_prt_int(const uint8_t *pItem, item_len_t len)
{
    stringstream ss;
    switch (len)
    {
    case sizeof(int8_t):
        ss << *((int8_t *)pItem);
        break;

    case sizeof(int16_t):
        ss << *((int16_t *)pItem);
        break;

    case sizeof(int32_t):
        ss << *((int32_t *)pItem);
        break;

    case sizeof(int64_t):
        ss << *((int64_t *)pItem);
        break;

    default:
        assert("Unexpected input integer len."==0);
    }
    return ss.str();
}

}
