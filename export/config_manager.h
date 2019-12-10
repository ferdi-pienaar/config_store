
#ifndef CFG_MAN_H
#define CFG_MAN_H

#include <stdint.h> // uint8_t, etc
#include <assert.h>
#include <iostream>
#include "config_manager_types.h"
#include "config_manager_command.h"


// xxx throughout I've provisionally avoided the use of references; revise this.

namespace cfg_mgr
{

class Store;

/// Configuration manager, managing all configurable items in the system.
// This class is the application programme's sole point of access to
// the configurable items.
class Config_manager
{
public:
    Config_manager(const Descriptor * pDesc);
    ~Config_manager();
    void handleCmd(int argc, char *argv[]);
    const char * getPromptString() const; ///< get context-dependent prompt string h file
    void * getConfig()
    {
        return (void *)ramBase;
    }

private:
    void save();
    void load();
    void resetCtxt();

    const Descriptor * base_desc;
    uint8_t *    ramBase;
    Cmd_context   currCtxt;      // current command-line context
    Cmd_context   candidateCtxt; // command-line context built while handling current command
    Store * store;
};

}
#endif // CFG_MAN_H


