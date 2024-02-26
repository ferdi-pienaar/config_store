
#pragma once

#include <stdint.h> // uint8_t, etc
#include "cfg_mgr_aggregate.h" // parent class
#include "cfg_mgr_types.h"
#include "cfg_mgr_metadata.h"

// One of the problems with the earlier version of this code that I wanted
// to avoid this time is requiring the application developer to know that
// a component counter has to precede an OWNED component array.
// Solution: during init, for an OWNED component, give a pointer
// to the descriptor of its counter, which must be a member of the same
// composite.  Hence, the offset (and size) is available, and the counter can be accessed
// (in RAM).  This also forces the application programmer to
// supply a counter reference (or explicitly give nullptr if it's an array with max size 1),
// i.e. the API guides him.
// xxx Is there something we can do to verify, maybe at run-time, that the correct thing
// has been done?

namespace cfg_mgr
{

class Command_stack;
class Cmd_context;
class Store;
class Contained_aggregate;

////////////////////////////////////////////////////////////////////////////////
/// In an owned aggregate, component items are owned but not contained
/// within the composite. The item memory is allocated by an 'add' operation
/// and freed by a 'del' operation.  By default, the number of items is 0.
class Owned_aggregate : public Aggregate
{
public:
    Owned_aggregate(const Aggregate_data * d,
                    const Contained_aggregate * cntAggr):
        Aggregate(d), m_counterAggr(cntAggr) {}

    virtual unsigned getCount(const uint8_t * pParentItem) const override;
    bool handleAdd(uint8_t * pItem) const override;
    bool handleDel(Command_stack * cmd, uint8_t * pItem) const override;
    void setCount(uint8_t * pParentItem, unsigned int) const override;
    uint8_t * add(uint8_t * pParentItem) const override;
    void del(uint8_t * pParentItem, unsigned int itemIdx) const override;
    uint8_t * getComponentItem(unsigned idx, uint8_t * pParentItem) const override;
    void help(const uint8_t * pItem) const override;

private:
    uint8_t * getFirstItem(const uint8_t * pParentItem) const override;
    void freeItems(uint8_t * pParentItem) const override;
    uint8_t * addImplicit(unsigned int itemIdx, uint8_t * pParentItem) const override;

    const Contained_aggregate * const m_counterAggr; // the counter for this owned component
};

}
