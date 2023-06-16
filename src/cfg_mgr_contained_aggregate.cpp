#include "cfg_mgr_contained_aggregate.h"
#include "cfg_mgr_descriptor.h"
#include "cfg_mgr_cmd_stack.h"
#include "cfg_mgr_cmd_ctxt.h"
#include "cfg_mgr_dbg.h"
#include "cfg_mgr_printf.h"
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


// Handle command 'add' on command line
bool Contained_aggregate::handleAdd(uint8_t * pItem) const
{
    cm_printf("'add' not supported for contained '%s'.\n", getData()->pDesc->getName());
    return false;
}


// Handle command 'del' on command line
bool Contained_aggregate::handleDel(Command_stack * cmd, uint8_t * pItem) const
{
    cm_printf("'del' not supported for contained '%s'.\n", getData()->pDesc->getName());
    return false;
}


// Implicit add is not supported for contained components, because add isn't supported
uint8_t * Contained_aggregate::addImplicit(unsigned int itemIdx, uint8_t * pParentItem) const
{
    return nullptr;
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
    cm_printf("%s [%u]\n", getData()->pDesc->getName(), getCount(pItem));
}

}
