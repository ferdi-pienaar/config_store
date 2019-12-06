// Initialization of configurable item descriptors used by my application.
// At initialization, this allocates and populates the meta-data for the
// data declarations in my_cfg1.h.

#include "my_cfg1.h"
#include "config_manager_util.h"
#include "config_manager_setdef_null.h"
#include "my_cfg_fn1.h" // User-written functions "plugged in" to config_manager


// Include the auto-generated configurable item initialization code
#include "cfg.cpp"

const cm_descriptor * get_base_descriptor()
{
    return &device;
}