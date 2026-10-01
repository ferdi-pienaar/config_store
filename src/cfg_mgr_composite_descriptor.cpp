#include "cfg_mgr_composite_descriptor.h"
#include "cfg_mgr_aggregate.h"
#include "cfg_mgr_cmd_stack.h"
#include "cfg_mgr_cmd_ctxt.h"
#include "cfg_mgr_prt_hexstr.h"
#include "cfg_mgr_dbg.h"
#include "store/cfg_mgr_store.h"
#include <assert.h>
#include <cstring> // strcmp

using namespace std;

namespace cfg_mgr
{

// Update this and its components with a reference to services provided by the assigned manager.
void Composite_descriptor::mgrInit(const Mgr_service *serv)
{
    m_mgr_service = serv;
    for (unsigned i = 0; i < m_data->aggrCount; i++)
    {
        getAggrAtIndex(i)->mgrInit(serv);
    }
}

// A composite has content iff any of its components do.
bool Composite_descriptor::hasPersistentContent(const uint8_t *pItem) const
{
    for (unsigned i = 0; i < m_data->aggrCount; i++)
    {
        if (getAggrAtIndex(i)->hasPersistentContent(pItem))
        {
            return true;
        }
    }
    return false;
}

// Evaluate a command to determine if it should be executed.
bool Composite_descriptor::evalCmd(Command_stack * cmd, uint8_t * pItem, Command_stack::eCmOp &op) const
{
    op = cmd->getTopOp();
    DBG_PRT("%s: op %s item '%s' at %p\n", __PRETTY_FUNCTION__, Command_stack::OpString(op), getName(), pItem);

    switch (cmd->getTopOp())
    {
    case Command_stack::CM_OP_NONE:
        return evalIdWord(cmd, pItem, op);

    case Command_stack::CM_ADD:
        // Remove the word 'add' and pass the remainder to the method.
        return evalAdd(&cmd->pop(), pItem);

    case Command_stack::CM_DEL:
        // Remove the word 'del' and pass the remainder to the method.
        return evalDel(&cmd->pop(), pItem);

    case Command_stack::CM_PRT:
    case Command_stack::CM_PRT_CFG:
    case Command_stack::CM_SETDEF:
    case Command_stack::CM_HELP:
    case Command_stack::CM_EMPTY: // context change.
        // Valid in this context.
        return true;

    default:
        DBG_PRT("%s: Invalid operation\n", __PRETTY_FUNCTION__);
        break;
    }
    return false;
}

//
// @param cmd - stack of strings containing name elements
// @param pItem - pointer to RAM where item is located
// @param candidateContext - in/out, candidate new command-line
//        context build up while interpreting cmd stack.
// @param updateCtx - out, true if candidateContext should become
//        the new context.
//
// @return true if command was handled
//
bool Composite_descriptor::handleCmd(Command_stack * cmd,
                                     uint8_t * pItem,
                                     Cmd_context * candidateContext,
                                     bool & updateCtxt) const
{
    DBG_PRT("%s\n op %s", __PRETTY_FUNCTION__, Command_stack::OpString(cmd->getTopOp()));

    assert(pItem != nullptr);

    switch (cmd->getTopOp())
    {
    case Command_stack::CM_OP_NONE:
        return handleIdWord(cmd, pItem, candidateContext, updateCtxt);

    case Command_stack::CM_ADD:
        // Remove the word 'add' and call the component named in the next word.
        return getAggr(cmd->pop().getTop())->handleAdd(pItem);

    case Command_stack::CM_DEL:
        // Remove the word 'del' and call the component named in the next word.
        return getAggr(cmd->pop().getTop())->handleDel(cmd, pItem);

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
        return true;

    case Command_stack::CM_EMPTY:
        // No more words, so the command is a context change to this item.
        updateCtxt = true;
        return true;

    default:
        assert(false && "Invalid operation");
    }
    return false;
}

// Evaluate word in command string that's not a reserved command word,
// hence presumably it identifies a component.
// @pre cmd contains at least one word, but it's not a reserved command word.
bool Composite_descriptor::evalIdWord(Command_stack * cmd, uint8_t * pItem, Command_stack::eCmOp &op) const
{
    DBG_PRT("%s: component '%s' item '%s' at %p\n", __PRETTY_FUNCTION__, cmd->getTop(), getName(), pItem);

    const Aggregate * pAggr = getAggr(cmd->getTop()); // Component that is identified by cmd
    if (pAggr == nullptr)
    {
        // Unhandled word(s): not a command, and also doesn't identify a component.
        m_mgr_service->m_print("Invalid: '%s' not in composite '%s'.\n", cmd->getTop(), getName());
        return false;
    }

    // The last parsed word identifies a component, so pass the remainder of the command to it.
    return pAggr->evalCmd(&cmd->pop(), pItem, op);
}

// Handle word in command string that's not a reserved command word,
// hence presumably it identifies a component.
//
// @param candidateContext - in/out, candidate new command=line
//        context build up while interpreting cmd stack.
// @param updateCtx - out, true if candidateContext should become
//        the new context.
// @return true if a word from cmd was parsed.
// @pre cmd contains at least one word, but it's not a reserved command word.
bool Composite_descriptor::handleIdWord(Command_stack * cmd,
                                        uint8_t * pItem,
                                        Cmd_context * candidateCtxt,
                                        bool & updateCtxt) const
{
    const Aggregate * pAggr = getAggr(cmd->getTop()); // Component that is identified by cmd
    assert(pAggr != nullptr); // cmd checked during eval phase.

    candidateCtxt->addToString(pAggr->getData()->pDesc->getName());

    uint8_t * pComponentItem;  // pointer to component RAM
    pAggr->getComponentItem(&cmd->pop(), pItem, &pComponentItem, candidateCtxt);
    // Pass the remainder of the command to the found component.
    return pAggr->getData()->pDesc->handleCmd(cmd, pComponentItem, candidateCtxt, updateCtxt);
}

// Delegate print command to components
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

// Look for the aggregate whose component has a matching name.
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

/// Save item to persistent storage
void Composite_descriptor::save(const uint8_t *pItem) const
{
    DBG_PRT("%s: %s (%hx)\n", __PRETTY_FUNCTION__, m_data->c.name, m_data->c.id);

    if (!hasPersistentContent(pItem))
    {
        // This composite has no items in RAM, so write nothing.
        return;
    }

    m_mgr_service->m_store->startWriteComposite(m_data);

    for (unsigned i = 0; i < getAggrCount(); i++)
    {
        getAggrAtIndex(i)->save(pItem);
    }
    m_mgr_service->m_store->endWriteComposite();
}

// Prepare to load item from persistent storage -- check if it exists in store.
Result Composite_descriptor::startLoad() const
{
    Result ret = m_mgr_service->m_store->startLoadComposite(m_data);
    DBG_PRT("%s: %s (%hx) res=%s\n", __PRETTY_FUNCTION__, m_data->c.name, m_data->c.id, ResultString(ret));
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
Result Composite_descriptor::endLoad(uint8_t * pItem) const
{
    for (unsigned i = 0; i < getAggrCount(); i++)
    {
        Result ret = getAggrAtIndex(i)->load(pItem);
        if (!((ret == Result::CM_SUCCESS) || (ret == Result::CM_NOT_FOUND)))
        {
            // An unexpected error, such as unexpected end of store
            // or INCOHERENT (L of simple item in store did not match
            // the amount we tried to read).
            return ret;
        }
    }
    DBG_PRT("%s: %s (%hx)\n", __PRETTY_FUNCTION__, m_data->c.name, m_data->c.id);
    return m_mgr_service->m_store->endLoadComposite();
}

// Evaluate add a component named by cmd to a composite.
bool Composite_descriptor::evalAdd(Command_stack * cmd, uint8_t * pItem) const
{
    DBG_PRT("%s: add in '%s' at %p\n", __PRETTY_FUNCTION__, getName(), pItem);

    if (cmd->getCount() != 1)
    {
        m_mgr_service->m_print("Invalid: %u parameters for 'add'.\n", cmd->getCount());
        return false;
    }

    const Aggregate * pAggr = getAggr(cmd->getTop());
    if (pAggr == nullptr)
    {
        m_mgr_service->m_print("Invalid: '%s' not in composite '%s'.\n", cmd->getTop(), getName());
        return false;
    }
    if (pItem == nullptr)
    {
        m_mgr_service->m_print("Invalid: can't add '%s' in non-existent '%s'.\n", cmd->getTop(), getName());
        return false;
    }
    return pAggr->evalAdd(pItem);
}

// Evaluate delete a component named by cmd from this composite.
bool Composite_descriptor::evalDel(Command_stack * cmd, uint8_t * pItem) const
{
    DBG_PRT("%s: del in '%s' at %p\n", __PRETTY_FUNCTION__, getName(), pItem);

    if (!((cmd->getCount() == 1) || (cmd->getCount() == 2)))
    {
        // Should provide item name and, optionally, index.
        m_mgr_service->m_print("Invalid: %u parameters for 'del'.\n", cmd->getCount());
        return false;
    }

    const Aggregate * pAggr = getAggr(cmd->getTop());
    if (pAggr == nullptr)
    {
        m_mgr_service->m_print("Invalid: '%s' not in composite '%s'.\n", cmd->getTop(), getName());
        return false;
    }
    if (pItem == nullptr)
    {
        m_mgr_service->m_print("Invalid: can't remove '%s' from non-existent '%s'.\n", cmd->getTop(), getName());
        return false;
    }
    return pAggr->evalDel(cmd, pItem);
}

}
