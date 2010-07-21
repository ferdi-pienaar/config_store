// Data structures for managed items, living in RAM.
// The user code does define instance of these data types.
// The user application accesses this in two ways:
// 1. Read access via the cm_get() API to get a pointer that can be
//    typecast to tDevice type.
// 2. Write access via the cm_do_command interface, to request
//    cfg_man to modify the data.  RAM data is never written to directly
//    (xxx can we enforce this rule?).
// 
#include <iostream>
#include "config_manager.h"
#include "my_cfg_fn.h" // User-written functions "plugged in" to config_manager

#define NUM_CLI_PORT      4
#define MAX_LEN_USER_NAME 16
#define MAX_LEN_LOCATION_NAME 16

typedef struct
{
    char           name[MAX_LEN_USER_NAME];
    unsigned long  id;
    short          temperature;

} tUser;


typedef struct
{
    char           name[MAX_LEN_LOCATION_NAME];
    
} tLocation;


// Contains two optional ways of describing home location:
// simple address or a location structure.
typedef struct
{
    unsigned long * pAddr;
    tLocation     * pLoc;
    
} tHome;


typedef struct
{
    unsigned long  addr;
    unsigned short cliPort[NUM_CLI_PORT];

    unsigned int   userCount;
    tUser        * users;
    tHome          home;
    
} tDevice;


// A pointer to the base descriptor is needed by the code that initializes config_manager
extern const cm_item_descriptor * pBaseDesc;
 
#define GET_DEVICE_CONFIG ((tDevice *)config_manager::getInstance()->getConfig())

