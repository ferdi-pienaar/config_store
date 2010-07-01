// Initialization of configurable item descriptors used by my application.
// At initialization, this allocates and populates the meta-data for the
// data declarations in my_cfg.h.

// xxx all const objects are used in the hope that they will be ROMable.
// From ISO's "Technical Report on C++ Performance":
//  In general, const objects of classes with constructors must be dynamically initialized.
//  However, in some cases compile-time initialization could be performed if static analysis
//  of the constructors resulted in constant values being used. In this case, the object could
//  be ROMable.
//
// It seems to depend on the type of static analysis that the compiler in question is capable
// of doing.  "Thinking in C++ states that objects with user-defined constructors are
// not ROMable, but the above seems to contradict that.
// It may also be that objects with virtual function are not ROMable (where is the vtable and
// when is it initialized?).

// xxx should be a naming convention: camelCase, or underscores, etc.
#include <iostream>
#include "my_cfg.h"
#include "config_manager_util.h"
using namespace std;


/* Start of composite item device */
enum
{
    IPADDR  = 0,
    CLIPORT = 1,
    USERCOUNT = 2,
    USER = 3,
    HOME = 4,
    
};

const cm_basic_item_descriptor ip_address = cm_basic_item_descriptor
(
"ipaddr",
IPADDR,
sizeof(unsigned long), // xxx define SIZEOF macro
cm_set_int,
NULL, // setdef
cm_prt_int
);
                                                                 
const cm_contained_aggregate ipaddressAggr(&ip_address, 1, offsetof(tDevice, addr));

const cm_basic_item_descriptor port = cm_basic_item_descriptor
(
    "port", 
    CLIPORT,
    sizeof(unsigned short), // xxx
    cm_set_int,
    NULL, // setdef
    cm_prt_int
);
                                                            
const cm_contained_aggregate portAggr(&port, NUM_CLI_PORT, offsetof(tDevice, cliPort));

const cm_cntr_item_descriptor userCnt = cm_cntr_item_descriptor
(
    "usercnt", 
    USERCOUNT,
    sizeof(unsigned int), // xxx
    cm_prt_int
);
                                                              
const cm_contained_aggregate userCntAggr = cm_contained_aggregate(&userCnt, 1, offsetof(tDevice, userCount));

/* Start of composite item user */
enum
{
    USER_NAME = 0,
    USER_ID = 1,
    USER_TEMP = 2,
};

const cm_basic_item_descriptor user_name = cm_basic_item_descriptor
(
    "name", 
    USER_NAME,
    MAX_LEN_USER_NAME, // xxx
    cm_set_str,
    NULL, // setdef
    cm_prt_str
);
                                                               
const cm_contained_aggregate userNameAggr(&user_name, 1, offsetof(tUser, name));                                                               

const cm_basic_item_descriptor user_id = cm_basic_item_descriptor
(
    "id", 
    USER_ID,
    sizeof(unsigned long), // xxx
    cm_set_int, // xxx unsigned
    NULL, // setdef
    cm_prt_int /* xxx unsigned */
);
                                                             
const cm_contained_aggregate userIdAggr(&user_id, 1, offsetof(tUser, id));

const cm_basic_item_descriptor user_temp = cm_basic_item_descriptor
(
    "temp", 
    USER_TEMP,
    sizeof(short), // xxx
    cm_set_int, // xxx set
    setdef_temp, // setdef
    cm_prt_int
);
                                                               
const cm_contained_aggregate userTempAggr(&user_temp, 1, offsetof(tUser, temperature));

/* List of aggregates in user */
const cm_aggregate * const userAggrList[] = 
{
    &userNameAggr,
    &userIdAggr,
    &userTempAggr,
};

const cm_composite_item_descriptor user = cm_composite_item_descriptor
(
    "user", 
    USER,
    sizeof(tUser),
    userAggrList,
    sizeof(userAggrList)/sizeof(userAggrList[0])
);                                             

const cm_owned_aggregate userAggr(&user, 3 /* xxx max number of users */, offsetof(tDevice, users), &userCntAggr);

/* Start of composite item home */
enum
{
    HOME_ADDR = 0,
    HOME_LOC = 1,
};

const cm_basic_item_descriptor home_addr = cm_basic_item_descriptor
(
    "addr", 
    HOME_ADDR,
    sizeof(unsigned long), // xxx
    cm_set_int, // xxx set
    NULL, // setdef
    cm_prt_int
);
                                                               
// The last param is NULL because the owned item has no corresponding counter
const cm_owned_aggregate homeAddrAggr(&home_addr, 1, offsetof(tHome, pAddr), NULL);

/* Start of composite item home location */
enum
{
    HOME_LOC_NAME = 0,
};

const cm_basic_item_descriptor home_loc_name = cm_basic_item_descriptor
(
    "name", 
    HOME_LOC_NAME,
    MAX_LEN_LOCATION_NAME, // xxx
    cm_set_str,
    NULL, // setdef
    cm_prt_str
);

const cm_contained_aggregate homeLocNameAggr(&home_loc_name, 1, offsetof(tLocation, name));

/* List of aggregates in home_loc */
const cm_aggregate * const homeLocAggrList[] = 
{
    &homeLocNameAggr,
};

const cm_composite_item_descriptor home_loc = cm_composite_item_descriptor
(
    "loc", 
    HOME_LOC,
    sizeof(tLocation),
    homeLocAggrList,
    sizeof(homeLocAggrList)/sizeof(homeLocAggrList[0])
);

// The last param is NULL because the owned item has no corresponding counter
const cm_owned_aggregate homeLocAggr(&home_loc, 1, offsetof(tHome, pLoc), NULL);

/* List of aggregates in home */
const cm_aggregate * const homeAggrList[] = 
{
    &homeAddrAggr,
    &homeLocAggr,
};

const cm_composite_item_descriptor home = cm_composite_item_descriptor
(
    "home", 
    HOME,
    sizeof(tHome),
    homeAggrList,
    sizeof(homeAggrList)/sizeof(homeAggrList[0])
);

const cm_contained_aggregate homeAggr(&home, 1 , offsetof(tDevice, home));

/* List of aggregates in device */
const cm_aggregate * const deviceAggrList[] = 
{
    &ipaddressAggr,
    &portAggr,
    &userCntAggr, // Inserting the counter here is optional: if you do, it is printed with the others
    &userAggr,
    &homeAggr,
};

const cm_composite_item_descriptor deviceDesc = cm_composite_item_descriptor
(
    "device",
    0xbabe,
    sizeof(tDevice),
    deviceAggrList,
    sizeof(deviceAggrList)/sizeof(deviceAggrList[0])
);

// A pointer to the base descriptor: a global used by the application code
// to initialize the config manager xxx
const cm_item_descriptor * pBaseDesc = &deviceDesc;
