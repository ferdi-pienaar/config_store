/// This file contains an optional but useful extension
//  to the config manager: set and print functions for
//  basic data types.  The user can add similar implementations
//  for special types, such as IP or Ethernet addresses.

#include "config_manager_util.h"
#include <assert.h>
#include <limits.h>
#include <stdint.h> // uint8_t, etc

using namespace std;


// The default setdef function, which fills the item with 0 bytes.
void cm_setdef(unsigned char *pItem, cm_item_len len)
{
    memset(pItem, 0, len);
}

// signed int
void cm_prt_int(const unsigned char *pItem, cm_item_len len)
{
    int v;
    
    switch (len)
    {
        case 1: // xxx replace with something from std C file yyy for portability
            v = *((char *)pItem);
            break;

        case 2:            
            v = *((short *)pItem);
            break;
            
        case 4:            
            v = *((int *)pItem);
            break;

        default:
            assert(0);
    }

    printf("%d", v);
}

void cm_set_int(unsigned char *pItem, cm_item_len len, string val)
{
    long int v = strtol(val.c_str(), NULL, 0);

    // xxx just return if v not initialized
    // xxx Sanity check on len
    
    switch (len)
    {
        case 1: // xxx replace with something from std C file yyy
            char cv = (char)v;
            memcpy(pItem, &cv, sizeof(cv));
            break;

        case 2:            
            short sv = (short)v;
            memcpy(pItem, &sv, sizeof(sv));
            break;
            
        case 4:            
            memcpy(pItem, &v, sizeof(v));
            break;

        default:
            assert(0);
    }
}
            

            
