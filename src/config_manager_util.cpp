/// This file contains an optional but useful extension
//  to the config manager: set and print functions for
//  basic data types.  The user can add similar implementations
//  for special types, such as IP or Ethernet addresses.

#include "config_manager_util.h"
#include "config_manager_printf.h"
#include <assert.h>
#include <limits.h>
#include <stdint.h> // uint8_t, etc
#include <stdlib.h> // strto...
#include <sstream>
#include <iomanip> // setw

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


// signed int
// pItem - pointer to memory to write an integer to.
// len - number of bytes the integer consists of
// val - a string representing the new value
//
// @return false if the received value does not represent
//         in integer, or is out-of-range for the target data type
//
// Integers are kept in the order prescribed by the given
// system (little-endian or big-endian).
//
bool cm_set_int(uint8_t *pItem, item_len_t len, string val)
{
    char * pEnd; // pointer to char after chars accepted by strtol

    long long int v = strtoll(val.c_str(), &pEnd, 0);

    // Just return if v not initialized, i.e. if nothing read.
    // Can I rely on val.c_str returning the same address on
    // subsequent calls?
    if (pEnd == val.c_str())
    {
        cm_printf("Not an integer.\n");
        return false;
    }

    switch (len)
    {
    case sizeof(int8_t):
        if ((v > INT8_MAX) || (v < INT8_MIN))
        {
            return false;
        }
        memcpy(pItem, (int8_t *)&v, sizeof(int8_t));
        return true;

    case sizeof(int16_t):
        if ((v > INT16_MAX) || (v < INT16_MIN))
        {
            return false;
        }
        memcpy(pItem, (int16_t *)&v, sizeof(int16_t));
        return true;

    case sizeof(int32_t):
        if ((v > INT32_MAX) || (v < INT32_MIN))
        {
            return false;
        }
        memcpy(pItem, (int32_t *)&v, sizeof(int32_t));
        return true;

    case sizeof(int64_t):
        if ((v > INT64_MAX) || (v < INT64_MIN))
        {
            return false;
        }
        memcpy(pItem, (int64_t *)&v, sizeof(int64_t));
        return true;

    default:
        assert("Unexpected input integer len."==0);
        return false;
    }
}


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
