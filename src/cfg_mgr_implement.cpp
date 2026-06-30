#include "cfg_mgr_implement.h"
#include "cfg_mgr_descriptor.h"
#include "cfg_mgr_dbg.h"
#include "store/cfg_mgr_store.h"
#include "cfg_mgr_printf.h"
#include "cfg_mgr_cmd_stack.h"

#include <stdlib.h> // malloc
#include <cstring> // memset
#include <stdint.h> // UINT8_MAX, etc
#include <assert.h>

using namespace std;

namespace cfg_mgr
{

// Array of command handlers, indexed by command ID.
const Config_manager_implement::cmd_handler Config_manager_implement::handlers[] =
{
    &Config_manager_implement::delegate, // CM_ADD
    &Config_manager_implement::delegate, // CM_DEL
    &Config_manager_implement::delegate, // CM_PRT
    &Config_manager_implement::delegate, // CM_PRT_CFG
    &Config_manager_implement::delegate, // CM_SET
    &Config_manager_implement::delegate, // CM_SETDEF
    &Config_manager_implement::load, // CM_LOAD
    &Config_manager_implement::save, // CM_SAVE
    &Config_manager_implement::delegate, // CM_HELP,
    &Config_manager_implement::resetCtxt, // CM_RESET_CTXT
    &Config_manager_implement::delegate, // CM_OP_NONE
    &Config_manager_implement::emptyCmd // CM_EMPTY
};

Config_manager_implement::Config_manager_implement(const Descriptor * desc, Nvram * nvram): m_baseDesc(desc)
{
    m_ramBase = (uint8_t *)malloc(m_baseDesc->getLen());
    DBG_PRT("init: ramBase, %d at %p\n", m_baseDesc->getLen(), m_ramBase);
    assert(m_ramBase != nullptr);
    memset(m_ramBase, 0, m_baseDesc->getLen());
    m_baseDesc->setDefault(m_ramBase);
    resetCtxt(nullptr);
    m_store = Store::createStore(nvram);
}

Config_manager_implement::~Config_manager_implement()
{
    free(m_ramBase);
    delete m_store; // xxx is this clean, is delete the obvious pair to createStore (in Config_manager_implement constructor)?
}

/// Execute command words entered by client (via CLI).
/// @param argc number of entries in the command word array
/// @param argv command word array
void Config_manager_implement::handleCmd(int argc, char *argv[])
{
    Command_stack cmd(argc, argv);
    // Get command handler corresponding to the command and execute it.
    cmd_handler handler = handlers[cmd.getTopOp()];
    return (this->*handler)(&cmd);
}

// Get a prompt string to display to user, representing the current context.
const char * Config_manager_implement::getPromptString() const
{
    return m_currCtxt.getString().c_str();
}

// Pass command that doesn't apply to CM as a whole, to current context for handling.
void Config_manager_implement::delegate(Command_stack * cmd)
{
    // The candidate context starts as a copy of the current context.
    Cmd_context candidateCtxt(m_currCtxt);
    bool updateCtxt = false;
    m_currCtxt.getDesc()->handleCmd(cmd, m_currCtxt.getItem(), &candidateCtxt, updateCtxt);
    if (updateCtxt)
    {
        m_currCtxt = candidateCtxt;
    }
}

// Set context back to base.
void Config_manager_implement::resetCtxt(Command_stack * cmd)
{
    DBG_PRT("resetCtxt\n");
    m_currCtxt = Cmd_context("", m_baseDesc, m_ramBase); // temp context with base properties
}

// Handle empty command stack.
void Config_manager_implement::emptyCmd(Command_stack * cmd)
{
    cm_printf("Enter a command.\n");
}

// Save data in RAM to persistent storage.
void Config_manager_implement::save(Command_stack * cmd)
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
// Resets context, since a reload re-allocates memory and makes current context invalid.
void Config_manager_implement::load(Command_stack *cmd)
{
    m_store->startLoad();

    // Before loading, thus allocating new memory, call setDefault to free owned memory.
    m_baseDesc->setDefault(m_ramBase);
    result_t res = m_baseDesc->startLoad(m_store);
    if (res == CM_SUCCESS)
    {
        res = m_baseDesc->endLoad(m_ramBase, m_store);
    }
    if (res != CM_SUCCESS)
    {
        cm_printf("Load failed error %u: defaults restored.\n", res);
        m_baseDesc->setDefault(m_ramBase);
    }
    m_store->endLoad();
    resetCtxt(nullptr);
}

}
