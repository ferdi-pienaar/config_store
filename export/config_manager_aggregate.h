
#ifndef CFG_MAN_AGGREGATE_H
#define CFG_MAN_AGGREGATE_H

#include <stdint.h> // uint8_t, etc
#include <assert.h>
#include "config_manager_types.h"
#include "config_manager_metadata.h"

// One of the problems with the earlier version of this code that I wanted
// to avoid this time is requiring the application developer to know that
// a component counter has to precede an OWNED component array.
// Solution: during init, for an OWNED component, give a pointer
// to the descriptor of its counter, which must be a member of the same
// composite.  Hence, the offset (and size) is available, and the counter can be accessed
// (in RAM).  This also forces the application programmer to
// supply a counter reference (or explicitly give NULL if it's a array with max size 1),
// i.e. the API guides him.
// xxx Is there something we can do to verify, maybe at run-time, that the correct thing
// has been done?

namespace cfg_mgr
{

class Command_stack;
class Cmd_context;
class Store;

////////////////////////////////////////////////////////////////////////////////
/// The way in which a component Descriptor forms part of a composite.
/// Within a composite descriptor, there's an aggregate for each
/// component descriptor (i.e. one for each array of component items).
/// These are the aspects of the relationship between composite and component
/// that are controlled by the aggregate:
/// - Components may be contained (memory allocated as part of the same
///   structure as the composite) or owned (memory allocated separately
///   from that of the component, and just referenced by the composite).
/// - There may be one or more instances (i.e. single item or an array of items).
/// - Offset, of the item array itself (if embedded) or of a pointer to the array
///   (if owned)
// xxx we could embed this class in Composite_descriptor, but then
// client could not create component lists at init.  The constructor for this
// class has to be exposed to the client programmer.
// Perhaps all members should be private, with Composite_descriptor
// as friend, since it has to read (but not write) them.
//
//
class Aggregate
{
public: // Should be private; currently public so we can call methods on pData->pDesc
    const Aggregate_data * const pData;

public:
    Aggregate(const Aggregate_data * d): pData(d) {};
    virtual ~Aggregate() {}
    bool needIndex(const uint8_t * pParentItem) const
    {
        return getCount(pParentItem) > 1;
    }
    bool getIndex(Command_stack * cmd, unsigned int & itemIndex) const;
    uint8_t * getItemAtIndex(const uint8_t * pParentItem, unsigned idx) const;
    /// returns number of items currently in the aggregate
    virtual unsigned getCount(const uint8_t * pParentItem) const = 0;
    virtual bool handleAdd(uint8_t * pItem) const = 0;
    virtual bool handleDel(Command_stack * cmd, uint8_t * pItem) const = 0;
    virtual void setCount(uint8_t * pParentItem, unsigned int) const = 0;
    void setDefault(uint8_t * pItem) const;
    void print(const uint8_t * pItem, std::string prefix, bool include_state) const;
    virtual uint8_t * add(uint8_t * pParentItem) const = 0;
    virtual void del(uint8_t * pParentItem, unsigned int itemIdx) const = 0;
    bool getComponentItem(Command_stack * cmd,
                          uint8_t * pParentItem,
                          uint8_t ** ppItem,
                          bool & added,
                          Cmd_context * candidateCtxt) const;
    virtual uint8_t * getComponentItem(unsigned idx, uint8_t * pParentItem) const = 0;
    void save(const uint8_t *pItem, Store * store) const;
    result_t load(uint8_t * pParentItem, Store * store) const;
    virtual void help(const uint8_t * pItem) const = 0;
    virtual uint8_t * addImplicit(unsigned int itemIdx, uint8_t * pParentItem) const = 0;

private:
    /// returns address of the first item in the array
    virtual uint8_t * getFirstItem(const uint8_t * pParentItem) const = 0;
    virtual void freeItems(uint8_t * pParentItem) const = 0;
};


////////////////////////////////////////////////////////////////////////////////
/// In a contained aggregate, component items are contained within the composite.
/// The item memory is allocated along with that of the composite item, and the
/// 'add' and 'del' operations can't be applied to the component.
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
        return NULL;
    }
    void del(uint8_t * pParentItem, unsigned int itemIdx) const
    {
        (void)pParentItem;
        (void)itemIdx;
        assert(false);
    }
    uint8_t * addImplicit(unsigned int itemIdx, uint8_t * pParentItem) const;
    uint8_t * getComponentItem(unsigned idx, uint8_t * pParentItem) const;
    void help(const uint8_t * pItem) const;

private:
    uint8_t * getFirstItem(const uint8_t * pParentItem) const;
    // For contained items, the aggregate doesn't own the item memory, so frees nothing
    void freeItems(uint8_t * pParentItem) const
    {
        (void)pParentItem;
    }
};


////////////////////////////////////////////////////////////////////////////////
/// In an owned aggregate, component items are owned but not contained
/// by the composite. The item memory is allocated by an 'add' operation
/// and freed by a 'del' operation.  By default, the number of items is 0.
class Owned_aggregate : public Aggregate
{
public:
    Owned_aggregate(const Aggregate_data * d,
                    const Contained_aggregate * cntAggr):
        Aggregate(d), pCounterAggr(cntAggr) {}

    virtual unsigned getCount(const uint8_t * pParentItem) const;
    bool handleAdd(uint8_t * pItem) const;
    bool handleDel(Command_stack * cmd, uint8_t * pItem) const;
    void setCount(uint8_t * pParentItem, unsigned int) const;
    uint8_t * add(uint8_t * pParentItem) const;
    void del(uint8_t * pParentItem, unsigned int itemIdx) const;
    uint8_t * addImplicit(unsigned int itemIdx, uint8_t * pParentItem) const;
    uint8_t * getComponentItem(unsigned idx, uint8_t * pParentItem) const;
    void help(const uint8_t * pItem) const;

private:
    uint8_t * getFirstItem(const uint8_t * pParentItem) const;
    void freeItems(uint8_t * pParentItem) const;

    const Contained_aggregate * const pCounterAggr; // the counter for this owned component
};

}
#endif // CFG_MAN_AGGREGATE_H



