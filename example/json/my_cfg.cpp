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
#include "../my_cfg.h"
#include "cfg_mgr_simple_descriptor.h"
#include "cfg_mgr_composite_descriptor.h"
#include "cfg_mgr_contained_aggregate.h"
#include "cfg_mgr_owned_aggregate.h"
#include "cfg_mgr_set_int.h"
#include "cfg_mgr_prt_int.h"
#include "cfg_mgr_set_str.h"
#include "cfg_mgr_prt_str.h"
#include "cfg_mgr_setdef_null.h"
#include "../my_cfg_fn.h" // User-written functions "plugged in" to Config_manager

// We can get the size of a member of a type without having to declare a variable of that type
// Using it means that if we change the type of a simple CONTAINED item
// in the .h file, we don't have to change it here, too.
#define SIZEOF(s,m) ((size_t) sizeof(((s *)0)->m))

using namespace cfg_mgr;

namespace device
{
// Start of composite item device

////////////////////////////////////////////////////////////////////////////////
// device::ip_address
////////////////////////////////////////////////////////////////////////////////
namespace ip_address
{
const Simple_metadata data =
{
    {
        "ipaddr",
        PERSISTENT,
        SIZEOF(tDevice, addr),
    },
    cm_set_int,
    cm_setdef_null,
    cm_prt_int
};

Simple_descriptor desc(&data);
const Aggregate_data aggregate_data = {&desc, 1, offsetof(tDevice, addr)};
Contained_aggregate aggregate(&aggregate_data);

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
        PERSISTENT,
        SIZEOF(tDevice, cliPort[0]), // the number of elements in the array are taken into account in the aggregate below
    },
    cm_set_int,
    cm_setdef_null,
    cm_prt_int
};

Simple_descriptor desc(&data);
const Aggregate_data aggregate_data = {&desc, NUM_CLI_PORT, offsetof(tDevice, cliPort)};
Contained_aggregate aggregate(&aggregate_data);

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
        NOT_PERSISTENT,
        SIZEOF(tDevice, userCount),
    },
    nullptr, // set: counter, so can't be set
    nullptr, // setdef: no setdef for a counter
    cm_prt_int
};

Simple_descriptor desc(&data);
const Aggregate_data aggregate_data = {&desc, 1, offsetof(tDevice, userCount)};
Contained_aggregate aggregate(&aggregate_data);

} // namespace userCnt

namespace user
{
// Start of composite item user

////////////////////////////////////////////////////////////////////////////////
// device::user::name
////////////////////////////////////////////////////////////////////////////////
namespace name
{
const Simple_metadata data =
{
    {
        "name",
        PERSISTENT,
        SIZEOF(tUser, name),
    },
    cm_set_str,
    cm_setdef_null,
    cm_prt_str
};

Simple_descriptor desc(&data);
const Aggregate_data aggregate_data = {&desc, 1, offsetof(tUser, name)};
Contained_aggregate aggregate(&aggregate_data);

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
        PERSISTENT,
        SIZEOF(tUser, id),
    },
    cm_set_int, // xxx unsigned
    cm_setdef_null,
    cm_prt_int /* xxx unsigned */
};

Simple_descriptor desc(&data);
const Aggregate_data aggregate_data = {&desc, 1, offsetof(tUser, id)};
Contained_aggregate aggregate(&aggregate_data);

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
        PERSISTENT,
        SIZEOF(tUser, temperature),
    },
    cm_set_int,
    setdef_temp,
    cm_prt_int
};

Simple_descriptor desc(&data);
const Aggregate_data aggregate_data = {&desc, 1, offsetof(tUser, temperature)};
Contained_aggregate aggregate(&aggregate_data);

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
        NOT_PERSISTENT,
        SIZEOF(tUser, elapsed),
    },
    nullptr, // no set function, because statistics are set by application, not cfg_mgr
    nullptr, // setdef
    cm_prt_int
};

Simple_descriptor desc(&data);
const Aggregate_data aggregate_data = {&desc, 1, offsetof(tUser, elapsed)};
Contained_aggregate aggregate(&aggregate_data);

} // namespace elapsed

////////////////////////////////////////////////////////////////////////////////
// device::user
////////////////////////////////////////////////////////////////////////////////

/* List of aggregates in user */
Aggregate * const aggrList[] =
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
        PERSISTENT,
        sizeof(tUser),
    },
    aggrList,
    sizeof(aggrList)/sizeof(aggrList[0])
};

Composite_descriptor desc(&data);
const Owned_aggregate_data aggregate_data = 
{
    {
        &desc,
        3 /* xxx max number of users */,
        offsetof(tDevice, users)
    },
    &userCnt::aggregate
};

Owned_aggregate aggregate(&aggregate_data);

} // namespace user

namespace home
{
// Start of composite item home

////////////////////////////////////////////////////////////////////////////////
// device::home::addr
////////////////////////////////////////////////////////////////////////////////
namespace addr
{
const Simple_metadata data =
{
    {
        "addr",
        PERSISTENT,
        sizeof(unsigned long), // xxx
    },
    cm_set_int, // xxx set
    cm_setdef_null,
    cm_prt_int
};

Simple_descriptor desc(&data);
const Owned_aggregate_data aggregate_data = 
{
    {
        &desc,
        1,
        offsetof(tHome, pAddr)
    },
    nullptr // no counter
};
Owned_aggregate aggregate(&aggregate_data); // nullptr => no counter

} // namespace addr

namespace location
{
////////////////////////////////////////////////////////////////////////////////
// device::home::location::name
////////////////////////////////////////////////////////////////////////////////
namespace name
{
const Simple_metadata data =
{
    {
        "name",
        PERSISTENT,
        MAX_LEN_LOCATION_NAME, // xxx
    },
    cm_set_str,
    cm_setdef_null,
    cm_prt_str
};

Simple_descriptor desc(&data);
const Aggregate_data aggregate_data = {&desc, 1, offsetof(tLocation, name)};
Contained_aggregate aggregate(&aggregate_data);

} // namespace name

////////////////////////////////////////////////////////////////////////////////
// device::home::location
////////////////////////////////////////////////////////////////////////////////
Aggregate * const aggrList[] =
{
    &name::aggregate,
};

const Composite_metadata data =
{
    {
        "loc",
        PERSISTENT,
        sizeof(tLocation),
    },
    aggrList,
    sizeof(aggrList)/sizeof(aggrList[0])
};

Composite_descriptor desc(&data);
const Owned_aggregate_data aggregate_data =
{
    {
        &desc,
        1,
        offsetof(tHome, pLoc)
    },
    nullptr // the owned item has no counter
};
Owned_aggregate aggregate(&aggregate_data);

} // namespace location

////////////////////////////////////////////////////////////////////////////////
// device::home
////////////////////////////////////////////////////////////////////////////////
/* List of aggregates in home */
Aggregate * const aggrList[] =
{
    &addr::aggregate,
    &location::aggregate,
};

const Composite_metadata data =
{
    {
        "home",
        PERSISTENT,
        sizeof(tHome),
    },
    aggrList,
    sizeof(aggrList)/sizeof(aggrList[0])
};

Composite_descriptor desc(&data);
const Aggregate_data aggregate_data = {&desc, 1, offsetof(tDevice, home)};
Contained_aggregate aggregate(&aggregate_data);

} // namespace home

////////////////////////////////////////////////////////////////////////////////
// device (top level)
////////////////////////////////////////////////////////////////////////////////
/* List of aggregates in device */
Aggregate * const aggrList[] =
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
        PERSISTENT,
        sizeof(tDevice),
    },
    aggrList,
    sizeof(aggrList)/sizeof(aggrList[0])
};

Composite_descriptor desc(&data);

Descriptor * get_base_descriptor()
{
    return &desc;
}

} // namespace device
