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
#include "my_cfg.h"
#include "config_manager_descriptor.h"
#include "config_manager_aggregate.h"
#include "config_manager_util.h"
#include "config_manager_setdef_null.h"
#include "my_cfg_fn.h" // User-written functions "plugged in" to Config_manager

// We can get the size of a member of a type without having to declare a variable of that type
// Using it means that if we change the type of a simple CONTAINED item
// in the .h file, we don't have to change it here, too.
#define SIZEOF(s,m) ((size_t) sizeof(((s *)0)->m))

using namespace cfg_mgr;

// Start of composite item device
enum
{
    IPADDR  = 0,
    CLIPORT = 1,
    USERCOUNT = 2,
    USER = 3,
    HOME = 4,

};


////////////////////////////////////////////////////////////////////////////////
// ip_addr
////////////////////////////////////////////////////////////////////////////////
Simple_metadata ip_address_d =
{
    {
        "ipaddr",
        IPADDR,
        SIZEOF(tDevice, addr),
        true
    },
    cm_set_int,
    cm_setdef_null,
    cm_prt_int
};

const Simple_descriptor ip_address(&ip_address_d);
const Aggregate_data ipaddressAggr_d = {&ip_address, 1, offsetof(tDevice, addr)};
const Contained_aggregate ipaddressAggr(&ipaddressAggr_d);


////////////////////////////////////////////////////////////////////////////////
// port
////////////////////////////////////////////////////////////////////////////////
const Simple_metadata port_d =
{
    {
        "port",
        CLIPORT,
        SIZEOF(tDevice, cliPort[0]), // the number of elements in the array are taken into account in the aggregate below
        true
    },
    cm_set_int,
    cm_setdef_null,
    cm_prt_int
};

const Simple_descriptor port(&port_d);
const Aggregate_data portAggr_d = {&port, NUM_CLI_PORT, offsetof(tDevice, cliPort)};
const Contained_aggregate portAggr(&portAggr_d);


////////////////////////////////////////////////////////////////////////////////
// userCnt
////////////////////////////////////////////////////////////////////////////////
const Simple_metadata userCnt_d =
{
    {
        "usercnt",
        USERCOUNT,
        SIZEOF(tDevice, userCount),
        false // counter is not persistent
    },
    NULL, // set: counter, so can't be set
    NULL, // setdef: no setdef for a counter
    cm_prt_int
};

// A counter is volatile => last param = false
const Simple_descriptor userCnt(&userCnt_d);
const Aggregate_data userCntAggr_d = {&userCnt, 1, offsetof(tDevice, userCount)};
const Contained_aggregate userCntAggr(&userCntAggr_d);


// Start of composite item user
enum
{
    USER_NAME = 0,
    USER_ID = 1,
    USER_TEMP = 2,
    USER_ELAPSED = 3,
};


////////////////////////////////////////////////////////////////////////////////
// user_name
////////////////////////////////////////////////////////////////////////////////
const Simple_metadata user_name_d =
{
    {
        "name",
        USER_NAME,
        SIZEOF(tUser, name),
        true
    },
    cm_set_str,
    cm_setdef_null,
    cm_prt_str
};

const Simple_descriptor user_name(&user_name_d);
const Aggregate_data userNameAggr_d = {&user_name, 1, offsetof(tUser, name)};
const Contained_aggregate userNameAggr(&userNameAggr_d);


////////////////////////////////////////////////////////////////////////////////
// user_id
////////////////////////////////////////////////////////////////////////////////
const Simple_metadata user_id_d =
{
    {
        "id",
        USER_ID,
        SIZEOF(tUser, id),
        true
    },
    cm_set_int, // xxx unsigned
    cm_setdef_null,
    cm_prt_int /* xxx unsigned */
};

const Simple_descriptor user_id(&user_id_d);
const Aggregate_data userIdAggr_d = {&user_id, 1, offsetof(tUser, id)};
const Contained_aggregate userIdAggr(&userIdAggr_d);


////////////////////////////////////////////////////////////////////////////////
// user_temp
////////////////////////////////////////////////////////////////////////////////
const Simple_metadata user_temp_d =
{
    {
        "temp",
        USER_TEMP,
        SIZEOF(tUser, temperature),
        true
    },
    cm_set_int,
    setdef_temp,
    cm_prt_int
};

const Simple_descriptor user_temp(&user_temp_d);
const Aggregate_data userTempAggr_d = {&user_temp, 1, offsetof(tUser, temperature)};
const Contained_aggregate userTempAggr(&userTempAggr_d);


////////////////////////////////////////////////////////////////////////////////
// user_elapsed: statistic, hence non-persistent
////////////////////////////////////////////////////////////////////////////////
const Simple_metadata user_elapsed_d =
{
    {
        "elapsed",
        USER_ELAPSED,
        SIZEOF(tUser, elapsed),
        false
    },
    NULL, // no set function, because statistics are set by application, not cfg_mgr
    NULL, // setdef
    cm_prt_int
};

const Simple_descriptor user_elapsed(&user_elapsed_d);
const Aggregate_data userElapsedAggr_d = {&user_elapsed, 1, offsetof(tUser, elapsed)};
const Contained_aggregate userElapsedAggr(&userElapsedAggr_d);


////////////////////////////////////////////////////////////////////////////////
// user
////////////////////////////////////////////////////////////////////////////////

/* List of aggregates in user */
const Aggregate * const userAggrList[] =
{
    &userNameAggr,
    &userIdAggr,
    &userTempAggr,
    &userElapsedAggr,
};

const Composite_metadata user_d =
{
    {
        "user",
        USER,
        sizeof(tUser),
        true
    },
    userAggrList,
    sizeof(userAggrList)/sizeof(userAggrList[0])
};

const Composite_descriptor user(&user_d);
const Aggregate_data userAggr_d = {&user, 3 /* xxx max number of users */, offsetof(tDevice, users)};
const Owned_aggregate userAggr(&userAggr_d, &userCntAggr);

// Start of composite item home
enum
{
    HOME_ADDR = 0,
    HOME_LOC = 1,
};


////////////////////////////////////////////////////////////////////////////////
// home_addr
////////////////////////////////////////////////////////////////////////////////
const Simple_metadata home_addr_d =
{
    {
        "addr",
        HOME_ADDR,
        sizeof(unsigned long), // xxx
        true
    },
    cm_set_int, // xxx set
    cm_setdef_null,
    cm_prt_int
};

const Simple_descriptor home_addr(&home_addr_d);
const Aggregate_data homeAddrAggr_d = {&home_addr, 1, offsetof(tHome, pAddr)};
const Owned_aggregate homeAddrAggr(&homeAddrAggr_d, NULL); // NULL => no counter

// Start of composite item home location
enum
{
    HOME_LOC_NAME = 0,
};


////////////////////////////////////////////////////////////////////////////////
// home_loc_name
////////////////////////////////////////////////////////////////////////////////
const Simple_metadata home_loc_name_d =
{
    {
        "name",
        HOME_LOC_NAME,
        MAX_LEN_LOCATION_NAME, // xxx
        true
    },
    cm_set_str,
    cm_setdef_null,
    cm_prt_str
};

const Simple_descriptor home_loc_name(&home_loc_name_d);
const Aggregate_data homeLocNameAggr_d = {&home_loc_name, 1, offsetof(tLocation, name)};
const Contained_aggregate homeLocNameAggr(&homeLocNameAggr_d);


////////////////////////////////////////////////////////////////////////////////
// home_loc
////////////////////////////////////////////////////////////////////////////////
/* List of aggregates in home_loc */
const Aggregate * const homeLocAggrList[] =
{
    &homeLocNameAggr,
};

const Composite_metadata home_loc_d =
{
    {
        "loc",
        HOME_LOC,
        sizeof(tLocation),
        true
    },
    homeLocAggrList,
    sizeof(homeLocAggrList)/sizeof(homeLocAggrList[0])
};

const Composite_descriptor home_loc(&home_loc_d);
const Aggregate_data homeLocAggr_d = {&home_loc, 1, offsetof(tHome, pLoc)};
const Owned_aggregate homeLocAggr(&homeLocAggr_d, NULL); // NULL because the owned item has no counter


////////////////////////////////////////////////////////////////////////////////
// home
////////////////////////////////////////////////////////////////////////////////
/* List of aggregates in home */
const Aggregate * const homeAggrList[] =
{
    &homeAddrAggr,
    &homeLocAggr,
};

const Composite_metadata home_d =
{
    {
        "home",
        HOME,
        sizeof(tHome),
        true
    },
    homeAggrList,
    sizeof(homeAggrList)/sizeof(homeAggrList[0])
};

const Composite_descriptor home(&home_d);
const Aggregate_data homeAggr_d = {&home, 1, offsetof(tDevice, home)};
const Contained_aggregate homeAggr(&homeAggr_d);


////////////////////////////////////////////////////////////////////////////////
// device (top level)
////////////////////////////////////////////////////////////////////////////////
/* List of aggregates in device */
const Aggregate * const deviceAggrList[] =
{
    &ipaddressAggr,
    &portAggr,
    &userCntAggr, // Inserting the counter here is optional: if you do, it is printed with the others
    &userAggr,
    &homeAggr,
};

const Composite_metadata device_d =
{
    {
        "device",
        0xbabe,
        sizeof(tDevice),
        true
    },
    deviceAggrList,
    sizeof(deviceAggrList)/sizeof(deviceAggrList[0])
};

const Composite_descriptor deviceDesc(&device_d);

const Descriptor * get_base_descriptor()
{
    return &deviceDesc;
}
