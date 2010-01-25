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

#define NUM_CLI_PORT      4
#define MAX_LEN_USER_NAME 16

typedef struct
{
    char           name[MAX_LEN_USER_NAME];
    unsigned long  id;
    short          temperature;

} tUser;

typedef struct
{
    unsigned long  addr;
    unsigned short cliPort[NUM_CLI_PORT];

    unsigned int   userCount;
    tUser        * users;

    
} tDevice;


// A pointer to the base descriptor is needed by the code that instantiates cm xxx???
extern const cm_item_descriptor * pBaseDesc;
 


