
#ifndef CFG_MGR_H
#define CFG_MGR_H

#include <stdint.h> // uint8_t, etc
#include <iostream>
#include "cfg_mgr_types.h"
#include "cfg_mgr_cmd_ctxt.h"

// xxx throughout I've provisionally avoided the use of references; revise this.

namespace cfg_mgr
{

class Store;
class Nvram;
class Command_stack;

/// Configuration manager, managing all configurable items in the system.
// This class is the application programme's sole point of access to
// the configurable items, that live in RAM allocated by this class.
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
    typedef void (Config_manager::*cmd_handler)(Command_stack *cmd);
    void delegate(Command_stack *cmd);
    void resetCtxt(Command_stack *cmd);
    void emptyCmd(Command_stack *cmd);
    void save(Command_stack *cmd);
    void load(Command_stack *cmd);

    const Descriptor * m_baseDesc;
    uint8_t *    m_ramBase;
    Cmd_context   m_currCtxt;      // current command-line context
    Store * m_store;
    static const cmd_handler handlers[];
};

}
#endif // CFG_MGR_H
