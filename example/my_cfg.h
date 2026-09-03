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
#include "cfg_mgr_descriptor.h"

#define NUM_CLI_PORT      4
#define MAX_LEN_USER_NAME 16
#define MAX_LEN_LOCATION_NAME 16

namespace device
{

namespace user
{

// device::user::tUser
struct tUser
{
    char           name[MAX_LEN_USER_NAME];
    unsigned long  id;
    short          temperature;
    unsigned       elapsed;
};

} // namespace user

namespace home
{

// device::home::tLocation
struct tLocation
{
    char           name[MAX_LEN_LOCATION_NAME];
};

// Contains two optional ways of describing home location:
// simple address or a location structure.
// device::home::tHome
struct tHome
{
    unsigned long * pAddr;
    tLocation     * pLoc;
};

} // namespace home

// device::tDevice
struct tDevice
{
    unsigned long  addr;
    unsigned short cliPort[NUM_CLI_PORT];

    unsigned int   userCount;
    user::tUser  * users;
    home::tHome    home;
};

const cfg_mgr::Descriptor *get_base_descriptor();

} // namespace device

#endif // MY_CFG_H
