
#pragma once

#include <stdint.h> // uint8_t, etc
#include "cfg_mgr_types.h"
#include "cfg_mgr_metadata.h"
#include "cfg_mgr_cmd_stack.h"

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
/// - Location: offset, of the item array itself (if contained) or of a pointer
///   to the array (if owned)
// Perhaps all members should be private, with Composite_descriptor
// as friend, since it has to read (but not write) them.
//
// This is the parent class; a client's data definition uses 1 of its 2 derived
// classes.
//
class Aggregate
{
public:
    Aggregate() = default;
    virtual ~Aggregate() = default;
    void mgrInit(const Mgr_service *);
    uint8_t * getItemAtIndex(const uint8_t * pParentItem, unsigned idx) const;
    virtual bool evalCmd(Command_stack * cmd, uint8_t * pParentItem, Command_stack::eCmOp &op) const = 0;
    /// returns number of items currently in the aggregate
    virtual unsigned getCount(const uint8_t * pParentItem) const = 0;
    virtual bool evalAdd(Command_stack * cmd, uint8_t * pItem) const = 0;
    virtual bool evalDel(Command_stack * cmd, uint8_t * pItem) const = 0;
    virtual bool handleAdd(Command_stack * cmd, uint8_t * pItem) const = 0;
    virtual bool handleDel(Command_stack * cmd, uint8_t * pItem) const = 0;
    virtual void setCount(uint8_t * pParentItem, unsigned int) const = 0;
    void setDefault(uint8_t * pItem) const;
    void print(const uint8_t * pParentItem, std::string prefix, bool include_state) const;
    virtual uint8_t * add(uint8_t * pParentItem, unsigned int itemIdx, const char *id) const = 0;
    virtual void del(uint8_t * pParentItem, unsigned int itemIdx) const = 0;
    bool getComponentItem(Command_stack * cmd,
                          uint8_t * pParentItem,
                          uint8_t ** ppItem,
                          Cmd_context * candidateCtxt) const;
    virtual uint8_t * getComponentItem(unsigned idx, uint8_t * pParentItem) const = 0;
    void save(const uint8_t *pItem) const;
    Result load(uint8_t * pParentItem) const;
    virtual void help(std::string prefix, unsigned depth) const = 0;
    virtual bool hasPersistentContent(const uint8_t * pItem) const;
    virtual const Aggregate_data * getData() const = 0;

private:

    friend class Item_identifier_index;

    /// returns address of the first item in the array
    virtual uint8_t * getFirstItem(const uint8_t * pParentItem) const = 0;
    virtual void freeItems(uint8_t * pParentItem) const = 0;
    Result loadItem(uint8_t * pParentItem, unsigned idx) const;
    virtual std::string getPrintPrefixAdd(const uint8_t * pItem, unsigned index) const = 0;
    virtual std::string extractId(Command_stack * cmd, uint8_t *pParentItem, unsigned &itemIdx) const = 0;

protected:
    std::string getPrintPrefixIndex(unsigned index) const;
    std::string extractIdIndex(Command_stack *cmd, unsigned &itemIdx) const;

};

}
