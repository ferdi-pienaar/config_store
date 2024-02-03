
#pragma once

#include <stdint.h> // uint8_t, etc
#include <assert.h>
#include "cfg_mgr_aggregate.h" // parent class
#include "cfg_mgr_types.h"
#include "cfg_mgr_metadata.h"

namespace cfg_mgr
{

class Command_stack;
class Cmd_context;
class Store;


////////////////////////////////////////////////////////////////////////////////
/// In a contained aggregate, component items are contained within the composite.
/// The item memory is allocated along with that of the composite item, and the
/// 'add' and 'del' operations can't be applied to the component, since the
/// number of contained items is fixed.
class Contained_aggregate : public Aggregate
{
public:
    Contained_aggregate(const Aggregate_data * d): Aggregate(d) {}

    virtual unsigned getCount(const uint8_t * pParentItem) const;
    void setCount(uint8_t * pParentItem, unsigned int) const
    {
        (void)pParentItem;
        assert(false); // not modifiable
    }
    bool handleAdd(uint8_t * pItem) const;
    bool handleDel(Command_stack * cmd, uint8_t * pItem) const;
    uint8_t * add(uint8_t * pParentItem) const
    {
        (void)pParentItem;
        assert(false);
        return nullptr;
    }
    void del(uint8_t * pParentItem, unsigned int itemIdx) const
    {
        (void)pParentItem;
        (void)itemIdx;
        assert(false);
    }
    uint8_t * getComponentItem(unsigned idx, uint8_t * pParentItem) const;
    void help(const uint8_t * pItem) const;

private:
    uint8_t * getFirstItem(const uint8_t * pParentItem) const;
    // For contained items, the aggregate doesn't own the item memory, so frees nothing
    void freeItems(uint8_t * pParentItem) const
    {
        (void)pParentItem;
    }
    uint8_t * addImplicit(unsigned int itemIdx, uint8_t * pParentItem) const;
};

}
