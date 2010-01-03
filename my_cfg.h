#include <iostream>
#include "config_manager.h"

// Data structures for managed items, living in RAM.
// The user code does define instance of these data types.
// The user application accesses this in two ways:
// 1. Read access via the cm_get() API to get a pointer that can be
//    typecast to tDevice type.
// 2. Write access via the cm_do_command interface, to request
//    cfg_man to modify the data.  RAM data is never written to directly
//    (xxx can we enforce this rule?).
// 
typedef struct
{
    unsigned long addr;
    unsigned short cliPort;

} tDevice;


// A pointer to the base descriptor is needed by the code that instantiates cm xxx???
extern cm_item_descriptor * pBaseDesc;
 


