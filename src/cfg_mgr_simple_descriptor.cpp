#include "cfg_mgr_simple_descriptor.h"
#include "cfg_mgr_cmd_stack.h"
#include "cfg_mgr_cmd_ctxt.h"
#include "cfg_mgr_prt_hexstr.h"
#include "cfg_mgr_dbg.h"
#include "store/cfg_mgr_store.h"
#include "cfg_mgr_printf.h"

#include <cstring> // strcmp

using namespace std;

namespace cfg_mgr
{

// An item does not print its own name, since
// it may be followed by an index, which is known
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


// @param store
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
