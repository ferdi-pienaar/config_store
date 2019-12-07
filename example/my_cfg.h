#ifndef MY_CFG_H
#define MY_CFG_H

// Data structures for managed items, living in RAM.
// The user code does not define instance of these data types -- instead, memory
// allocation is handled by Config_manager.
// The user application accesses this data in two ways:
// 1. From ordinary application code:
//    Read/write access via the getConfig() API to get a pointer that can be
//    typecast to tDevice type: typically done by the GET_DEVICE_CONFIG macro.
//    Read access is for variables that are writeable from the point of view
//    of the configuration API.
//    Write access is for variables that are readable from the point of view
//    of the configuration interface.
// 2. From the configuration interface:
//    Read/write access via Config_manager::handleCmd.
//
#include "config_manager.h"

#define NUM_CLI_PORT      4
#define MAX_LEN_USER_NAME 16
#define MAX_LEN_LOCATION_NAME 16

typedef struct
{
    char           name[MAX_LEN_USER_NAME];
    unsigned long  id;
    short          temperature;
    unsigned       elapsed;

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

const cfg_mgr::Descriptor * get_base_descriptor();

#endif // MY_CFG_H

