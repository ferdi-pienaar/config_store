
#ifndef CFG_MGR_AGGREGATE_H
#define CFG_MGR_AGGREGATE_H

#include <stdint.h> // uint8_t, etc
#include "cfg_mgr_types.h"
#include "cfg_mgr_metadata.h"

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
///   from that of the composite, and just referenced by the composite).
/// - The number of component instances (i.e. single item or an array of items).
/// - Location: offset, of the item array itself (if embedded) or of a pointer
///   to the array (if owned)
// xxx we could embed this class in Composite_descriptor, but then
// client could not create component lists at init.  The constructor for this
// class has to be exposed to the client programmer.
// Perhaps all members should be private, with Composite_descriptor
// as friend, since it has to read (but not write) them.
//
// This is the parent class; a client's data definition uses 1 of its 2 derived
// classes.
//
class Aggregate
{
public:
    Aggregate(const Aggregate_data * d): m_data(d) {};
    virtual ~Aggregate() {}
    bool needIndex(const uint8_t * pParentItem) const;
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
    const Aggregate_data * getData() const
    {
        return m_data;
    }

private:
    /// returns address of the first item in the array
    virtual uint8_t * getFirstItem(const uint8_t * pParentItem) const = 0;
    virtual void freeItems(uint8_t * pParentItem) const = 0;
    virtual uint8_t * addImplicit(unsigned int itemIdx, uint8_t * pParentItem) const = 0;
    result_t loadItem(uint8_t * pParentItem, unsigned idx, Store * store) const;

    const Aggregate_data * const m_data;
};

}

#endif // CFG_MGR_AGGREGATE_H
