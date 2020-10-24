
#ifndef CFG_MGR_H
#define CFG_MGR_H

#include <stdint.h> // uint8_t, etc
#include <assert.h>
#include <iostream>
#include "cfg_mgr_types.h"
#include "cfg_mgr_cmd_ctxt.h"


// xxx throughout I've provisionally avoided the use of references; revise this.

namespace cfg_mgr
{

class Store;
class Nvram;

/// Configuration manager, managing all configurable items in the system.
// This class is the application programme's sole point of access to
// the configurable items.
class Config_manager
{
public:
    Config_manager(const Descriptor * pDesc, Nvram * nvram);
    ~Config_manager();
    void handleCmd(int argc, char *argv[]);
    const char * getPromptString() const; ///< get context-dependent prompt string h file
    void * getConfig()
    {
        return (void *)m_ramBase;
    }

private:
    void save();
    void load();
    void resetCtxt();

    const Descriptor * m_baseDesc;
    uint8_t *    m_ramBase;
    Cmd_context   m_currCtxt;      // current command-line context
    Cmd_context   m_candidateCtxt; // command-line context built while handling current command
    Store * m_store;
};

}
#endif // CFG_MGR_H


