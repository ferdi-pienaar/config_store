#include "item_identifier_key.h"
#include "cfg_mgr_contained_aggregate.h"
#include "cfg_mgr_dbg.h"
#include "cfg_mgr_descriptor.h"
#include "cfg_mgr_owned_aggregate.h"
#include "cfg_mgr_prt_str.h"
#include "cfg_mgr_set_str.h"
#include <cassert>
#include <cstring> // strcmp

using namespace std;

namespace cfg_mgr
{

// Eval phase.
// Return index that matches key. If there is no match, set index = count.
// Returns false if key is invalid, i.e. not quoted or length 0.
// @pre command is not empty.
bool Item_identifier_key::get_indexEval(Command_stack *cmd, const Owned_aggregate &aggr,
                                        const uint8_t *pParentItem, unsigned &itemIdx,
                                        unsigned count) const
{
    DBG_PRT("%s: item '%s' key %p count %u\n", __PRETTY_FUNCTION__,
            aggr.getData()->pDesc->getName(), aggr.m_data->m_keyAggr, count);

    const char *key = pop_key(cmd, aggr);
    if (not key)
    {
        return false;
    }

    if (pParentItem == nullptr)
    {
        // Implicit add, which means there are no items, so any key is new and thus valid.
        return true;
    }
    find_key(key, aggr, pParentItem, itemIdx, count);
    return true;
}

// Add eval and exec phases.
// Verify key (and pop it from cmd): it must be different from any existing one. The returned index
// is count, because when using keys, the new element is added at the end.
// @param key - output, the key for use in exec phase.
bool Item_identifier_key::get_indexAdd(Command_stack *cmd, const Owned_aggregate &aggr,
                                       const uint8_t *pParentItem, unsigned &itemIdx,
                                       const char *&key) const
{
    DBG_PRT("%s: item '%s' key %p\n", __PRETTY_FUNCTION__, aggr.getData()->pDesc->getName(),
            aggr.m_data->m_keyAggr);

    assert(pParentItem != nullptr);
    key = pop_key(cmd, aggr);
    if (not key)
    {
        return false;
    }

    if (find_key(key, aggr, pParentItem, itemIdx, aggr.getCount(pParentItem)))
    {
        aggr.getData()->pDesc->m_mgr_service->m_print("Invalid: '%s' %s already exists.\n",
                                                      aggr.getData()->pDesc->getName(), key);
        return false;
    }
    // itemIdx=count.
    return true;
}

// Delete eval and exec phases.
// return false if there's no item matching the key, else return true and the corresponding index.
bool Item_identifier_key::get_indexDel(Command_stack *cmd, const Owned_aggregate &aggr,
                                       const uint8_t *pParentItem, unsigned &itemIdx) const
{
    DBG_PRT("%s: item '%s' key %p\n", __PRETTY_FUNCTION__, aggr.getData()->pDesc->getName(),
            aggr.m_data->m_keyAggr);

    assert(pParentItem != nullptr);
    const char *key = pop_key(cmd, aggr);
    if (not key)
    {
        return false;
    }

    if (find_key(key, aggr, pParentItem, itemIdx, aggr.getCount(pParentItem)))
    {
        return true;
    }

    // Does not happen in exec phase, since it is checked in eval phase.
    aggr.getData()->pDesc->m_mgr_service->m_print("Invalid: '%s' %s doesn't exist.\n",
                                                  aggr.getData()->pDesc->getName(), key);
    return false;
}

// k - key in string format.
// Use the key's set function to translate the string format to the in-memory value.
void Item_identifier_key::set_key(const Owned_aggregate &aggr, uint8_t *pItem, const char *key) const
{
    DBG_PRT("%s: item '%s' key %s\n", __PRETTY_FUNCTION__, aggr.getData()->pDesc->getName(), key);

    if (key == nullptr)
    {
        // During load from store, key is not available when the item is added.
        return;
    }
    uint8_t *pKeyItem = aggr.m_data->m_keyAggr->getFirstItem(pItem);
    aggr.m_data->m_keyAggr->getData()->pDesc->set(pKeyItem, key);
}

// From pItem, return a prefix addition containing the descriptor name and key of the item.
string Item_identifier_key::getPrintPrefixAdd(const Owned_aggregate &aggr, const uint8_t *pItem,
                                              unsigned index) const
{
    DBG_PRT("%s: item '%s' index %u at %p\n", __PRETTY_FUNCTION__, aggr.getData()->pDesc->getName(),
            index, pItem);

    string p = aggr.getData()->pDesc->getName();
    const uint8_t *pKeyItem = aggr.m_data->m_keyAggr->getFirstItem(pItem);
    string key = aggr.m_data->m_keyAggr->getData()->pDesc->to_string(pKeyItem);
    return p + " " + key + " ";
}

// Exec phase.
// Pop the key from the cmd stack and return it, and update itemIdx to the index of the item
// with this key. If no existing item has this key, set itemIdx to the next value.
string Item_identifier_key::extractId(const Owned_aggregate &aggr, Command_stack *cmd,
                                      const uint8_t *pParentItem, unsigned &itemIdx) const
{
    const char *key = cmd->getTop();
    cmd->pop();

    find_key(key, aggr, pParentItem, itemIdx, aggr.getCount(pParentItem));
    return key;
}

const char *Item_identifier_key::pop_key(Command_stack *cmd, const Owned_aggregate &aggr) const
{
    if (cmd->getCount() == 0)
    {
        aggr.getData()->pDesc->m_mgr_service->m_print("Invalid: missing key.");
        return nullptr;
    }
    const char *key = cmd->getTop();
    cmd->pop();

    // Use set function's eval function (i.e. with pItem=nullptr) to check if key is valid.
    if (not aggr.m_data->m_keyAggr->getData()->pDesc->set(nullptr, key))
    {
        return nullptr;
    }
    return key;
}

// @param key - string representation of key.
// return true iff key is found in existing items.
// if not found, itemIdx is set to count on return.
bool Item_identifier_key::find_key(const char *key, const Owned_aggregate &aggr,
                                   const uint8_t *pParentItem, unsigned &itemIdx,
                                   unsigned count) const
{
    for (itemIdx = 0; itemIdx < count; ++itemIdx)
    {
        // Get the owned item, and from that get its component, the key.
        const uint8_t *pItem = aggr.getItemAtIndex(pParentItem, itemIdx);
        const uint8_t *pKeyItem = aggr.m_data->m_keyAggr->getFirstItem(pItem);
        string candidate_key = aggr.m_data->m_keyAggr->getData()->pDesc->to_string(pKeyItem);

        if (strcmp(key, candidate_key.c_str()) == 0)
        {
            DBG_PRT("%s: item '%s' match index %u.\n", __PRETTY_FUNCTION__,
                    aggr.getData()->pDesc->getName(), itemIdx);
            return true;
        }
    }
    DBG_PRT("%s: item '%s' no match: itemIdx=count %u.\n", __PRETTY_FUNCTION__,
            aggr.getData()->pDesc->getName(), itemIdx);
    return false;
}

} // namespace cfg_mgr
