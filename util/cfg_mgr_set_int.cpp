/// This file contains an optional but useful extension to the config manager: a set function
// for a basic data type. The user can add similar implementations for special types,
// such as IP or Ethernet addresses.

#include "cfg_mgr_set_int.h"
#include <assert.h>
#include <limits.h>
#include <stdlib.h> // strto...
#include <cstring> // memcpy

using namespace std;

namespace cfg_mgr
{

static bool set_int(void *dest, long long v, size_t len, long long max, long long min, PRINTF_FN_TYPE print)
{
    if ((v > max) || (v < min))
    {
        if (!dest && print)
        {
            // This is evaluation, and we have the means to give user feedback.
            print("Out-of-range value for int len %ll bytes\n", len);
        }
        // Invalid set.
        return false;
    }
    if (dest)
    {
        memcpy(dest, &v, len);
    }
    // Valid set.
    return true;
}

// signed int
// pItem - pointer to memory to write an integer to.
// len - number of bytes the integer consists of
// val - a string representing the new value
//
// @return false if the received string does not represent
//         an integer, or is out-of-range for the target data type
//
// Integers are kept in the order prescribed by the given
// system (little-endian or big-endian).
//
bool cm_set_int(uint8_t *pItem, item_len_t len, string val, PRINTF_FN_TYPE print)
{
    char * pEnd; // pointer to char after chars accepted by strtol

    long long int v = strtoll(val.c_str(), &pEnd, 0);

    // Just return if v not initialized, i.e. if nothing read.
    // Can I rely on val.c_str returning the same address on subsequent calls?
    if (pEnd == val.c_str())
    {
        if (!pItem && print)
        {
            // This is evaluation, and we have the means to give user feedback.
            print("Invalid: '%s' is not an integer.\n", val.c_str());
        }
        return false;
    }

    switch (len)
    {
    case sizeof(int8_t):
        return set_int(pItem, v, len, INT8_MAX, INT8_MIN, print);

    case sizeof(int16_t):
        return set_int(pItem, v, len, INT16_MAX, INT16_MIN, print);

    case sizeof(int32_t):
        return set_int(pItem, v, len, INT32_MAX, INT32_MIN, print);

    case sizeof(int64_t):
        return set_int(pItem, v, len, INT64_MAX, INT64_MIN, print);

    default:
        assert("Unexpected input integer len."==0);
        return false;
    }
}

}
