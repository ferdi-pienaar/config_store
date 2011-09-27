#ifndef MY_CFG_H
#define MY_CFG_H

#include "config_manager.h"

#define NUM_CLI_PORT      4
#define MAX_LEN_USER_NAME 16
#define MAX_LEN_LOCATION_NAME 16

// Include the auto-generated data structures for configurable items
#include "cfg.h"

void init_config();
t_device * get_config();

#endif // MY_CFG_H

