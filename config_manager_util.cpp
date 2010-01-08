/// This file contains an optional but useful extension
//  to the config manager: set and print functions for
//  basic data types.  The user can add similar implementations
//  for special types, such as IP or Ethernet addresses.

#include "config_manager_util.h"
#include <assert.h>
using namespace std;


// signed int
void cm_prt_int(unsigned char *pItem, cm_item_len len)
{
    int v;
    
    switch (len)
    {
        case 1: // replace with something from std C file yyy
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
            

            
