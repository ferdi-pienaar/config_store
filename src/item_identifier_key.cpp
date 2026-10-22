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

static const char *pop_key(Command_stack *cmd, PRINTF_FN_TYPE print, const char *name)
{
    if (cmd->getCount() == 0)
    {
        print("Invalid: missing key for item '%s'.\n", name);
        return nullptr;
    }
    const char *key = cmd->getTop();
    unsigned len = strlen(key);
    if ((len < 3) || (key[0] != '\"') || (key[len-1] != '\"'))
    {
        print("Invalid key len %u for item '%s'.\n", len, name);
        return nullptr;
    }
    return key;
}

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

    const char *key = pop_key(cmd, aggr.getData()->pDesc->m_mgr_service->m_print, aggr.getData()->pDesc->getName());
    if (not key)
    {
        return false;
    }
    cmd->pop();

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
    key = pop_key(cmd, aggr.getData()->pDesc->m_mgr_service->m_print, aggr.getData()->pDesc->getName());
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
    const char *key = pop_key(cmd, aggr.getData()->pDesc->m_mgr_service->m_print, aggr.getData()->pDesc->getName());
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

// k - key string, including opening and closing quotes, which are stripped off by cm_set_str
// before saving it in the key item memory.
void Item_identifier_key::set_key(const Owned_aggregate &aggr, uint8_t *pItem, const char *k) const
{
    DBG_PRT("%s: item '%s' key %s\n", __PRETTY_FUNCTION__, aggr.getData()->pDesc->getName(), k);

    if (k == nullptr)
    {
        // During load from store, key is not available when the item is added.
        return;
    }
    uint8_t *key_item = aggr.m_data->m_keyAggr->getFirstItem(pItem);
    cm_set_str(key_item, aggr.m_data->m_keyAggr->m_data->pDesc->getLen(), k, nullptr);
}

// From pItem, return a prefix addition containing the descriptor name and key of the item.
string Item_identifier_key::getPrintPrefixAdd(const Owned_aggregate &aggr, const uint8_t *pItem,
                                              unsigned index) const
{
    DBG_PRT("%s: item '%s' index %u at %p\n", __PRETTY_FUNCTION__, aggr.getData()->pDesc->getName(),
            index, pItem);

    string p = aggr.getData()->pDesc->getName();
    const uint8_t *key = aggr.m_data->m_keyAggr->getFirstItem(pItem);
    return p + " " + cm_prt_str(key, aggr.m_data->m_keyAggr->m_data->pDesc->getLen()) + " ";
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

// @param key - includes opening and closing quotes.
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
        const char *candidate_key = (char *)aggr.m_data->m_keyAggr->getFirstItem(pItem);

        unsigned len_k = strlen(key) - 2; // remove the quotes
        unsigned len_c = strlen(candidate_key);
        unsigned len = (len_c > len_k) ? len_c : len_k;

        if (strncmp(key + 1, candidate_key, len) == 0)
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
