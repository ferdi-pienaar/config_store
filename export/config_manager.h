
#ifndef CFG_MAN_H
#define CFG_MAN_H

#include <stdint.h> // uint8_t, etc
#include <assert.h>
#include <iostream>
#include "config_manager_types.h"
#include "config_manager_tlv.h"


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

// Function pointers -- types registered by user when descriptor is created
typedef bool (*CM_SET_FPTR)(uint8_t *pItem, cm_item_len_t len, std::string val);
typedef void (*CM_SETDEF_FPTR)(uint8_t *pItem, cm_item_len_t len);
typedef void (*CM_PRT_FPTR)(const uint8_t *pItem, cm_item_len_t len);


// Item metadata, i.e. information about what's kept in RAM.
// Place metadata in structure separate from the descriptor,
// to make it easy to ensure it is ROMable (the rules for ensuring
// a class object is ROMable, are very restrictive).
// This comes at the cost of having an additional level of indirection.
struct cm_common_metadata
{
    const char *  name;       ///< name by which item is addressed on CLI    
    cm_item_id_t  id;         ///< ID (unique within the context of the component's composite) of item in NVRAM
    cm_item_len_t len;        ///< Number of bytes occupied by an item in RAM
    bool          persistent; ///< Saved to NVRAM?
};

struct cm_simple_metadata
{
    cm_common_metadata c;

    // populate the following ptrs when creating a descriptor
    // The config manager itself provides a set for basic types
    // of configurable items, with 'C' linkage.
    //
    const CM_SET_FPTR    pSet;
    const CM_SETDEF_FPTR pSetDefault;
    const CM_PRT_FPTR    pPrt;
};

class cm_aggregate;

struct cm_composite_metadata
{
    cm_common_metadata c;

    // Information about the components of the composite
    const cm_aggregate * const * aggrList;  // Array of pointers to aggregates (pointers, because abstract cm_aggregate can't be instantiated)
    const unsigned short         aggrCount; // Number of aggregates in the list (number of descriptors, not items)

};

class cm_descriptor;

struct cm_aggregate_data
{    
    const cm_descriptor * pDesc;     ///< the component's descriptor
    const unsigned short  maxCount;  ///< Max number of instances of the item
    const unsigned int    offset;    ///< Offset [bytes] of items, or pointer to items, within the composite item
};


// xxx should not be exported
typedef struct t_cm_context
{
    std::string            str;
    const cm_descriptor *  pDesc;
    uint8_t *              pItem;
} cm_context;


// We eliminate the getItem method, and pass the command string recursively down the
// hierarchy of descriptors, until we either consume the whole command
// or reach a command keyword (set, setDefault, prt, add, del).
// 'save' and 'load' commands are intercepted by the CM itself, since they
// have global applicability only.
//
// The recursion code should then be present in only 1 place:
// cm_composite_descriptor::handleCmd
//  Following commands are executed by a simple:    set, setDefault, prt
//  Following commands are executed by a composite: setDefault, add, del, prt

class command_stack;

////////////////////////////////////////////////////////////////////////////////
/// Descriptor of configurable item (either simple or compound).
// xxx methods are private (not for user), but config_manager is friend?
class cm_descriptor
{

public:
    cm_descriptor() {}
    virtual ~cm_descriptor(){}

    virtual bool handleCmd(command_stack * cmd, uint8_t * pItem, struct t_cm_context & ctxt ) const = 0;
    virtual std::string getName() const = 0;
    virtual cm_item_id_t getId() const = 0;
    virtual void writeTlv(const uint8_t * pItem) const = 0;
    virtual t_cm_result loadFromTlv(uint8_t * pItem, unsigned * pComplete) const = 0;
    virtual cm_item_len_t getLen() const = 0;
    virtual void print(const uint8_t * pItem, std::string prefix) const = 0;
    virtual void setDefault(uint8_t * pItem) const = 0;
    virtual void help(const uint8_t * pItem) const = 0;
    virtual bool isPersistent() const = 0;

};


////////////////////////////////////////////////////////////////////////////////
/// The way in which a component cm_descriptor forms part of a composite.
/// Within a composite descriptor, there's an aggregate for each
/// component descriptor (i.e. one for each aggregate of component items).
/// These are the aspects of the relationship between composite and component
/// that are controlled by the aggregate:
/// - Components may be contained (memory allocated as part of the same
///   structure as the composite) or owned (memory allocated separately
///   from that of the component, and just referenced by the composite).
/// - There may be one or more instances (i.e. single item or an array of items).
/// - Offset, of the item itself (if embedded) or of a pointer to the item
///   (if owned)
// xxx we could embed this class in cm_composite_descriptor, but then
// client could not create component lists at init.  The constructor for this
// class has to be exposed to the client programmer.
// Perhaps all members should be private, with cm_composite_descriptor
// as friend, since it has to read (but not write) them.
// 
//
class cm_aggregate
{
public: // Should be private; currently public so we can call methods on pData->pDesc
    const cm_aggregate_data * pData;
    
public:   
    cm_aggregate(const cm_aggregate_data * d): pData(d){};
    virtual ~cm_aggregate(){}
    bool needIndex(const uint8_t * pParentItem) const {return getCount(pParentItem) > 1;}
    bool getIndex(command_stack * cmd, const uint8_t * pParentItem, unsigned int & itemIndex) const;
    uint8_t * getItemAtIndex(const uint8_t * pParentItem, unsigned idx) const;
    /// returns number of items currently in the aggregate
    virtual unsigned getCount(const uint8_t * pParentItem) const = 0;
    virtual bool isAddSupported() const = 0;
    virtual void setCount(uint8_t * pParentItem, unsigned int) const = 0;
    void setDefault(uint8_t * pItem) const;
    void print(const uint8_t * pItem, std::string prefix) const;

private:
    /// returns address of the first item in the array
    virtual uint8_t * getFirstItem(const uint8_t * pParentItem) const = 0;
    virtual void freeItems(uint8_t * pParentItem) const = 0;
};


////////////////////////////////////////////////////////////////////////////////
/// In a contained aggregate, component items are contained within the composite.
/// The item memory is allocated along with that of the composite item, and the
/// 'add' and 'del' operations can't be applied to the component.
class cm_contained_aggregate : public cm_aggregate
{
public:
    cm_contained_aggregate(const cm_aggregate_data * d): cm_aggregate(d){}

    virtual unsigned getCount(const uint8_t * pParentItem) const;
    bool isAddSupported() const {return false;}
    void setCount(uint8_t * pParentItem, unsigned int) const {assert(isAddSupported());} // add operation doesn't apply

private:
    uint8_t * getFirstItem(const uint8_t * pParentItem) const;
    // For contained items, the aggregate doesn't own the item memory, so frees nothing
    void freeItems(uint8_t * pParentItem) const {}
};


////////////////////////////////////////////////////////////////////////////////
/// In an owned aggregate, component items are owned but not contained
/// by the composite. The item memory is allocated by an 'add' operation
/// and freed by a 'del' operation.  By default, the number of items is 0.
class cm_owned_aggregate : public cm_aggregate
{
public:
    cm_owned_aggregate(const cm_aggregate_data * d,
                       const cm_contained_aggregate * cntAggr):
                       cm_aggregate(d), pCounterAggr(cntAggr){}

    virtual unsigned getCount(const uint8_t * pParentItem) const;
    bool isAddSupported() const {return true;}
    void setCount(uint8_t * pParentItem, unsigned int) const;

private:
    uint8_t * getFirstItem(const uint8_t * pParentItem) const;
    void freeItems(uint8_t * pParentItem) const;
    
    const cm_contained_aggregate * pCounterAggr; // the counter for this owned component
};


////////////////////////////////////////////////////////////////////////////////
/// A composite descriptor consists of components, linked to the composite via
/// aggregates.
// xxx methods (apart from constructor) are private (not for user), but config_manager is friend?
class cm_composite_descriptor : public cm_descriptor
{ 
public:    
    cm_composite_descriptor(const cm_composite_metadata * pMeta);
    ~cm_composite_descriptor(){};
    std::string getName() const {return pData->c.name;}
    virtual cm_item_id_t getId() const {return pData->c.id;}
    virtual cm_item_len_t getLen() const {return pData->c.len;}
    bool handleCmd(command_stack * cmd, uint8_t * pItem, cm_context & ctxt) const;
    void print(const uint8_t * pItem, std::string prefix) const;
    void setDefault(uint8_t * pItem) const;
    virtual void help(const uint8_t * pItem) const;
    void writeTlv(const uint8_t * pItem) const;
    t_cm_result loadFromTlv(uint8_t * pItem, unsigned * pComplete) const;
    bool isPersistent() const { return pData->c.persistent; }

private:
    bool handleAdd(command_stack * cmd, uint8_t * pItem) const;
    bool handleDel(command_stack * cmd, uint8_t * pItem) const;
    bool handleIdWord(command_stack * cmd, uint8_t * pItem, cm_context & ctxt) const;
    bool getComponentItem(command_stack * cmd,
                          const cm_aggregate ** ppAggr,
                          uint8_t * pParentItem,
                          uint8_t ** ppItem,
                          cm_context & ctxt,
                          bool & added) const;
    bool getComponentItem(unsigned idx,
                          const cm_aggregate * pAggr,
                          uint8_t * pParentItem,
                          uint8_t ** ppItem) const;
    virtual unsigned short getAggrCount() const {return pData->aggrCount;}
    virtual const cm_aggregate * getAggrAtIndex(unsigned int i) const {return pData->aggrList[i];}
    uint8_t * add(uint8_t * pParentItem, const cm_aggregate * pAggr) const;
    void del(uint8_t * pParentItem,
             const cm_aggregate * pAggr,
             unsigned int itemIdx,
             unsigned int cnt) const;
    const cm_aggregate * getAggr(const char * name) const;
    const cm_aggregate * getAggr(cm_item_id_t id) const;

    const cm_composite_metadata * const pData;
};


////////////////////////////////////////////////////////////////////////////////
/// A simple descriptor is a leaf in the tree of descriptors, representing
/// metadata for a single configurable item.
// xxx methods (apart from constructor) are private (not for user), but config_manager is friend?
class cm_simple_descriptor : public cm_descriptor
{
public:
    cm_simple_descriptor(const cm_simple_metadata * pMeta);
    virtual ~cm_simple_descriptor() {}
    bool handleCmd(command_stack * cmd, uint8_t * pItem, cm_context & ctxt) const;
    std::string getName() const {return pData->c.name;}
    virtual cm_item_id_t getId() const {return pData->c.id;}
    virtual cm_item_len_t getLen() const {return pData->c.len;}
    void print(const uint8_t * pItem, std::string prefix) const;
    bool set(uint8_t * pItem, std::string val) const;
    void setDefault(uint8_t * pItem) const;
    void help(const uint8_t * pItem) const {std::cout << "len " << getLen() << std::endl;}
    virtual void writeTlv(const uint8_t * pItem) const;
    t_cm_result loadFromTlv(uint8_t * pItem, unsigned * pComplete) const;
    bool isPersistent() const { return pData->c.persistent; }

private:
    const cm_simple_metadata * const pData;

};


/// Configuration manager, managing all configurable items in the system.
// This is a singleton because it avoids the following practical problem:
// in config_manager.cpp, how do the other classes access this one to
// modify the current context, pCtxt.  We could give each instance a
// pointer to this object, but it seems like overkill since there SHOULD
// only be one of them.
// This class is the application programme's sole point of access to
// the configurable items.
class config_manager
{  
public:
    void handleCmd(int argc, char *argv[]);
    void init(const cm_descriptor * pDesc);
    const char * getPromptString(); ///< get context-dependent prompt string h file
    void * getConfig(){return (void *)ramBase;}
    static config_manager * getInstance();

    // xxx should only be accessible to friend classes
    void resetCtxt();
    void updateCtxt(cm_context * pC);

    Tlv tlv;


private:
    config_manager():base_desc(NULL), ramBase(NULL){}
    void save();
    void load();

    static config_manager * instance;
    const cm_descriptor * base_desc;    
    uint8_t *    ramBase;
    cm_context   currCtxt; // current context


};


//
class command_stack
{
public:
    // Operations - each represents a reserved 'word' in commands passed to config_manager
    enum eCmOp
    {
        CM_ADD,
        CM_DEL,
        CM_PRT,
        CM_SET,
        CM_SETDEF,
        CM_LOAD,
        CM_SAVE,
        CM_HELP,       //
        CM_RESET_CTXT, // return context to top level
        CM_OP_NONE
    };
    
    command_stack(int argc, char ** argv) : count(argc), wordPtr(argv) {}
    command_stack & pop()
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
    
private:
    int     count;
    char ** wordPtr;
};

#endif // CFG_MAN_H

