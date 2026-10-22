#include "item_identifier_index.h"
#include "cfg_mgr_dbg.h"
#include "cfg_mgr_descriptor.h"
#include "cfg_mgr_owned_aggregate.h"

using namespace std;

namespace cfg_mgr
{
// Eval phase.
// Return index in top cmd word (which we pop), and index not larger than count.
// @return false if index is required but cmd word is not an integer, or if index is out-of-range.
bool Item_identifier_index::get_indexEval(Command_stack *cmd, const Owned_aggregate &aggr,
                                          const uint8_t *pParentItem, unsigned &itemIdx,
                                          unsigned count) const
{
    if ((aggr.getData()->maxCount > 1) && !cmd->popUnsignedInt(itemIdx))
    {
        aggr.getData()->pDesc->m_mgr_service->m_print("Invalid: need index for owned '%s'.\n",
                                                      itemIdx, aggr.getData()->pDesc->getName());
        return false;
    }

    if (itemIdx > count)
    {
        aggr.getData()->pDesc->m_mgr_service->m_print(
            "Invalid: index %u for owned '%s' > max %u.\n", itemIdx,
            aggr.getData()->pDesc->getName(), count);
        return false;
    }
    return true;
}

// Add eval and exec phases.
// Return integer index in top cmd word (which we pop), or itemIdx unchanged if cmd is empty.
// If cmd has a word, but not an integer, return false.
// @param key - not used by Item_identifier_index.
bool Item_identifier_index::get_indexAdd(Command_stack *cmd, const Owned_aggregate &aggr,
                                         const uint8_t *pParentItem, unsigned &itemIdx,
                                         const char *&key) const
{
    // If there is another cmd word, it must be an index. If there isn't, add item at end.
    if ((cmd->getCount() == 1) && !cmd->popUnsignedInt(itemIdx))
    {
        // During exec phase, this shouldn't happen, it is verified during eval.
        aggr.getData()->pDesc->m_mgr_service->m_print("Invalid: not an index.\n");
        return false;
    }
    return true;
}

// Delete eval and exec phases.
// return false if index is needed but not in cmd, else return the index extracted from cmd.
bool Item_identifier_index::get_indexDel(Command_stack *cmd, const Owned_aggregate &aggr,
                                         const uint8_t *pParentItem, unsigned &itemIdx) const
{
    if ((aggr.getData()->maxCount > 1) && !cmd->popUnsignedInt(itemIdx))
    {
        // An index is needed but couldn't be extracted from the command, will not happen in
        // exec phase since it is checked in eval.
        aggr.getData()->pDesc->m_mgr_service->m_print(
            "Invalid: no index provided for item '%s' to delete.\n",
            aggr.getData()->pDesc->getName());
        return false;
    }
    return true;
}

// Identifier is index, not key, so nothing to do.
void Item_identifier_index::set_key(const Owned_aggregate &aggr, uint8_t *pItem,
                                    const char *k) const
{
}

string Item_identifier_index::getPrintPrefixAdd(const Owned_aggregate &aggr, const uint8_t *pItem,
                                                unsigned index) const
{
    return aggr.getPrintPrefixIndex(index);
}

// Exec phase.
string Item_identifier_index::extractId(const Owned_aggregate &aggr, Command_stack *cmd,
                                        const uint8_t *pParentItem, unsigned &itemIdx) const
{
    return aggr.extractIdIndex(cmd, itemIdx);
}

} // namespace cfg_mgr
