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
#include "cfg_mgr_simple_descriptor.h"
#include "cfg_mgr_composite_descriptor.h"
#include "cfg_mgr_contained_aggregate.h"
#include "cfg_mgr_owned_aggregate.h"
#include "cfg_mgr_set_int.h"
#include "cfg_mgr_prt_int.h"
#include "cfg_mgr_set_str.h"
#include "cfg_mgr_prt_str.h"
#include "cfg_mgr_setdef_null.h"
#include "my_cfg_fn.h" // User-written functions "plugged in" to Config_manager

// We can get the size of a member of a type without having to declare a variable of that type
// Using it means that if we change the type of a simple CONTAINED item
// in the .h file, we don't have to change it here, too.
#define SIZEOF(s,m) ((size_t) sizeof(((s *)0)->m))

using namespace cfg_mgr;

namespace device
{
// Start of composite item device
enum
{
    ID_IPADDR  = 0,
    ID_CLIPORT = 1,
    ID_USERCOUNT = 2,
    ID_USER = 3,
    ID_HOME = 4,

};

////////////////////////////////////////////////////////////////////////////////
// device::ip_address
////////////////////////////////////////////////////////////////////////////////
namespace ip_address
{
const Simple_metadata data =
{
    {
        "ipaddr",
        ID_IPADDR,
        SIZEOF(tDevice, addr),
        true
    },
    cm_set_int,
    cm_setdef_null,
    cm_prt_int
};

const Simple_descriptor desc(&data);
const Aggregate_data aggregate_data = {&desc, 1, offsetof(tDevice, addr)};
const Contained_aggregate aggregate(&aggregate_data);

} // namespace ip_address

////////////////////////////////////////////////////////////////////////////////
// device::port
////////////////////////////////////////////////////////////////////////////////
namespace port
{
const Simple_metadata data =
{
    {
        "port",
        ID_CLIPORT,
        SIZEOF(tDevice, cliPort[0]), // the number of elements in the array are taken into account in the aggregate below
        true
    },
    cm_set_int,
    cm_setdef_null,
    cm_prt_int
};

const Simple_descriptor desc(&data);
const Aggregate_data aggregate_data = {&desc, 1, offsetof(tDevice, cliPort)};
const Contained_aggregate aggregate(&aggregate_data);

} // namespace port

////////////////////////////////////////////////////////////////////////////////
// device::userCnt
////////////////////////////////////////////////////////////////////////////////
namespace userCnt
{
const Simple_metadata data =
{
    {
        "usercnt",
        ID_USERCOUNT,
        SIZEOF(tDevice, userCount),
        false // counter is not persistent
    },
    nullptr, // set: counter, so can't be set
    nullptr, // setdef: no setdef for a counter
    cm_prt_int
};

const Simple_descriptor desc(&data);
const Aggregate_data aggregate_data = {&desc, 1, offsetof(tDevice, userCount)};
const Contained_aggregate aggregate(&aggregate_data);

} // namespace userCnt

namespace user
{
// Start of composite item user
enum
{
    ID_NAME = 0,
    ID_ID = 1,
    ID_TEMP = 2,
    ID_ELAPSED = 3,
};


////////////////////////////////////////////////////////////////////////////////
// device::user::name
////////////////////////////////////////////////////////////////////////////////
namespace name
{
const Simple_metadata data =
{
    {
        "name",
        ID_NAME,
        SIZEOF(tUser, name),
        true
    },
    cm_set_str,
    cm_setdef_null,
    cm_prt_str
};

const Simple_descriptor desc(&data);
const Aggregate_data aggregate_data = {&desc, 1, offsetof(tUser, name)};
const Contained_aggregate aggregate(&aggregate_data);

} // namespace name

////////////////////////////////////////////////////////////////////////////////
// device::user::id
////////////////////////////////////////////////////////////////////////////////
namespace id
{
const Simple_metadata data =
{
    {
        "id",
        ID_ID,
        SIZEOF(tUser, id),
        true
    },
    cm_set_int, // xxx unsigned
    cm_setdef_null,
    cm_prt_int /* xxx unsigned */
};

const Simple_descriptor desc(&data);
const Aggregate_data aggregate_data = {&desc, 1, offsetof(tUser, id)};
const Contained_aggregate aggregate(&aggregate_data);

} // namespace id

////////////////////////////////////////////////////////////////////////////////
// device::user::temp
////////////////////////////////////////////////////////////////////////////////
namespace temp
{
const Simple_metadata data =
{
    {
        "temp",
        ID_TEMP,
        SIZEOF(tUser, temperature),
        true
    },
    cm_set_int,
    setdef_temp,
    cm_prt_int
};

const Simple_descriptor desc(&data);
const Aggregate_data aggregate_data = {&desc, 1, offsetof(tUser, temperature)};
const Contained_aggregate aggregate(&aggregate_data);

} // namespace temp

////////////////////////////////////////////////////////////////////////////////
// device::user::elapsed: statistic, hence non-persistent
////////////////////////////////////////////////////////////////////////////////
namespace elapsed
{
const Simple_metadata data =
{
    {
        "elapsed",
        ID_ELAPSED,
        SIZEOF(tUser, elapsed),
        false
    },
    nullptr, // no set function, because statistics are set by application, not cfg_mgr
    nullptr, // setdef
    cm_prt_int
};

const Simple_descriptor desc(&data);
const Aggregate_data aggregate_data = {&desc, 1, offsetof(tUser, elapsed)};
const Contained_aggregate aggregate(&aggregate_data);

} // namespace elapsed

////////////////////////////////////////////////////////////////////////////////
// device::user
////////////////////////////////////////////////////////////////////////////////

/* List of aggregates in user */
const Aggregate * const aggrList[] =
{
    &name::aggregate,
    &id::aggregate,
    &temp::aggregate,
    &elapsed::aggregate,
};

const Composite_metadata data =
{
    {
        "user",
        ID_USER,
        sizeof(tUser),
        true
    },
    aggrList,
    sizeof(aggrList)/sizeof(aggrList[0])
};

const Composite_descriptor desc(&data);
const Aggregate_data aggregate_data = {&desc, 3 /* xxx max number of users */, offsetof(tDevice, users)};
const Owned_aggregate aggregate(&aggregate_data, &userCnt::aggregate);

} // namespace user

namespace home
{
// Start of composite item home
enum
{
    ID_ADDR = 0,
    ID_LOC = 1,
};


////////////////////////////////////////////////////////////////////////////////
// device::home::addr
////////////////////////////////////////////////////////////////////////////////
namespace addr
{
const Simple_metadata data =
{
    {
        "addr",
        ID_ADDR,
        sizeof(unsigned long), // xxx
        true
    },
    cm_set_int, // xxx set
    cm_setdef_null,
    cm_prt_int
};

const Simple_descriptor desc(&data);
const Aggregate_data aggregate_data = {&desc, 1, offsetof(tHome, pAddr)};
const Owned_aggregate aggregate(&aggregate_data, nullptr); // nullptr => no counter

} // namespace addr

namespace location
{
enum
{
    ID_NAME = 0,
};

////////////////////////////////////////////////////////////////////////////////
// device::home::location::name
////////////////////////////////////////////////////////////////////////////////
namespace name
{
const Simple_metadata data =
{
    {
        "name",
        ID_NAME,
        MAX_LEN_LOCATION_NAME, // xxx
        true
    },
    cm_set_str,
    cm_setdef_null,
    cm_prt_str
};

const Simple_descriptor desc(&data);
const Aggregate_data aggregate_data = {&desc, 1, offsetof(tLocation, name)};
const Contained_aggregate aggregate(&aggregate_data);

} // namespace name

////////////////////////////////////////////////////////////////////////////////
// device::home::location
////////////////////////////////////////////////////////////////////////////////
const Aggregate * const aggrList[] =
{
    &name::aggregate,
};

const Composite_metadata data =
{
    {
        "loc",
        ID_LOC,
        sizeof(tLocation),
        true
    },
    aggrList,
    sizeof(aggrList)/sizeof(aggrList[0])
};

const Composite_descriptor desc(&data);
const Aggregate_data aggregate_data = {&desc, 1, offsetof(tHome, pLoc)};
const Owned_aggregate aggregate(&aggregate_data, nullptr); // nullptr because the owned item has no counter

} // namespace location

////////////////////////////////////////////////////////////////////////////////
// device::home
////////////////////////////////////////////////////////////////////////////////
/* List of aggregates in home */
const Aggregate * const aggrList[] =
{
    &addr::aggregate,
    &location::aggregate,
};

const Composite_metadata data =
{
    {
        "home",
        ID_HOME,
        sizeof(tHome),
        true
    },
    aggrList,
    sizeof(aggrList)/sizeof(aggrList[0])
};

const Composite_descriptor desc(&data);
const Aggregate_data aggregate_data = {&desc, 1, offsetof(tDevice, home)};
const Contained_aggregate aggregate(&aggregate_data);

} // namespace home

////////////////////////////////////////////////////////////////////////////////
// device (top level)
////////////////////////////////////////////////////////////////////////////////
/* List of aggregates in device */
const Aggregate * const aggrList[] =
{
    &ip_address::aggregate,
    &port::aggregate,
    &userCnt::aggregate, // Inserting the counter here is optional: if you do, it is printed with the others
    &user::aggregate,
    &home::aggregate,
};

const Composite_metadata data =
{
    {
        "device",
        0xbabe,
        sizeof(tDevice),
        true
    },
    aggrList,
    sizeof(aggrList)/sizeof(aggrList[0])
};

const Composite_descriptor desc(&data);

const Descriptor * get_base_descriptor()
{
    return &desc;
}

} // namespace device
