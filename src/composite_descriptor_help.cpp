#include "cfg_mgr_composite_descriptor.h"
#include "cfg_mgr_aggregate.h"
#include "cfg_mgr_dbg.h"
#include <cassert>
#include <string>

using namespace std;

namespace cfg_mgr
{

//
void Composite_descriptor::handleHelp(Command_stack * cmd) const
{
    DBG_PRT("%s: op %s\n", __PRETTY_FUNCTION__, Command_stack::OpString(cmd->getTopOp()));

    switch (cmd->getTopOp())
    {
    case Command_stack::CM_OP_NONE:
        // Not a reserved command word, so presumably it identifies a component.
        handleHelpIdWord(cmd);
        return;

    case Command_stack::CM_DEPTH:
        // pop the word 'depth' and process the expected unsigned int to follow.
        helpDepth(&cmd->pop());
        return;

    case Command_stack::CM_EMPTY:
        // No more words, so the command is help for this item. depth=0 means unlimited.
        help("", "", 0);
        return;

    default:
        DBG_PRT("%s: unexpected op %s\n", __PRETTY_FUNCTION__, Command_stack::OpString(cmd->getTopOp()));
    }
}

void Composite_descriptor::handleHelpIdWord(Command_stack * cmd) const
{
    DBG_PRT("%s: component '%s' item '%s'\n", __PRETTY_FUNCTION__, cmd->getTop(), getName());

    const Aggregate * pAggr = getAggr(cmd->getTop()); // Component that is identified by cmd
    if (pAggr == nullptr)
    {
        // Unhandled word(s): not a command, and also doesn't identify a component.
        m_mgr_service->m_print("Invalid: '%s' not in composite '%s'.\n", cmd->getTop(), getName());
        return;
    }

    // The last parsed word identifies a component, so pass the remainder of the command to it.
    pAggr->getData()->pDesc->handleHelp(&cmd->pop());
}

// Extract depth from cmd stack and pass to help().
void Composite_descriptor::helpDepth(Command_stack * cmd) const
{
    unsigned depth = 0;
    if (!cmd->popUnsignedInt(depth))
    {
        m_mgr_service->m_print("Invalid: 'depth' needs a non-negative integer.\n", cmd->getTop(), getName());
        return;
    }
    help("", "", depth);
}

// Give help for each component.
// If depth is 0, iterate to leaf; if 1, print this level and return; else decrement and pass to aggregate.
void Composite_descriptor::help(string prefix, const string &aggr_info, unsigned depth) const
{
    m_mgr_service->m_print("%s%s %slen %u %spersistent\n",
        prefix.c_str(), getName(), aggr_info.c_str(), getLen(), m_data->c.persistent ? "" : "not ");

    if (depth > 0)
    {
        if (--depth == 0)
        {
            return;
        }
    }

    for (unsigned i = 0; i < m_data->aggrCount; i++)
    {
        getAggrAtIndex(i)->help(prefix + "  ", depth);
    }
}

}
