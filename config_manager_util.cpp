/// This file contains an optional but useful extension
//  to the config manager: set and print functions for
//  basic data types.  The user can add similar implementations
//  for special types, such as IP or Ethernet addresses.

#include "config_manager_util.h"
#include <assert.h>
using namespace std;


// signed int
void cm_prt_int(unsigned char *pRam, cm_item_len len)
{
    int v;
    
    switch (len)
    {
        case 1: // replace with something from std C file yyy
            v = *((char *)pRam);
            break;

        case 2:            
            v = *((short *)pRam);
            break;
            
        case 4:            
            v = *((int *)pRam);
            break;

        default:
            assert(0);
    }

    printf("%d", v);
}
            

            
