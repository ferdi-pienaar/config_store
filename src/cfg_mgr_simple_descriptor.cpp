#include "cfg_mgr_simple_descriptor.h"
#include "cfg_mgr_cmd_stack.h"
#include "cfg_mgr_cmd_ctxt.h"
#include "cfg_mgr_prt_hexstr.h"
#include "cfg_mgr_dbg.h"
#include "store/cfg_mgr_store.h"
#include <cassert>

using namespace std;

namespace cfg_mgr
{

// An item does not print its own name, since it may be followed by an index, which is known
// to the item's composite but not to the item.
void Simple_descriptor::print(const uint8_t * pItem, string prefix, bool show_state) const
{
    DBG_PRT("print simple %s len %d at %p show_state=%d\n", getName(), getLen(), pItem, show_state);

    m_mgr_service->m_print("%s= ", prefix.c_str());

    if (m_data->pPrt == nullptr)
    {
        // No function installed so use default print function: hex chars
        m_mgr_service->m_print("%s", cm_prt_hexstr(pItem, getLen()).c_str());
    }
    else
    {
        m_mgr_service->m_print("%s", m_data->pPrt(pItem, getLen()).c_str());
    }
    m_mgr_service->m_print("\n");
}

// Evaluate cmd for simple item.
bool Simple_descriptor::evalCmd(Command_stack * cmd, uint8_t * pItem, Command_stack::eCmOp &op) const
{
    op = cmd->getTopOp();
    DBG_PRT("%s: item '%s' op %s (%u) at %p\n", __PRETTY_FUNCTION__, getName(), Command_stack::OpString(op), op, pItem);

    switch (op)
    {
    case Command_stack::CM_PRT:
    case Command_stack::CM_PRT_CFG:
    case Command_stack::CM_SETDEF:
    case Command_stack::CM_HELP:
    case Command_stack::CM_EMPTY: // No futher words: context change.
        // These commands succeed on a simple item.
        return true;

    case Command_stack::CM_SET:
        if (cmd->pop().getCount() != 1)
        {
            m_mgr_service->m_print("Invalid: give exactly one value for item '%s'.\n", getName());
            return false;
        }
        return evalSet(pItem, cmd->getTop());

    default:
        DBG_PRT("%s: Invalid operation\n", __PRETTY_FUNCTION__);
        break;
    }
    return false;
}

//
// @param cmd - array of strings containing name elements
// @param pItem - pointer to RAM where item is located
// @param candidateContext - in/out, candidate new command=line
//        context build up while interpreting cmd stack.
// @param updateCtx - out, true if candidateContext should become
//        the new context.
bool Simple_descriptor::handleCmd(Command_stack * cmd,
                                  uint8_t * pItem,
                                  Cmd_context *candidateCtxt,
                                  bool & updateCtxt) const
{
    DBG_PRT("%s: item %p\n", __PRETTY_FUNCTION__, pItem);

    switch (cmd->getTopOp())
    {
    case Command_stack::CM_PRT:
        print(pItem, "", true);
        return true;

    case Command_stack::CM_PRT_CFG:
        print(pItem, "", false);
        return true;

    case Command_stack::CM_SET:
        return set(pItem, cmd->pop().getTop());

    case Command_stack::CM_SETDEF:
        setDefault(pItem);
        return true;

    case Command_stack::CM_HELP:
        help(pItem);
        return true;

    case Command_stack::CM_EMPTY: // No futher words: context change.
        updateCtxt = true;
        return true;

    default:
        assert(false && "Invalid operation");
    }
    return false;
}

// Evaluate set item to a value input as string on command line.
bool Simple_descriptor::evalSet(uint8_t * pItem, string val) const
{
    DBG_PRT("%s: item '%s' at %p to '%s'\n", __PRETTY_FUNCTION__, getName(), pItem, val.c_str());

    if (m_data->pSet != nullptr)
    {
        // Call with item=nullptr, i.e. evaluate only.
        return m_data->pSet(nullptr, getLen(), val, m_mgr_service->m_print);
    }
    // No set function provided.
    m_mgr_service->m_print("Invalid: '%s' can't be set.\n", getName());
    return false;
}

// Set item to a value input as string on command line.
bool Simple_descriptor::set(uint8_t * pItem, string val) const
{
    DBG_PRT("set simple %s at %p to '%s'\n", getName(), pItem, val.c_str());

    if (m_data->pSet != nullptr)
    {
        return m_data->pSet(pItem, getLen(), val, m_mgr_service->m_print);
    }
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
    m_mgr_service->m_print("len %u\n", getLen());
}

/// Save item to persistent storage
void Simple_descriptor::save(const uint8_t *pItem) const
{
    DBG_PRT("%s: %s (%hx)\n", __PRETTY_FUNCTION__, m_data->c.name, m_data->c.id);
    m_mgr_service->m_store->writeSimple(m_data, pItem);
}

Result Simple_descriptor::startLoad() const
{
    Result ret = m_mgr_service->m_store->startLoadSimple(m_data);
    DBG_PRT("%s: %s (%hx) res=%s\n", __PRETTY_FUNCTION__, m_data->c.name, m_data->c.id, ResultString(ret));
    return ret;
}

// @param pItem
Result Simple_descriptor::endLoad(uint8_t * pItem) const
{
    Result ret = m_mgr_service->m_store->endLoadSimple(pItem, m_data);
    DBG_PRT("%s: %s (%hx) res=%s\n", __PRETTY_FUNCTION__, m_data->c.name, m_data->c.id, ResultString(ret));
    return ret;
}

}
