/// config manager def
#include "config_manager_aggregate.h"
#include "config_manager_command.h"
#include "config_manager_descriptor.h"
#include "config_manager_util.h"
#include "config_manager_dbg.h"
#include "config_manager_printf.h"

#include <stdlib.h> // malloc
#include <string.h> // memset, strcmp, memcpy
#include <stdint.h> // UINT8_MAX, etc

#include <sstream>
using namespace std;

namespace cfg_mgr
{

////////////////////////////////////////////////////////////////////////////////
//
// Aggregate
//
////////////////////////////////////////////////////////////////////////////////

// Utility method to extract in index from an array of command words
// @return false if unable to extract a valid (in-range) index,
//         true if returning a valid (in-range) index.
//
bool Aggregate::getIndex(Command_stack * cmd, unsigned int & itemIdx) const
{
    if (cmd->getIndex(itemIdx)) return true;
    cm_printf("'%s' needs index.\n", pData->pDesc->getName());
    return false;
}


// Return pointer to item, given parent item and index
uint8_t * Aggregate::getItemAtIndex(const uint8_t * pParentItem, unsigned idx) const
{
    if (idx >= getCount(pParentItem))
    {
        return NULL;
    }
    return getFirstItem(pParentItem) + idx * pData->pDesc->getLen();
}


// Set items to default (and free the memory they occupied, if OWNed)
// @param pItem - item this aggregate belongs to
void Aggregate::setDefault(uint8_t * pItem) const
{
    // Set each item to default (thus freeing memory of OWNed sub-components)
    for (unsigned i = 0; i < getCount(pItem); i++)
    {
        pData->pDesc->setDefault(getItemAtIndex(pItem, i));
    }

    // Free item memory (for OWNed aggregates, no effect on CONTAINed)
    freeItems(pItem);
}


// Print, appending index to prefix (if necessary) and delegating to items
// @param prefix string to be pre-pended to the value, representing its context
void Aggregate::print(const uint8_t * pItem, std::string prefix, bool include_state) const
{
    if (!include_state && !pData->pDesc->isPersistent())
    {
        // The item is not persistent, i.e. state, so exclude it because not required
        return;
    }

    char indexbuf[6] = {0}; // xxx big enough to avoid truncation in all cases?

    for (unsigned i = 0; i < getCount(pItem); i++)
    {
        if (pData->maxCount > 1)
        {
            // There can be more than one item, so print the index to distinguish among them
            snprintf(indexbuf, sizeof(indexbuf), " %d", i);
        }
        pData->pDesc->print(getItemAtIndex(pItem, i),
                            prefix + pData->pDesc->getName() + indexbuf + " ",
                            include_state);
    }
}


// From remaining command-line words, find component item of this aggregate.
// If the item does not exist, it is created in certain cases.
// First, look in the command words for an integer representing an index.
//
// @param cmd - command string stack
// @param pParentItem: (in) the owning item
// @param ppItem: (out) the wanted item
// @param added: (out) did this function allocate memory for the item?
//
// @return true if item is returned, false if no index, or index out of range
//
bool Aggregate::getComponentItem(Command_stack * cmd,
                                 uint8_t * pParentItem,
                                 uint8_t ** ppItem,
                                 bool & added,
                                 Cmd_context * candidateCtxt) const
{
    added = false; // By default, didn't add a new component
    unsigned int itemIdx = 0; // If no index is needed, we'll use offset 0

    if (pData->maxCount > 1)
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
            *ppItem, pData->offset, itemIdx, getCount(pParentItem), pData->pDesc->getLen());

    if (itemIdx >= getCount(pParentItem))
    {
        // We may add a new RAM item, depending on index and aggregate type
        if ((*ppItem = addImplicit(itemIdx, pParentItem)) == NULL)
        {
            cm_printf("Index %u out of range.\n", itemIdx);
            return false;
        }
        added = true;
    }
    *ppItem = getItemAtIndex(pParentItem, itemIdx);
    candidateCtxt->setDesc(pData->pDesc);
    candidateCtxt->setItem(*ppItem);
    return true;
}


// Save to persistent storage all elements in the array, if its metadata says it's a persistent item
void Aggregate::save(const uint8_t *pItem, Store * store) const
{
    if (!pData->pDesc->isPersistent())
    {
        return;
    }
    for (unsigned i = 0; i < getCount(pItem); i++)
    {
        pData->pDesc->save(getItemAtIndex(pItem, i), store);
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
    if (!pData->pDesc->isPersistent())
    {
        return CM_SUCCESS;
    }

    for (unsigned idx = 0; idx < pData->maxCount; idx++)
    {
        // This fails if there isn't an item in the store to load.
        result_t res = pData->pDesc->startLoad(store);
        if (res != CM_SUCCESS)
        {
            return res;
        }

        // There is an item in the store, so get RAM for it --
        // in case of an owned aggregate, this allocates memory.
        uint8_t * pItem = getComponentItem(idx, pParentItem);
        if (pItem == NULL)
        {
            // Memory couldn't be allocated for the item.
            return CM_SUCCESS; //xxxx success??
        }

        // We have memory to load the item into, so complete the load.
        res = pData->pDesc->endLoad(pItem, store);
        if (res != CM_SUCCESS)
        {
            return res;
        }
    }
    return CM_SUCCESS;
}


////////////////////////////////////////////////////////////////////////////////
//
// Contained_aggregate
//
////////////////////////////////////////////////////////////////////////////////

// Return address of the first item in the item array.
// pParentItem: pointer to parent item; from this the aggregate obtains the
//              address of the first item in the array that it links to the parent.
//
uint8_t * Contained_aggregate::getFirstItem(const uint8_t * pParentItem) const
{
    return (uint8_t *)(pParentItem + pData->offset);
}


// Return the number of items in the component's array.
// For a contained component, the count is fixed at maxCount.
unsigned Contained_aggregate::getCount(const uint8_t * pParentItem) const
{
    return pData->maxCount;
}


// Handle command 'add' on command line
bool Contained_aggregate::handleAdd(uint8_t * pItem) const
{
    cm_printf("'add' not supported for contained '%s'.\n", pData->pDesc->getName());
    return false;
}


// Handle command 'del' on command line
bool Contained_aggregate::handleDel(Command_stack * cmd, uint8_t * pItem) const
{
    cm_printf("'del' not supported for contained '%s'.\n", pData->pDesc->getName());
    return false;
}


// Implicit add is not supported for contained components, because add isn't supported
uint8_t * Contained_aggregate::addImplicit(unsigned int itemIdx, uint8_t * pParentItem) const
{
    return NULL;
}


//
// From index, return the pointer to component item in this aggregate.
//
// idx: (in) index of wanted component
// pParentItem: (in) the owning item
//
// @return the wanted item, or NULL
//
uint8_t * Contained_aggregate::getComponentItem(unsigned idx, uint8_t * pParentItem) const
{
    return getItemAtIndex(pParentItem, idx);
}


// Give name, count
void Contained_aggregate::help(const uint8_t * pItem) const
{
    cm_printf("%s [%u]\n", pData->pDesc->getName(), getCount(pItem));
}


////////////////////////////////////////////////////////////////////////////////
//
// Owned_aggregate
//
////////////////////////////////////////////////////////////////////////////////

// Return address of the first item in the item array, or NULL if there are
// no items allocated.
// pParentItem: pointer to parent item; from this the aggregate obtains the
//              address of the first item in the array that it links to the parent.
//
uint8_t * Owned_aggregate::getFirstItem(const uint8_t * pParentItem) const
{
    return *(uint8_t **)(pParentItem + pData->offset); // location is a pointer to the OWNED item
}


// Return the number of items in the component's array
// xxx giving a fixed size to counters would simplify this, but
// would restrict the application developer.
unsigned Owned_aggregate::getCount(const uint8_t * pParentItem) const
{
    if (pCounterAggr == NULL)
    {
        // If there's no counter, then count is just 0 (absence) or 1 (presence)
        return (getFirstItem(pParentItem) == NULL) ? 0 : 1;
    }

    switch (pCounterAggr->pData->pDesc->getLen())
    {
    case sizeof(uint8_t):
        return (unsigned)(*(pParentItem + pCounterAggr->pData->offset));

    case sizeof(uint16_t):
        return (unsigned)(*((uint16_t *)(pParentItem + pCounterAggr->pData->offset)));

    case sizeof(uint32_t):
        return (unsigned)(*((uint32_t *)(pParentItem + pCounterAggr->pData->offset)));

    default:
        assert(0);
    }
}


// Set value in RAM that records the number of items in the array of items
// xxx giving a fixed size to counters would simplify this, but
// would restrict the application developer.
void Owned_aggregate::setCount(uint8_t * pParentItem, unsigned int count) const
{
    if (pCounterAggr == NULL)
    {
        // There is no counter -- it's optional if maxCount == 1
        // xxx what happens if there's no counter but maxCount > 1?
        return;
    }

    DBG_PRT("%s: %s count=%d, %d bytes at %p\n",
            __PRETTY_FUNCTION__, pData->pDesc->getName(), count,
            pCounterAggr->pData->pDesc->getLen(), pParentItem + pCounterAggr->pData->offset);

    assert(count <= pData->maxCount);

    switch (pCounterAggr->pData->pDesc->getLen())
    {
    case sizeof(uint8_t):
    {
        assert(count <= UINT8_MAX);
        uint8_t cnt = count;
        memcpy(pParentItem + pCounterAggr->pData->offset, &cnt, sizeof(cnt));
        break;
    }

    case sizeof(uint16_t):
    {
        assert(count <= UINT16_MAX);
        uint16_t cnt = count;
        memcpy(pParentItem + pCounterAggr->pData->offset, &cnt, sizeof(cnt));
        break;
    }

    case sizeof(uint32_t):
    {
        assert(count <= UINT32_MAX);
        uint32_t cnt = count;
        memcpy(pParentItem + pCounterAggr->pData->offset, &cnt, sizeof(cnt));
        break;
    }

    default:
        assert(0);
    }
}


// Free memory of items, if any.  This is called when setting the parent
// item to default -- the default for OWNed components is that there are none.
//
void Owned_aggregate::freeItems(uint8_t * pParentItem) const
{
    uint8_t ** ppItems = (uint8_t **)(pParentItem + pData->offset);

    if (*ppItems == NULL)
    {
        // Sanity check: if the pointer is NULL, there are no items.
        assert(getCount(pParentItem) == 0);
        return;
    }

    // Sanity check: there's a pointer to items, so there are items to free
    assert(getCount(pParentItem) > 0);
    DBG_PRT("freeItems: %u at %p\n", getCount(pParentItem), *ppItems);
    free(*ppItems);
    *ppItems = NULL;
    setCount(pParentItem, 0);
}


// Handle command 'add' on command line
bool Owned_aggregate::handleAdd(uint8_t * pItem) const
{
    if (getCount(pItem) >= pData->maxCount)
    {
        cm_printf("Can't add '%s' (max %u).\n", pData->pDesc->getName(), pData->maxCount);
        return false;
    }

    if (add(pItem) == NULL)
    {
        return false;
    }
    return true;
}


// Handle command 'del' on command line
bool Owned_aggregate::handleDel(Command_stack * cmd, uint8_t * pItem) const
{
    unsigned int cnt = getCount(pItem); // number of items currently in array

    if (cnt == 0)
    {
        cm_printf("Currently no '%s'.\n", pData->pDesc->getName());
        return false;
    }

    unsigned int itemIdx = 0; // If no explicit index is needed, use 0 offset

    if (needIndex(pItem) && !getIndex(&cmd->pop(), itemIdx))
    {
        // An index is needed but couldn't be extracted from the command
        return false;
    }

    if (itemIdx >= cnt)
    {
        cm_printf("Index %u out of range (0.. %u).\n", itemIdx, cnt-1);
        return false;
    }
    del(pItem, itemIdx);
    return true;
}


// Add OWNED item in RAM.
// @pre Counter is in range
// This allocates memory for the new item, sets it to default values,
// and increments the corresponding counter.
// @return pointer to new allocated memory, or NULL in case of failure
uint8_t * Owned_aggregate::add(uint8_t * pParentItem) const
{
    // Reallocate memory, and save pointer in the same location
    unsigned   cnt     = getCount(pParentItem);
    uint8_t ** ppItems = (uint8_t **)(pParentItem + pData->offset);

    assert(cnt < pData->maxCount);

    DBG_PRT("%s: %d * %d at %p + %d (%p), currently %p\n",
            __PRETTY_FUNCTION__, cnt+1, pData->pDesc->getLen(), pParentItem, pData->offset, ppItems, *ppItems);

    uint8_t * pNewMem = (uint8_t *)realloc(*ppItems, (cnt + 1) * pData->pDesc->getLen());
    if (pNewMem == NULL)
    {
        cm_printf("No %u for %s\n", pData->pDesc->getLen(), pData->pDesc->getName());
        return NULL;
    }

    // Memory successfully allocated, so reference the (possibly new) memory
    *ppItems = pNewMem;

    uint8_t * pNewItem = pNewMem + cnt * pData->pDesc->getLen();

    // Initialize added item with default values. First memset to ensure
    // counters, which have no setDefault fn, are 0 (also sets pointers to owned to NULL).
    memset(pNewItem, 0, pData->pDesc->getLen());
    pData->pDesc->setDefault(pNewItem);

    DBG_PRT("%s: pNewMem=%p\n", __PRETTY_FUNCTION__, pNewMem);

    setCount(pParentItem, cnt + 1);
    return pNewItem;
}


// Del OWNED item.
// This re-allocates the necessary memory, updates the counter if necessary,
// and sets the pointer to the memory to NULL if it's all been freed.
void Owned_aggregate::del(uint8_t * pParentItem, unsigned int itemIdx) const
{
    uint8_t ** ppItems = (uint8_t **)(pParentItem + pData->offset);
    item_len_t componentLen = pData->pDesc->getLen();
    unsigned cnt = getCount(pParentItem);

    assert(*ppItems != NULL);
    assert(cnt > 0);
    assert(itemIdx < cnt);

    DBG_PRT("del at %p index %d len %d\n", *ppItems, itemIdx, componentLen);

    // Shift down items to occupy the memory vacated by deleted item
    memmove(*ppItems + itemIdx * componentLen,
            *ppItems + (itemIdx + 1) * componentLen,
            (cnt - itemIdx - 1) * componentLen);

    // Reallocate memory, and save pointer in the same location
    *ppItems = (uint8_t *)realloc(*ppItems, (cnt - 1) * componentLen);

    // xxx realloc should return NULL if memory to be allocated is 0, but it doesn't seem to...
    if (cnt == 1)
    {
        *ppItems = NULL;
    }
    setCount(pParentItem, cnt - 1);
}


// Implicit add a RAM item, i.e. add an item because it is referenced by
// a command that is not an explicit 'add'. This allows the client to re-create
// configuration by "playing back" the output from command "prt".
// @return a pointer to the new item, if the index is one larger than the current
// largest item index, and in-range; else NULL.
//
uint8_t * Owned_aggregate::addImplicit(unsigned int itemIdx, uint8_t * pParentItem) const
{
    DBG_PRT("%s: %s idx=%d cnt=%d\n",
            __PRETTY_FUNCTION__, pData->pDesc->getName(), itemIdx, getCount(pParentItem));

    if ((itemIdx == getCount(pParentItem)) && (itemIdx < pData->maxCount))
    {
        // Index refers to an item to create
        return add(pParentItem);
    }
    return NULL;
}


// From index, return the pointer to component item in this aggregate.
// Because this function is called during loading, items are created as needed.
//
// idx: (in) index of wanted component, 1 larger than the index previously
//           passed to this method for the same id in the same composite
// pParentItem: (in) the owning item
//
// @return the wanted item, or NULL
//
uint8_t * Owned_aggregate::getComponentItem(unsigned idx, uint8_t * pParentItem) const
{
    return addImplicit(idx, pParentItem);
}


// Give name, current count, and maxcount.
void Owned_aggregate::help(const uint8_t * pItem) const
{
    cm_printf("%s [%u/%u]\n", pData->pDesc->getName(), getCount(pItem), pData->maxCount);
}

}
