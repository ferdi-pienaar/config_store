/// This file contains an optional but useful extension
//  to the config manager: set and print functions for
//  basic data types.  The user can add similar implementations
//  for special types, such as IP or Ethernet addresses.

#include "config_manager_prt_str.h"
#include <sstream>

using namespace std;

namespace cfg_mgr
{

// C-style string in memory to string output.
// pItem - pointer to memory containing a NULL-terminated string
// len - number of bytes reserved in memory (including terminator).
//
// The string output surrounds the string with quotes; we chose
// that as the format that the user sees/inputs, because it is
// also useful when we save JSON format in NVRAM.
//
string cm_prt_str(const uint8_t *pItem, item_len_t len)
{
    stringstream ss;
    ss << "\"" << (char *)pItem << "\"";
    return ss.str();
}

}
