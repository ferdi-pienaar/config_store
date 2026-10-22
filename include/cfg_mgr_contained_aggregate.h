
#pragma once

#include <stdint.h> // uint8_t, etc
#include <assert.h>
#include "cfg_mgr_aggregate.h" // parent class
#include "cfg_mgr_types.h"
#include "cfg_mgr_metadata.h"

namespace cfg_mgr
{

class Command_stack;

////////////////////////////////////////////////////////////////////////////////
/// In a contained aggregate, component items are contained within the composite.
/// The item memory is allocated along with that of the composite item, and the
/// 'add' and 'del' operations can't be applied to the component, since the
/// number of contained items is fixed.
class Contained_aggregate : public Aggregate
{
public:
    Contained_aggregate(const Aggregate_data * d): m_data(d) {}
    virtual unsigned getCount(const uint8_t * pParentItem) const override;
    void setCount(uint8_t * pParentItem, unsigned int) const override
    {
        (void)pParentItem;
        assert(false); // not modifiable
    }
    bool evalCmd(Command_stack * cmd, uint8_t * pParentItem, Command_stack::eCmOp &op) const override;
    bool evalAdd(Command_stack * cmd, uint8_t * pItem) const override;
    bool evalDel(Command_stack * cmd, uint8_t * pItem) const override;
    bool handleAdd(Command_stack * cmd, uint8_t * pItem) const override;
    bool handleDel(Command_stack * cmd, uint8_t * pItem) const override;
    uint8_t * add(uint8_t * pParentItem, unsigned int itemIdx, const char *id) const override
    {
        (void)pParentItem;
        assert(false);
        return nullptr;
    }
    void del(uint8_t * pParentItem, unsigned int itemIdx) const override
    {
        (void)pParentItem;
        (void)itemIdx;
        assert(false);
    }
    uint8_t * getComponentItem(unsigned idx, uint8_t * pParentItem) const override;
    void help(std::string prefix, unsigned depth) const override;
    const Aggregate_data * getData() const override { return m_data; }

private:
    uint8_t * getFirstItem(const uint8_t * pParentItem) const override;
    // For contained items, the aggregate doesn't own the item memory, so frees nothing
    void freeItems(uint8_t * pParentItem) const override
    {
        (void)pParentItem;
    }
    std::string getPrintPrefixAdd(const uint8_t * pItem, unsigned index) const override;
    std::string extractId(Command_stack * cmd, uint8_t *pParentItem, unsigned &itemIdx) const override;

    friend class Item_identifier_key;

    const Aggregate_data * const m_data;
};

}
