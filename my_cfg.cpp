#include <iostream>
#include "my_cfg.h"
#include "config_manager_util.h"
using namespace std;

// Initialization of configurable item descriptors used by my application.
// At initialization, this allocates and populates the meta-data for the
// data declarations in my_cfg.h.


// Allocate item IDs, which are unique within their contexts.
// These are saved to NVRAM.
enum
{
    DEVICE_USER_NAME = 0,
    DEVICE_USER_ID = 1,
    DEVICE_USER_TEMP = 2,
};

enum
{
    DEVICE_IPADDR  = 0,
    DEVICE_CLIPORT = 1,
    DEVICE_USERCOUNT = 2,
    DEVICE_USER = 3,
    
};


cm_simple_item_descriptor ip_address = cm_simple_item_descriptor("ipaddr",
                                                                 DEVICE_IPADDR,
                                                                 sizeof(unsigned long), // xxx define SIZEOF macro
                                                                 cm_set_int,
                                                                 NULL,
                                                                 cm_prt_int);

cm_simple_item_descriptor port = cm_simple_item_descriptor("port", 
                                                            DEVICE_CLIPORT,
                                                            sizeof(unsigned short), // xxx
                                                            cm_set_int,
                                                            NULL,
                                                            cm_prt_int);

cm_simple_item_descriptor userCnt = cm_simple_item_descriptor("usercnt", 
                                                              DEVICE_USERCOUNT,
                                                              sizeof(unsigned int), // xxx
                                                              NULL, // No set fn used for counter
                                                              NULL, // No setdef fn used for counter
                                                              cm_prt_int);

cm_simple_item_descriptor user_name = cm_simple_item_descriptor("name", 
                                                               DEVICE_USER_NAME,
                                                               MAX_LEN_USER_NAME, // xxx
                                                               NULL, // xxx set
                                                               NULL,
                                                               NULL /* xxx prt */);

cm_simple_item_descriptor user_id = cm_simple_item_descriptor("id", 
                                                             DEVICE_USER_ID,
                                                             sizeof(unsigned long), // xxx
                                                             cm_set_int, // xxx unsigned
                                                             NULL,
                                                             cm_prt_int /* xxx unsigned */);

cm_simple_item_descriptor user_temp = cm_simple_item_descriptor("temp", 
                                                               DEVICE_USER_TEMP,
                                                               sizeof(short), // xxx
                                                               cm_set_int, // xxx set
                                                               NULL,
                                                               cm_prt_int);

/* We define this one separately because we want to reference it in two places xxx */
cm_contained_component userCntComp = cm_contained_component(&userCnt, 1, offsetof(tDevice, userCount));

// xxx Does the use of 'new' mean these could not be located in ROM on an embedded device?
cm_component * deviceUserComponentList[] = 
{
    new cm_contained_component(&user_name, 1, offsetof(tUser, name)),
    new cm_contained_component(&user_id, 1, offsetof(tUser, id)),
    new cm_contained_component(&user_temp, 1, offsetof(tUser, temperature)),
};

cm_composite_item_descriptor user = cm_composite_item_descriptor(
    "user", 
    DEVICE_USER,
    sizeof(tUser),
    deviceUserComponentList,
    sizeof(deviceUserComponentList)/sizeof(deviceUserComponentList[0])
);

cm_component * deviceComponentList[] = 
{
    new cm_contained_component(&ip_address, 1, offsetof(tDevice, addr)),
    new cm_contained_component(&port, NUM_CLI_PORT, offsetof(tDevice, cliPort)),
    &userCntComp,
    new cm_owned_component(&user, 3 /* xxx max number of users */, offsetof(tDevice, users), &userCntComp),
};

cm_composite_item_descriptor deviceDesc = cm_composite_item_descriptor(
    "device",
    0xbabe,
    sizeof(tDevice),
    deviceComponentList,
    sizeof(deviceComponentList)/sizeof(deviceComponentList[0]));

// A pointer to the base descriptor: a global used by the application code
// to initialize the config manager xxx
cm_item_descriptor * pBaseDesc = &deviceDesc;

