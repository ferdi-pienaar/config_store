
#pragma once

#include <stdint.h> // uint8_t, etc
#include <iostream>
#include "cfg_mgr_types.h"
#include "cfg_mgr_cmd_ctxt.h"

// xxx throughout I've provisionally avoided the use of references; revise this.

namespace cfg_mgr
{

class Store;
class Nvram_itf;
class Command_stack;

/// Configuration manager, managing all configurable items in the system.
// This class is the application programme's sole point of access to
// the configurable items, that live in RAM allocated by this class.
class Config_manager_implement
{
public:
    Config_manager_implement(const Descriptor * pDesc, uint8_t ** ppRAM, Nvram_itf * nvram);
    ~Config_manager_implement();
    void handleCmd(int argc, char *argv[]);
    const char * getPromptString() const; ///< get context-dependent prompt string h file

private:
    typedef void (Config_manager_implement::*cmd_handler)(Command_stack *cmd);
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
