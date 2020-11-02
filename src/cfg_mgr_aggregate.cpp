/// config manager def
#include "cfg_mgr_aggregate.h"
#include "cfg_mgr_descriptor.h"
#include "cfg_mgr_cmd_stack.h"
#include "cfg_mgr_cmd_ctxt.h"
#include "cfg_mgr_dbg.h"
#include "cfg_mgr_printf.h"
#include "cfg_mgr_store.h"

#include <stdlib.h> // malloc
#include <cstring> // memset, strcmp, memcpy

using namespace std;

namespace cfg_mgr
{

// Utility method to extract in index from an array of command words
// @return false if unable to extract a valid (in-range) index,
//         true if returning a valid (in-range) index.
//
bool Aggregate::getIndex(Command_stack * cmd, unsigned int & itemIdx) const
{
    if (cmd->popIndex(itemIdx)) return true;
    cm_printf("'%s' needs index.\n", m_data->pDesc->getName());
    return false;
}


// Return pointer to item, given parent item and index
uint8_t * Aggregate::getItemAtIndex(const uint8_t * pParentItem, unsigned idx) const
{
    if (idx >= getCount(pParentItem))
    {
        return nullptr;
    }
    return getFirstItem(pParentItem) + idx * m_data->pDesc->getLen();
}


// Set items to default (and free the memory they occupied, if OWNed)
// @param pItem - item this aggregate belongs to
void Aggregate::setDefault(uint8_t * pItem) const
{
    // Set each item to default (thus freeing memory of OWNed sub-components)
    for (unsigned i = 0; i < getCount(pItem); i++)
    {
        m_data->pDesc->setDefault(getItemAtIndex(pItem, i));
    }

    // Free item memory (for OWNed aggregates, no effect on CONTAINed)
    freeItems(pItem);
}


// Print, appending index to prefix (if necessary) and delegating to items
// @param prefix string to be pre-pended to the value, representing its context
void Aggregate::print(const uint8_t * pItem, std::string prefix, bool show_state) const
{
    if (!show_state && !m_data->pDesc->isPersistent())
    {
        // The item is not persistent, i.e. state, so exclude it because not required
        return;
    }

    char indexbuf[6] = {0}; // xxx big enough to avoid truncation in all cases?

    for (unsigned i = 0; i < getCount(pItem); i++)
    {
        if (m_data->maxCount > 1)
        {
            // There can be more than one item, so print the index to distinguish among them
            snprintf(indexbuf, sizeof(indexbuf), " %d", i);
        }
        m_data->pDesc->print(getItemAtIndex(pItem, i),
                             prefix + m_data->pDesc->getName() + indexbuf + " ",
                             show_state);
    }
}


// From remaining command-line words, find component item of this aggregate.
// If the item does not exist, it is created in certain cases.
// First, look in the command words for an integer representing an index.
//
// @param cmd - command string stack
// @param pParentItem: (in) the owning item
// @param ppItem: (out) the wanted item
// @param added: (out) set 'true' if this function allocated memory for the item.
//
// @return true if item is returned, false if no index, or index out of range
//
bool Aggregate::getComponentItem(Command_stack * cmd,
                                 uint8_t * pParentItem,
                                 uint8_t ** ppItem,
                                 bool & added,
                                 Cmd_context * candidateCtxt) const
{
    unsigned int itemIdx = 0; // If no index is needed, we'll use offset 0

    if (m_data->maxCount > 1)
    {
        // There can be more than one instance, so we need an explicit index
        if (!getIndex(cmd, itemIdx))
        {
            // The necessary index was not in the command
            return false;
        }
        // Index is available: add it to the context string
        candidateCtxt->add(itemIdx);
    }

    DBG_PRT("getComponentItem %p offset %d idx %d cnt %d len %d\n",
            *ppItem, m_data->offset, itemIdx, getCount(pParentItem), m_data->pDesc->getLen());

    if (itemIdx >= getCount(pParentItem))
    {
        // We may add a new RAM item, depending on index and aggregate type
        if ((*ppItem = addImplicit(itemIdx, pParentItem)) == nullptr)
        {
            cm_printf("Index %u out of range.\n", itemIdx);
            return false;
        }
        added = true;
    }
    *ppItem = getItemAtIndex(pParentItem, itemIdx);
    candidateCtxt->setDesc(m_data->pDesc);
    candidateCtxt->setItem(*ppItem);
    return true;
}


// Save to persistent storage all elements in the array, if its metadata says it's a persistent item
void Aggregate::save(const uint8_t *pItem, Store * store) const
{
    if (!m_data->pDesc->isPersistent())
    {
        return;
    }

    if (m_data->maxCount > 1)
    {
        store->startWriteArray(m_data->pDesc->getName());
    }
    for (unsigned i = 0; i < getCount(pItem); i++)
    {
        m_data->pDesc->save(getItemAtIndex(pItem, i), store);
    }
    if (m_data->maxCount > 1)
    {
        store->endWriteArray();
    }
}


// Load an item (or array) from persistent storage into RAM, which may be
// allocated (if owned) or retrieved (if contained, thus already allocated).
//
// @param pParentItem - base RAM address where loaded items are stored.
//
// @return CM_SUCCESS if item successfully loaded from store (it may have
//           been saved into RAM or dumped)
//         else an indication of why store load failed
result_t Aggregate::load(uint8_t * pParentItem, Store * store) const
{
    if (!m_data->pDesc->isPersistent())
    {
        return CM_SUCCESS;
    }
    if (m_data->maxCount > 1)
    {
        store->startLoadArray(m_data->pDesc->getName());
    }
    for (unsigned idx = 0; idx < m_data->maxCount; idx++)
    {
        result_t res = loadItem(pParentItem, idx, store);
        if (res == CM_NOT_FOUND)
        {
            // Array in store contains < maxCount. This is normal, so exit this function normally.
            break;
        }
        if (res != CM_SUCCESS)
        {
            // A 'real' error, so exit.
            return res;
        }
    }
    if (m_data->maxCount > 1)
    {
        store->endLoadArray();
    }
    return CM_SUCCESS;
}


// @return true if an index is necessary (when deleting an item on command line).
// If there can be more than 1 item, an index identifies the target item.
bool Aggregate::needIndex(const uint8_t * pParentItem) const
{
    return m_data->maxCount > 1;
}


// Load item from persistent store into RAM.
// @param idx -- the offset in RAM.
result_t Aggregate::loadItem(uint8_t * pParentItem, unsigned idx, Store * store) const
{
    // This fails if there isn't an item in the store to load.
    result_t res = m_data->pDesc->startLoad(store);
    if (res != CM_SUCCESS)
    {
        return res;
    }
    // There is an item in the store, so get RAM for it --
    // in case of an owned aggregate, this allocates memory.
    uint8_t * pItem = getComponentItem(idx, pParentItem);
    if (pItem == nullptr)
    {
        // Memory couldn't be allocated for the item.
        return CM_FAIL;
    }
    // We have memory to load the item into, so complete the load.
    return m_data->pDesc->endLoad(pItem, store);
}

}
