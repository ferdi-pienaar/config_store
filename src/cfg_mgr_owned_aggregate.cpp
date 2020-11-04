#include "cfg_mgr_owned_aggregate.h"
#include "cfg_mgr_contained_aggregate.h"
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

// Return address of the first item in the item array, or nullptr if there are
// no items allocated.
// pParentItem: pointer to parent item; from this the aggregate obtains the
//              address of the first item in the array that it links to the parent.
//
uint8_t * Owned_aggregate::getFirstItem(const uint8_t * pParentItem) const
{
    return *(uint8_t **)(pParentItem + getData()->offset); // location is a pointer to the OWNED item
}


// Return the number of items in the component's array
// xxx giving a fixed size to counters would simplify this, but
// would restrict the application developer.
unsigned Owned_aggregate::getCount(const uint8_t * pParentItem) const
{
    if (m_counterAggr == nullptr)
    {
        // If there's no counter, then count is just 0 (absence) or 1 (presence)
        return (getFirstItem(pParentItem) == nullptr) ? 0 : 1;
    }

    switch (m_counterAggr->getData()->pDesc->getLen())
    {
    case sizeof(uint8_t):
        return (unsigned)(*(pParentItem + m_counterAggr->getData()->offset));

    case sizeof(uint16_t):
        return (unsigned)(*((uint16_t *)(pParentItem + m_counterAggr->getData()->offset)));

    case sizeof(uint32_t):
        return (unsigned)(*((uint32_t *)(pParentItem + m_counterAggr->getData()->offset)));

    default:
        assert(0);
    }
}


// Set value in RAM that records the number of items in the array of items
// xxx giving a fixed size to counters would simplify this, but
// would restrict the application developer.
void Owned_aggregate::setCount(uint8_t * pParentItem, unsigned int count) const
{
    if (m_counterAggr == nullptr)
    {
        // There is no counter -- it's optional if maxCount == 1
        // xxx what happens if there's no counter but maxCount > 1?
        return;
    }

    DBG_PRT("%s: %s count=%d, %d bytes at %p\n",
            __PRETTY_FUNCTION__, getData()->pDesc->getName(), count,
            m_counterAggr->getData()->pDesc->getLen(), pParentItem + m_counterAggr->getData()->offset);

    assert(count <= getData()->maxCount);

    switch (m_counterAggr->getData()->pDesc->getLen())
    {
    case sizeof(uint8_t):
        assert(count <= UINT8_MAX);
        memcpy(pParentItem + m_counterAggr->getData()->offset, (uint8_t *)&count, sizeof(uint8_t));
        break;

    case sizeof(uint16_t):
        assert(count <= UINT16_MAX);
        memcpy(pParentItem + m_counterAggr->getData()->offset, (uint16_t *)&count, sizeof(uint16_t));
        break;

    case sizeof(uint32_t):
        assert(count <= UINT32_MAX);
        memcpy(pParentItem + m_counterAggr->getData()->offset, (uint32_t *)&count, sizeof(uint32_t));
        break;

    default:
        assert(0);
    }
}


// Free memory of items, if any.  This is called when setting the parent
// item to default -- the default for OWNed components is that there are none.
//
void Owned_aggregate::freeItems(uint8_t * pParentItem) const
{
    uint8_t ** ppItems = (uint8_t **)(pParentItem + getData()->offset);

    if (*ppItems == nullptr)
    {
        // Sanity check: if the pointer is nullptr, there are no items.
        assert(getCount(pParentItem) == 0);
        return;
    }

    // Sanity check: there's a pointer to items, so there are items to free
    assert(getCount(pParentItem) > 0);
    DBG_PRT("freeItems: %u at %p\n", getCount(pParentItem), *ppItems);
    free(*ppItems);
    *ppItems = nullptr;
    setCount(pParentItem, 0);
}


// Handle command 'add' on command line
bool Owned_aggregate::handleAdd(uint8_t * pItem) const
{
    if (getCount(pItem) >= getData()->maxCount)
    {
        cm_printf("Can't add '%s' (max %u).\n", getData()->pDesc->getName(), getData()->maxCount);
        return false;
    }

    if (add(pItem) == nullptr)
    {
        return false;
    }
    return true;
}


// Handle command 'del' on command line
bool Owned_aggregate::handleDel(Command_stack * cmd, uint8_t * pItem) const
{
    unsigned int cnt = getCount(pItem); // number of items currently in array

    DBG_PRT("%s: count=%u\n", __PRETTY_FUNCTION__, cnt);

    if (cnt == 0)
    {
        cm_printf("Currently no '%s'.\n", getData()->pDesc->getName());
        return false;
    }

    unsigned int itemIdx = 0; // If no explicit index is needed, use 0 offset

    if (needIndex(pItem) && !getIndex(&cmd->pop(), itemIdx))
    {
        // An index is needed but couldn't be extracted from the command
        return false;
    }

    DBG_PRT("%s: count=%u itemIdx=%u\n", __PRETTY_FUNCTION__, cnt, itemIdx);

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
// @return pointer to new allocated memory, or nullptr in case of failure
uint8_t * Owned_aggregate::add(uint8_t * pParentItem) const
{
    unsigned cnt = getCount(pParentItem);
    assert(cnt < getData()->maxCount);

    // Reallocate memory, and save pointer in the same location
    uint8_t ** ppItems = (uint8_t **)(pParentItem + getData()->offset);

    DBG_PRT("%s: %d * %d at %p + %d (%p), currently %p\n",
            __PRETTY_FUNCTION__, cnt+1, getData()->pDesc->getLen(), pParentItem, getData()->offset, ppItems, *ppItems);

    uint8_t * pNewMem = (uint8_t *)realloc(*ppItems, (cnt + 1) * getData()->pDesc->getLen());
    if (pNewMem == nullptr)
    {
        cm_printf("No %u for %s\n", getData()->pDesc->getLen(), getData()->pDesc->getName());
        return nullptr;
    }

    // Memory successfully allocated, so reference the (possibly new) memory.
    *ppItems = pNewMem;

    uint8_t * pNewItem = pNewMem + cnt * getData()->pDesc->getLen();

    // Initialize added item with default values. First memset to ensure
    // counters, which have no setDefault fn, are 0 (also sets pointers to owned to nullptr).
    memset(pNewItem, 0, getData()->pDesc->getLen());
    getData()->pDesc->setDefault(pNewItem);

    DBG_PRT("%s: pNewMem=%p\n", __PRETTY_FUNCTION__, pNewMem);

    setCount(pParentItem, cnt + 1);
    return pNewItem;
}


// Del OWNED item.
// This re-allocates the necessary memory, updates the counter if necessary,
// and sets the pointer to the memory to nullptr if it's all been freed.
void Owned_aggregate::del(uint8_t * pParentItem, unsigned int itemIdx) const
{
    uint8_t ** ppItems = (uint8_t **)(pParentItem + getData()->offset);
    item_len_t componentLen = getData()->pDesc->getLen();
    unsigned cnt = getCount(pParentItem);

    assert(*ppItems != nullptr);
    assert(cnt > 0);
    assert(itemIdx < cnt);

    DBG_PRT("del at %p index %d len %d\n", *ppItems, itemIdx, componentLen);

    // Shift down items to occupy the memory vacated by deleted item
    memmove(*ppItems + itemIdx * componentLen,
            *ppItems + (itemIdx + 1) * componentLen,
            (cnt - itemIdx - 1) * componentLen);

    // Reallocate memory, and save pointer in the same location
    *ppItems = (uint8_t *)realloc(*ppItems, (cnt - 1) * componentLen);

    // xxx realloc should return nullptr if memory to be allocated is 0, but it doesn't seem to...
    if (cnt == 1)
    {
        *ppItems = nullptr;
    }
    setCount(pParentItem, cnt - 1);
}


// Implicit add a RAM item, i.e. add an item because it is referenced by
// a command that is not an explicit 'add', or during load from NVRAM.
// This allows the client to re-create
// configuration by "playing back" the output from command "prt".
// @return a pointer to the new item, if the index is one larger than the current
// largest item index, and in-range; else nullptr.
//
uint8_t * Owned_aggregate::addImplicit(unsigned int itemIdx, uint8_t * pParentItem) const
{
    DBG_PRT("%s: %s idx=%d cnt=%d\n",
            __PRETTY_FUNCTION__, getData()->pDesc->getName(), itemIdx, getCount(pParentItem));

    if ((itemIdx == getCount(pParentItem)) && (itemIdx < getData()->maxCount))
    {
        // Index refers to an item to create
        return add(pParentItem);
    }
    return nullptr;
}


// From index, return the pointer to component item in this aggregate.
// Because this function is called during loading, items are created as needed.
//
// idx: (in) index of wanted component, 1 larger than the index previously
//           passed to this method for the same id in the same composite
// pParentItem: (in) the owning item
//
// @return the wanted item, or nullptr
//
uint8_t * Owned_aggregate::getComponentItem(unsigned idx, uint8_t * pParentItem) const
{
    return addImplicit(idx, pParentItem);
}


// Give name, current count, and maxcount.
void Owned_aggregate::help(const uint8_t * pItem) const
{
    cm_printf("%s [%u/%u]\n", getData()->pDesc->getName(), getCount(pItem), getData()->maxCount);
}

}
