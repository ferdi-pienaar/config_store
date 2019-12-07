
#ifndef CFG_MAN_H
#define CFG_MAN_H

#include <stdint.h> // uint8_t, etc
#include <assert.h>
#include <iostream>
#include "Config_manager_types.h"
#include "Config_manager_store.h"
#include "Config_manager_metadata.h"

/*
 *
 *              +----------------+
 *              |  descriptor    |
 *              |----------------|
 *              | name           | 1
 *              | id             |<-------------------------------------+
 *              | len            |                                      |
 *              | persistent     |                                      |
 *              +----------------+                                      |
 *                       ^                                              |
 *                      / \                                             |
 *                     -----                                            |
 *                       |                                              |
 *          +------------+------------+                                 |
 *          |                         |                                 |
 * +-------------------+  +----------------------+ 1  * +-----------+ 1 |
 * | simple descriptor |  | composite descriptor +<>--->| aggregate |<>-+
 * |-------------------|  |----------------------|      |-----------|
 *                                                      | maxCount  |
 *                                                      | offset    |
 *                                                      +-----------+
 *                                                            ^
 *                                                           / \
 *                                                          -----
 *                                                            |
 *                                              +-------------+-------+
 *                                              |                     |
 *                                  +---------------------+  +-----------------+
 *                                  | contained aggregate |  | owned aggregate |
 *                                  |---------------------|  |-----------------|
 *
 */


// xxx throughout I've provisionally avoided the use of references; revise this.


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

// xxx should not be exported
// The context in which a command string is interpreted:
// the current item, its descriptor (i.e. its metadata), and the
// string that's displayed on the command-line to represent the context, i.e. the location
// of the item within the hierarchy of items.
class Cmd_context
{
public:
    Cmd_context (std::string istr = "", const Descriptor * desc = NULL, uint8_t * item = NULL):
        str(istr), pDesc(desc), pItem(item) {}
    void add(std::string w);
    void add(unsigned idx);
    void setDesc(const Descriptor * desc)
    {
        pDesc = desc;
    }

    void setItem(uint8_t * item)
    {
        pItem = item;
    }

    std::string getString() const {
        return str;
    }
    const Descriptor * getDesc() const {
        return pDesc;
    }
    uint8_t * getItem() const {
        return pItem;
    }

private:
    std::string           str;
    const Descriptor *    pDesc;
    uint8_t *             pItem;
};


// We eliminate the getItem method, and pass the command string recursively down the
// hierarchy of descriptors, until we either consume the whole command
// or reach a command keyword (set, setDefault, prt, add, del).
// 'save' and 'load' commands are intercepted by the CM itself, since they
// have global applicability only.
//
// The recursion code should then be present in only 1 place:
// Composite_descriptor::handleCmd
//  Following commands are executed by a simple:    set, setDefault, prt
//  Following commands are executed by a composite: setDefault, add, del, prt

class Command_stack;

////////////////////////////////////////////////////////////////////////////////
/// Descriptor of configurable item (either simple or composite).
// xxx methods are private (not for user), but Config_manager is friend?
class Descriptor
{

public:
    Descriptor() {}
    virtual ~Descriptor() {}

    virtual bool handleCmd(Command_stack * cmd, uint8_t * pItem, Cmd_context * candidate, bool & setCtxt) const = 0;
    virtual const char * getName() const = 0;
    virtual item_id_t getId() const = 0;
    virtual void save(const uint8_t * pItem, Store * store) const = 0;
    virtual result_t startLoad(Store * store) const = 0;
    virtual result_t endLoad(uint8_t * pItem, Store * store) const = 0;
    virtual item_len_t getLen() const = 0;
    virtual void print(const uint8_t * pItem, std::string prefix, bool include_state) const = 0;
    virtual void setDefault(uint8_t * pItem) const = 0;
    virtual void help(const uint8_t * pItem) const = 0;
    virtual bool isPersistent() const = 0;

};


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
    bool needIndex(const uint8_t * pParentItem) const {
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
    void setCount(uint8_t * pParentItem, unsigned int) const {
        (void)pParentItem;
        assert(false); // not modifiable
    }
    bool handleAdd(uint8_t * pItem) const;
    bool handleDel(Command_stack * cmd, uint8_t * pItem) const;
    uint8_t * add(uint8_t * pParentItem) const {
        (void)pParentItem;
        assert(false);
        return NULL;
    }
    void del(uint8_t * pParentItem, unsigned int itemIdx) const {
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
    void freeItems(uint8_t * pParentItem) const {
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


////////////////////////////////////////////////////////////////////////////////
/// A composite descriptor consists of components, linked to the composite via
/// aggregates.
// xxx methods (apart from constructor) are private (not for user), but Config_manager is friend?
class Composite_descriptor : public Descriptor
{
public:
    Composite_descriptor(const Composite_metadata * pMeta);
    ~Composite_descriptor() {};
    const char * getName() const {
        return pData->c.name;
    }
    virtual item_id_t getId() const {
        return pData->c.id;
    }
    virtual item_len_t getLen() const {
        return pData->c.len;
    }
    bool handleCmd(Command_stack * cmd, uint8_t * pItem, Cmd_context * candidate, bool & setCtxt) const;
    void print(const uint8_t * pItem, std::string prefix, bool include_state) const;
    void setDefault(uint8_t * pItem) const;
    virtual void help(const uint8_t * pItem) const;
    void save(const uint8_t * pItem, Store * store) const;
    result_t startLoad(Store * store) const;
    result_t endLoad(uint8_t * pItem, Store * store) const;
    bool isPersistent() const {
        return pData->c.persistent;
    }

private:
    bool handleAdd(Command_stack * cmd, uint8_t * pItem) const;
    bool handleDel(Command_stack * cmd, uint8_t * pItem) const;
    bool handleIdWord(Command_stack * cmd, uint8_t * pItem, Cmd_context * candidate, bool & setCtxt) const;
    virtual unsigned short getAggrCount() const {
        return pData->aggrCount;
    }
    virtual const Aggregate * getAggrAtIndex(unsigned int i) const {
        return pData->aggrList[i];
    }
    const Aggregate * getAggr(const char * name) const;
    const Aggregate * getAggr(item_id_t id) const;

    const Composite_metadata * const pData;
};


////////////////////////////////////////////////////////////////////////////////
/// A simple descriptor is a leaf in the tree of descriptors, representing
/// metadata for a single configurable item.
// xxx methods (apart from constructor) are private (not for user), but Config_manager is friend?
class Simple_descriptor : public Descriptor
{
public:
    Simple_descriptor(const Simple_metadata * pMeta);
    virtual ~Simple_descriptor() {}
    bool handleCmd(Command_stack * cmd, uint8_t * pItem, Cmd_context * candidate, bool & setCtxt) const;
    const char * getName() const {
        return pData->c.name;
    }
    virtual item_id_t getId() const {
        return pData->c.id;
    }
    virtual item_len_t getLen() const {
        return pData->c.len;
    }
    void print(const uint8_t * pItem, std::string prefix, bool include_state) const;
    bool set(uint8_t * pItem, std::string val) const;
    void setDefault(uint8_t * pItem) const;
    void help(const uint8_t * pItem) const {
        (void)pItem;
        std::cout << "len " << getLen() << std::endl;
    }
    virtual void save(const uint8_t * pItem, Store * store) const;
    result_t startLoad(Store * store) const;
    result_t endLoad(uint8_t * pItem, Store * store) const;
    bool isPersistent() const {
        return pData->c.persistent;
    }

private:
    const Simple_metadata * const pData;
};


/// Configuration manager, managing all configurable items in the system.
// This class is the application programme's sole point of access to
// the configurable items.
class Config_manager
{
public:
    Config_manager(const Descriptor * pDesc);
    ~Config_manager();
    void handleCmd(int argc, char *argv[]);
    const char * getPromptString() const; ///< get context-dependent prompt string h file
    void * getConfig() {
        return (void *)ramBase;
    }

private:
    void save();
    void load();
    void resetCtxt();

    const Descriptor * base_desc;
    uint8_t *    ramBase;
    Cmd_context   currCtxt;      // current command-line context
    Cmd_context   candidateCtxt; // command-line context built while handling current command
    Store * store;
};


// Container for the command passed to cfg_mgr.  It's a stack of words, i.e.
// a stack of C-strings, each string consisting of 1 word only (i.e. no spaces).
class Command_stack
{
public:
    // Operations - each represents a reserved 'word' in commands passed to Config_manager
    enum eCmOp
    {
        CM_ADD,
        CM_DEL,
        CM_PRT,
        CM_PRT_CFG,
        CM_SET,
        CM_SETDEF,
        CM_LOAD,
        CM_SAVE,
        CM_HELP,       //
        CM_RESET_CTXT, // return context to top level
        CM_OP_NONE
    };

    Command_stack(int argc, char ** argv) : count(argc), wordPtr(argv) {}

    // Pop top word.
    // Return ref to self so the value returned by the command can be passed to a fn
    Command_stack & pop()
    {
        count--;
        wordPtr++;
        return *this;
    }

    char * getTop() const
    {
        return wordPtr[0];
    }

    int getCount() const
    {
        return count;
    }

    eCmOp getTopOp() const;
    bool getIndex(unsigned int & itemIdx);

private:
    int     count;
    char ** wordPtr;
};
}
#endif // CFG_MAN_H

