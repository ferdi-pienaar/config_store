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
    DEVICE_IPADDR = 0,
    DEVICE_CLIPORT = 1,
};

cm_simple_item_descriptor ip_address = cm_simple_item_descriptor("ipaddr",
                                                                 DEVICE_IPADDR,
                                                                 sizeof(unsigned long), // xxx
                                                                 NULL,
                                                                 NULL,
                                                                 cm_prt_int);

cm_simple_item_descriptor cliPort = cm_simple_item_descriptor("cliPort", 
                                                              DEVICE_CLIPORT,
                                                              sizeof(unsigned short), // xxx
                                                              NULL,
                                                              NULL,
                                                              cm_prt_int);

cm_component deviceComponentList[] = 
{
    cm_component(cm_component::CONTAINED, &ip_address, 1, offsetof(tDevice, addr)),
    cm_component(cm_component::CONTAINED, &cliPort, NUM_CLI_PORT, offsetof(tDevice, cliPort)),
};

cm_composite_item_descriptor deviceDesc = cm_composite_item_descriptor(
    "device",
    0xbabe,
    deviceComponentList,
    sizeof(deviceComponentList)/sizeof(deviceComponentList[0]));

// A pointer to the base descriptor: a global used by the application code
// to initialize the config manager xxx
cm_item_descriptor * pBaseDesc = &deviceDesc;

