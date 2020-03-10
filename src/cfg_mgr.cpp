/// config manager def
#include "cfg_mgr.h"
#include "cfg_mgr_descriptor.h"
#include "cfg_mgr_dbg.h"
#include "cfg_mgr_store.h"
#include "cfg_mgr_printf.h"

#include <stdlib.h> // malloc
#include <cstring> // memset, strcmp, memcpy
#include <stdint.h> // UINT8_MAX, etc

#include <sstream>
using namespace std;

namespace cfg_mgr
{

////////////////////////////////////////////////////////////////////////////////
//
// Config_manager
//
////////////////////////////////////////////////////////////////////////////////

Config_manager::Config_manager(const Descriptor * desc, Nvram * nvram): m_baseDesc(desc)
{
    m_ramBase = (uint8_t *)malloc(m_baseDesc->getLen());
    DBG_PRT("init: ramBase, %d at %p\n", m_baseDesc->getLen(), m_ramBase);
    assert(m_ramBase != nullptr);
    memset(m_ramBase, 0, m_baseDesc->getLen());
    m_baseDesc->setDefault(m_ramBase);
    resetCtxt();
    m_store = Store::createStore(nvram);
}

Config_manager::~Config_manager()
{
    free(m_ramBase);
    delete m_store; // xxx is this clean, is delete the obvious pair to getStore (in Config_manager constructor)?
}

/// Execute command words entered by client on CLI cpp file
/// @param argc number of command words
/// @param argv command word array
void Config_manager::handleCmd(int argc, char *argv[])
{
    if (argc <= 0)
    {
        return;
    }
    Command_stack cmd(argc, argv);
    bool updateCtxt = false;

    // First treat the commands that are only applicable at the top level
    switch (cmd.getTopOp())
    {
    case Command_stack::CM_LOAD:
        return load();

    case Command_stack::CM_SAVE:
        return save();

    case Command_stack::CM_RESET_CTXT:
        return resetCtxt();

    default: // Other commands are passed to current context
        break;
    }

    // The candidate context starts as a copy of the current context
    m_candidateCtxt = m_currCtxt;

    // Pass command that doesn't apply to CM as a whole, to current context for handling
    m_currCtxt.getDesc()->handleCmd(&cmd, m_currCtxt.getItem(), &m_candidateCtxt, updateCtxt);
    if (updateCtxt)
    {
        m_currCtxt = m_candidateCtxt;
    }
}


// Set context back to base
void Config_manager::resetCtxt()
{
    DBG_PRT("resetCtxt\n");
    m_currCtxt = Cmd_context("", m_baseDesc, m_ramBase); // temp context with base properties
}


// Get a prompt string to display to user, representing the current context
const char * Config_manager::getPromptString() const
{
    return m_currCtxt.getString().c_str();
}


// Save data in RAM to persistent storage
void Config_manager::save()
{
    if (!m_baseDesc->isPersistent())
    {
        return;
    }
    m_store->startWrite();
    m_baseDesc->save(m_ramBase, m_store);
    m_store->endWrite();
}


// Load data in persistent storage, to configurable items in RAM.
// Resets context, since a reload re-allocates memory and makes current context invalid
void Config_manager::load()
{
    m_store->startLoad();

    // Before loading, thus allocating new memory, call setDefault to free owned memory
    m_baseDesc->setDefault(m_ramBase);
    result_t res = m_baseDesc->startLoad(m_store);
    if (res == CM_SUCCESS)
    {
        res = m_baseDesc->endLoad(m_ramBase, m_store);
    }
    if (res != CM_SUCCESS)
    {
        cm_printf("Load failed: defaults restored.\n");
        m_baseDesc->setDefault(m_ramBase);
    }
    m_store->endLoad();
    resetCtxt();
}

}
