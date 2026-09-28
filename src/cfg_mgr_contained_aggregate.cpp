#include "cfg_mgr_contained_aggregate.h"
#include "cfg_mgr_descriptor.h"
#include "cfg_mgr_cmd_stack.h"
#include "cfg_mgr_cmd_ctxt.h"
#include "cfg_mgr_dbg.h"
#include <stdlib.h> // malloc
#include <cstring> // memset, strcmp, memcpy

using namespace std;

namespace cfg_mgr
{

// Return address of the first item in the item array.
// pParentItem: pointer to parent item; from this the aggregate obtains the
//              address of the first item in the array that it links to the parent.
//
uint8_t * Contained_aggregate::getFirstItem(const uint8_t * pParentItem) const
{
    return (uint8_t *)(pParentItem + getData()->offset);
}

// Return the number of items in the component's array.
// For a contained component, the count is fixed at maxCount.
unsigned Contained_aggregate::getCount(const uint8_t * pParentItem) const
{
    return getData()->maxCount;
}

// Evaluate a command: the parent composite removed the cmd word that identifies this component,
// here we pop the index if necessary and hand over to component descriptor.
bool Contained_aggregate::evalCmd(Command_stack * cmd, uint8_t * pParentItem, Command_stack::eCmOp &op) const
{
    DBG_PRT("%s: item '%s'\n", __PRETTY_FUNCTION__, m_data->pDesc->getName());

    unsigned int itemIdx = 0; // If no index from user is needed, we use offset 0.
    if ((m_data->maxCount > 1) && !getIndex(cmd, itemIdx))
    {
        // There can be more than one item, so we need an explicit index, but operator didn't provide it.
        return false;
    }

    if (itemIdx >= m_data->maxCount)
    {
        m_data->pDesc->m_mgr_service->m_print("Invalid: index %u for contained '%s' > max %u.\n",
                                              itemIdx, m_data->pDesc->getName(), m_data->maxCount-1);
        return false;
    }

    uint8_t *pItem = nullptr;
    if (pParentItem != nullptr)
    {
        // Our parent item is not implicitly added, so this item's memory exists.
        pItem = getItemAtIndex(pParentItem, itemIdx);
    }
    return m_data->pDesc->evalCmd(cmd, pItem, op);
}

bool Contained_aggregate::evalAdd(uint8_t * pItem) const
{
    DBG_PRT("%s: item '%s'\n", __PRETTY_FUNCTION__, getData()->pDesc->getName());
    m_data->pDesc->m_mgr_service->m_print("Invalid: 'add' not supported for contained '%s'.\n", getData()->pDesc->getName());
    return false;
}

// Handle command 'add' on command line
bool Contained_aggregate::handleAdd(uint8_t * pItem) const
{
    return false;
}

// Evaluate command 'del' on command line
bool Contained_aggregate::evalDel(Command_stack * cmd, uint8_t * pItem) const
{
    DBG_PRT("%s: item '%s'\n", __PRETTY_FUNCTION__, getData()->pDesc->getName());
    m_data->pDesc->m_mgr_service->m_print("Invalid: 'del' not supported for contained '%s'.\n", getData()->pDesc->getName());
    return false;
}

// Handle command 'del' on command line
bool Contained_aggregate::handleDel(Command_stack * cmd, uint8_t * pItem) const
{
    return false;
}

//
// From index, return the pointer to component item in this aggregate.
//
// idx: (in) index of wanted component
// pParentItem: (in) the owning item
//
// @return the wanted item, or nullptr
//
uint8_t * Contained_aggregate::getComponentItem(unsigned idx, uint8_t * pParentItem) const
{
    return getItemAtIndex(pParentItem, idx);
}

// Give name, count
void Contained_aggregate::help(const uint8_t * pItem) const
{
    m_data->pDesc->m_mgr_service->m_print("%s [%u]\n", getData()->pDesc->getName(), getCount(pItem));
}

}
