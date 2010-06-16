/// This file contains an optional but useful extension
//  to the config manager: set and print functions for
//  basic data types.  The user can add similar implementations
//  for special types, such as IP or Ethernet addresses.

#include "config_manager_util.h"
#include <assert.h>
#include <limits.h>
#include <stdint.h> // uint8_t, etc

using namespace std;

// signed int
// pItem - pointer to memory containing an integer.
// len - number of bytes the integer consists of
//
// Since integers are kept in the order prescribed by the given
// system (little-endian or big-endian), we just typecast it
// it correctly and print.
//
void cm_prt_int(const unsigned char *pItem, cm_item_len len)
{
    switch (len)
    {
        case sizeof(int8_t):
            printf("%d", *((int8_t *)pItem));
            break;

        case sizeof(int16_t):            
            printf("%d", *((int16_t *)pItem));
            break;
            
        case sizeof(int32_t):            
            printf("%d", *((int32_t *)pItem));
            break;

        default:
            assert(0);
    }
}


// signed int
// pItem - pointer to memory to write an integer to.
// len - number of bytes the integer consists of
// val - a string representing the new value
//
// Integers are kept in the order prescribed by the given
// system (little-endian or big-endian).
//
void cm_set_int(unsigned char *pItem, cm_item_len len, string val)
{
    char * pEnd; // pointer to char after chars accepted by strtol

    long int v = strtol(val.c_str(), &pEnd, 0);

    // Just return if v not initialized, i.e. if nothing read/
    // Can I rely on val.c_str returning the same address on
    // subsequent calls?
    if (pEnd == val.c_str())
    {
        cout << "Not an integer." << endl;
    }
   
    switch (len)
    {
        case sizeof(int8_t):
            int8_t cv = (int8_t)v;
            memcpy(pItem, &cv, sizeof(cv));
            break;

        case sizeof(int16_t):            
            int16_t sv = (int16_t)v;
            memcpy(pItem, &sv, sizeof(sv));
            break;
            
        case sizeof(int32_t):
            int32_t lv = (int32_t)v;
            memcpy(pItem, &lv, sizeof(lv));
            break;

        default:
            assert(0);
    }
}
            

// C-style string
// pItem - pointer to memory containing an integer.
// len - number of bytes the string consists of
//
//
void cm_prt_str(const unsigned char *pItem, cm_item_len len)
{
    printf("'%s'", (char *)pItem);
}


// C-style string
// pItem - pointer to memory to write the chars to.
// len - number of bytes the string consists of
// val - a string representing the new value
//
void cm_set_str(unsigned char *pItem, cm_item_len len, string val)
{
    snprintf((char *)pItem, len, val.c_str());
}

