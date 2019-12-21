/// config manager def
#include "config_manager_descriptor.h"
#include "config_manager_aggregate.h"
#include "config_manager_command.h"
#include "config_manager_prt_hexstr.h"
#include "config_manager_dbg.h"
#include "config_manager_store.h"
#include "config_manager_printf.h"

#include <string.h> // memset, strcmp, memcpy

using namespace std;

namespace cfg_mgr
{

////////////////////////////////////////////////////////////////////////////////
//
// Composite_descriptor
//
////////////////////////////////////////////////////////////////////////////////

//
// @param cmd - stack of strings containing name elements
// @param pItem - pointer to RAM where item is located
// @param candidateContext - in/out, candidate new command=line
//        context build up while interpreting cmd stack.
// @param updateCtx - out, true if candidateContext should become
//        the new context.
//
// @return true if command was handled
//
// xxx should free memory allocated as side-effect of a non-set or
//     go-to-node command, not just an invalid command.
//
bool Composite_descriptor::handleCmd(Command_stack * cmd,
                                     uint8_t * pItem,
                                     Cmd_context * candidateContext,
                                     bool & updateCtxt) const
{
    DBG_PRT("composite::handleCmd: %s\n", cmd->getTop());

    assert(pItem != nullptr);

    switch (cmd->getTopOp())
    {
    case Command_stack::CM_OP_NONE:
        return handleIdWord(cmd, pItem, candidateContext, updateCtxt);

    case Command_stack::CM_ADD:
        // Remove the word 'add' and pass the remainder to the method
        return handleAdd(&cmd->pop(), pItem);

    case Command_stack::CM_DEL:
        // Remove the word 'del' and pass the remainder to the method
        return handleDel(&cmd->pop(), pItem);

    case Command_stack::CM_PRT:
        print(pItem, "", true);
        return true;

    case Command_stack::CM_PRT_CFG:
        print(pItem, "", false);
        return true;

    case Command_stack::CM_SETDEF:
        setDefault(pItem);
        return true;

    case Command_stack::CM_HELP:
        help(pItem);
        return true; // xxx true?

    default:
        break;
    }
    cm_printf("Command '%s' not handled in composite item '%s'\n", cmd->getTop(), getName());
    return false;
}


// Handle word in command string that's not a reserved command word,
// hence presumably it identifies a component
//
// @param candidateContext - in/out, candidate new command=line
//        context build up while interpreting cmd stack.
// @param updateCtx - out, true if candidateContext should become
//        the new context.
// @return true if a word was from cmd was parsed
//
bool Composite_descriptor::handleIdWord(Command_stack * cmd,
                                        uint8_t * pItem,
                                        Cmd_context * candidateCtxt,
                                        bool & updateCtxt) const
{
    const Aggregate * pAggr = getAggr(cmd->getTop()); // Component that is identified by cmd
    if (pAggr == nullptr)
    {
        // Unhandled word(s): not a command, and also doesn't identify a component
        cm_printf("'%s' not in composite '%s'.\n", cmd->getTop(), getName());
        return false;
    }

    candidateCtxt->add(pAggr->getData()->pDesc->getName());

    bool      added = false;   // Set true by getComponentItem if it creates a new item.
    uint8_t * pComponentItem;  // pointer to component RAM

    if (!pAggr->getComponentItem(&cmd->pop(), pItem, &pComponentItem, added, candidateCtxt))
    {
        // Index problems are reported by the called fn
        return false;
    }

    // A component was found
    if (cmd->getCount() == 0)
    {
        // We have a component, but there are no more words in the command
        updateCtxt = true;
        return true;
    }

    // Pass the remainder of the command to the found component
    if (!pAggr->getData()->pDesc->handleCmd(cmd, pComponentItem, candidateCtxt, updateCtxt))
    {
        // Component says the command is invalid
        if (added)
        {
            // Free memory allocated by a command that turns out to be invalid
            pAggr->del(pItem, pAggr->getCount(pItem) - 1);
        }
        return false;
    }
    return true;
}


// Try to add a component named by cmd to a composite.
// After verifying the operation is applicable, the item is added.
// @return true if the operation was successful, false if it failed.
bool Composite_descriptor::handleAdd(Command_stack * cmd, uint8_t * pItem) const
{
    DBG_PRT("handleAdd %s\n", cmd->getTop());

    if (cmd->getCount() != 1)
    {
        cm_printf("%u parameters for 'add'.\n", cmd->getCount());
        return false;
    }

    const Aggregate * pAggr = getAggr(cmd->getTop());
    if (pAggr == nullptr)
    {
        cm_printf("'%s' not in composite '%s'.\n", cmd->getTop(), getName());
        return false;
    }
    return pAggr->handleAdd(pItem);
}


// Del an owned component named by cmd from a composite
// @return true if the operation was successful, false if it failed.
bool Composite_descriptor::handleDel(Command_stack * cmd, uint8_t * pItem) const
{
    DBG_PRT("handleDel %s\n", cmd->getTop());

    if ((cmd->getCount() != 1) && (cmd->getCount() != 2))
    {
        // Provide item name and, optionally, index
        cm_printf("%u parameters for 'del'.\n", cmd->getCount());
        return false;
    }

    const Aggregate * pAggr = getAggr(cmd->getTop());

    if (pAggr == nullptr)
    {
        cm_printf("'%s' not in composite '%s'.\n", cmd->getTop(), getName());
        return false;
    }
    return pAggr->handleDel(cmd, pItem);
}


// Delegate print command to components
//
void Composite_descriptor::print(const uint8_t * pItem, string prefix, bool show_state) const
{
    DBG_PRT("print composite %s len %d show_state=%d\n", getName(), getLen(), show_state);

    if (!show_state && !m_data->c.persistent)
    {
        // The item is not persistent, i.e. state, so exclude it because not required
        return;
    }

    for (unsigned i = 0; i < m_data->aggrCount; i++)
    {
        getAggrAtIndex(i)->print(pItem, prefix, show_state);
    }
}


// Delegate setDefault command to components
//
// @pre: item contains valid data, i.e. if an OWNED
// component has no items allocated, the pointer to the items
// is nullptr, so we can know not to try to free them.
//
// For OWNED components, we free owned memory before setting
// the corresponding counter to 0.
//
void Composite_descriptor::setDefault(uint8_t * pItem) const
{
    // Set each component to default
    for (unsigned i = 0; i < m_data->aggrCount; i++)
    {
        getAggrAtIndex(i)->setDefault(pItem);
    }
}


// Give help for each component.
void Composite_descriptor::help(const uint8_t * pItem) const
{
    for (unsigned i = 0; i < m_data->aggrCount; i++)
    {
        getAggrAtIndex(i)->help(pItem);
    }
}


// Look for the aggregate whose component has a matching name
const Aggregate * Composite_descriptor::getAggr(const char * name) const
{
    for (unsigned i = 0; i < m_data->aggrCount; i++)
    {
        if (strcmp(name, getAggrAtIndex(i)->getData()->pDesc->getName()) == 0)
        {
            return getAggrAtIndex(i);
        }
    }
    return nullptr;
}


// Look for the aggregate whose component has a matching ID
// @return aggregate, or nullptr if ID does not identify an aggregate in this context
const Aggregate * Composite_descriptor::getAggr(item_id_t id) const
{
    for (unsigned i = 0; i < m_data->aggrCount; i++)
    {
        if (getAggrAtIndex(i)->getData()->pDesc->getId() == id)
        {
            return getAggrAtIndex(i);
        }
    }
    return nullptr;
}


/// Save item to persistent storage
//
void Composite_descriptor::save(const uint8_t *pItem, Store * store) const
{
    DBG_PRT("%s: %s (%hx)\n", __PRETTY_FUNCTION__, m_data->c.name, m_data->c.id);
    store->startWriteComposite(m_data);

    for (unsigned i = 0; i < getAggrCount(); i++)
    {
        getAggrAtIndex(i)->save(pItem, store);
    }
    store->endWriteComposite();
}


// Prepare to load item from persistent storage -- check if it exists in store.
//
//
result_t Composite_descriptor::startLoad(Store * store) const
{
    result_t ret = store->startLoadComposite(m_data);
    DBG_PRT("%s: %s (%hx) res=%d\n", __PRETTY_FUNCTION__, m_data->c.name, m_data->c.id, ret);
    return ret;
}

// Load item from persistent storage.
// For a composite item, this means loading the components, then closing.
//
// @param pItem (input) - pointer to the RAM memory where loaded values will be saved.
//
// @pre -- this items startLoad was successful, i.e. an unread instance of this
//         remains in the store.
//
result_t Composite_descriptor::endLoad(uint8_t * pItem, Store * store) const
{
    for (unsigned i = 0; i < getAggrCount(); i++)
    {
        result_t ret = getAggrAtIndex(i)->load(pItem, store);
        if (!((ret == CM_SUCCESS) || (ret == CM_NOT_FOUND)))
        {
            // An unexpected error, such as unexpected end of store
            // or INCOHERENT (L of simple item in store did not match
            // the amount we tried to read).
            return ret;
        }
    }
    DBG_PRT("%s: %s (%hx)\n", __PRETTY_FUNCTION__, m_data->c.name, m_data->c.id);
    return store->endLoadComposite();
}

////////////////////////////////////////////////////////////////////////////////
//
// Simple_descriptor
//
////////////////////////////////////////////////////////////////////////////////

// An item does not print its own name, since
// it may be preceded by an index, which is known
// to the item's composite but not to the item.
void Simple_descriptor::print(const uint8_t * pItem, string prefix, bool show_state) const
{
    DBG_PRT("print simple %s len %d at %p show_state=%d\n", getName(), getLen(), pItem, show_state);

    cm_printf("%s= ", prefix.c_str());

    if (m_data->pPrt == nullptr)
    {
        // No function installed so use default print function: hex chars
        cm_printf("%s", cm_prt_hexstr(pItem, getLen()).c_str());
    }
    else
    {
        cm_printf("%s", m_data->pPrt(pItem, getLen()).c_str());
    }
    cm_printf("\n");
}


//
// @param cmd - array of strings containing name elements
// @param pItem - pointer to RAM where item is located
// @param candidateContext - in/out, candidate new command=line
//        context build up while interpreting cmd stack.
// @param updateCtx - out, true if candidateContext should become
//        the new context.
//
bool Simple_descriptor::handleCmd(Command_stack * cmd,
                                  uint8_t * pItem,
                                  Cmd_context *candidateCtxt,
                                  bool & updateCtxt) const
{
    DBG_PRT("simple cmd at %p\n", pItem);

    switch (cmd->getTopOp())
    {
    case Command_stack::CM_PRT:
        print(pItem, "", true);
        return true;

    case Command_stack::CM_PRT_CFG:
        print(pItem, "", false);
        return true;

    case Command_stack::CM_SET:
        if (cmd->pop().getCount() == 1)
        {
            return set(pItem, cmd->getTop());
        }
        break;

    case Command_stack::CM_SETDEF:
        setDefault(pItem);
        return true;

    case Command_stack::CM_HELP:
        help(pItem);
        return true; // true?

    default:
        cm_printf("'%s' not in simple '%s'.\n", cmd->getTop(), getName());
    }
    return false;
}


// Set item to a value input as string on command line
bool Simple_descriptor::set(uint8_t * pItem, string val) const
{
    DBG_PRT("set simple %s at %p to '%s'\n", getName(), pItem, val.c_str());

    if (m_data->pSet != nullptr)
    {
        return m_data->pSet(pItem, getLen(), val);
    }
    cm_printf("'%s' can't be set.\n", getName());
    return false;
}


// Set configurable item to its default value.
void Simple_descriptor::setDefault(uint8_t * pItem) const
{
    if (m_data->pSetDefault != nullptr)
    {
        m_data->pSetDefault(pItem, getLen());
    }
}


void Simple_descriptor::help(const uint8_t * pItem) const
{
    (void)pItem;
    cm_printf("len %u\n", getLen());
}


/// Save item to persistent storage
void Simple_descriptor::save(const uint8_t *pItem, Store * store) const
{
    DBG_PRT("%s: %s (%hx)\n", __PRETTY_FUNCTION__, m_data->c.name, m_data->c.id);
    store->writeSimple(m_data, pItem);
}


// @param pItem
result_t Simple_descriptor::startLoad(Store * store) const
{
    result_t ret = store->startLoadSimple(m_data);
    DBG_PRT("%s: %s (%hx) res=%d\n", __PRETTY_FUNCTION__, m_data->c.name, m_data->c.id, ret);
    return ret;
}

// @param pItem
result_t Simple_descriptor::endLoad(uint8_t * pItem, Store * store) const
{
    result_t ret = store->endLoadSimple(pItem, m_data);
    DBG_PRT("%s: %s (%hx) res=%d\n", __PRETTY_FUNCTION__, m_data->c.name, m_data->c.id, ret);
    return ret;
}

}
