
#ifndef CFG_MAN_H
#define CFG_MAN_H

#include <stdint.h> // uint8_t, etc
#include <iostream>

#define CFG_FILE_NAME "cfg.bin"

// xxx throughout I've provisionally avoided the use of references; revise this.


// xxx one of the problems with the earlier version of this code that I'd like
// to avoid this time is requiring the application developer to know that
// a component counter has to precede an OWNED component or component array.
// Possible solution: during init, for an OWNED component, give a pointer
// to the descriptor of its counter, which must be a member of the same
// composite.  Hence, the offset (and size) is available, and the item can be accessed
// (in RAM).  This also forces the application programmer to
// supply something, i.e. the API guides him.
// Is there something we can do to verify, maybe at run-time, that the correct thing
// has been done?

// Number of bytes in an item; used in NVRAM
// Because it determines the longest possible length of any item in NVRAM,
// it's also big enough to be used for the length of items in RAM
// (which are shorter, as the exclude the Id and Length fields saved to NVRAM).
typedef uint16_t cm_item_len;

// Identifier ID, unique within its context, used to identify it in NVRFAM
typedef uint16_t cm_descriptor_id;

// Function pointers -- types registered by user when descriptor is created
typedef bool (*CM_SET_FPTR)(uint8_t *pItem, cm_item_len len, std::string val);
typedef void (*CM_SETDEF_FPTR)(uint8_t *pItem, cm_item_len len);
typedef void (*CM_PRT_FPTR)(const uint8_t *pItem, cm_item_len len);

// Pre-declare so we can use it in method prototype before defining it
struct t_cm_context;


// We eliminate the getItem method, and pass the command string recursively down the
// hierarchy of descriptors, until we either consume the whole command
// or reach a command keyword (set, setdef, prt, add, del).
// 'save' and 'load' commands are intercepted by the CM itself, since they
// have global applicability only.
//
// The recursion code should then be present in only 1 place:
// cm_composite_item_descriptor::handleCmd
//  Following commands are executed by a simple:    set, setdef, prt
//  Following commands are executed by a composite: setdef, add, del, prt

// Descriptor of configurable item (either simple or compound)
// xxx methods are private (not for user), but config_manager is friend?
class cm_item_descriptor
{
private:
    
protected: // xxx these are protected because I want to access them from the derived classes
    const std::string      name;     // name by which item is addressed on CLI
    const cm_item_len      len;      // Number of bytes occupied by an item in RAM


public:     
    const cm_descriptor_id id;       // ID (unique within the context of the component's composite) of item in NVRAM    

    cm_item_descriptor(std::string iname, cm_descriptor_id iid, cm_item_len ilen):
                       name(iname), len(ilen), id(iid){}
    virtual ~cm_item_descriptor(){}

    virtual bool handleCmd(int argc, char *argv[], uint8_t * pItem, struct t_cm_context & ctxt ) const = 0;
    virtual cm_item_len getLen() const {return len;}
    virtual cm_item_len getTlvLen(const uint8_t * pItem) const = 0;
    virtual void writeTlv(const uint8_t * pItem, uint8_t ** ppBuf) const = 0;
    virtual unsigned int loadFromTlv(FILE * fp, uint8_t * pItem) const = 0;
    virtual void print(const uint8_t * pItem, std::string prefix) const = 0;
    virtual void setdef(uint8_t * pItem) const = 0;
    virtual void help(const uint8_t * pItem) const = 0;

    std::string getName() const {return name;}

};


// xxx should not be exported
typedef struct t_cm_context
{
    std::string                str;
    const cm_item_descriptor * pDesc;
    uint8_t *                  pItem;
} cm_context;


///
//
// The way in which a cm_item_descriptor forms part of a composite.
// Within a composite descriptor, there's one of these for each
// component descriptor (i.e. one for each aggregate of component items).
//  It may be CONTAINED or OWNED.
//  There may be one or more instances (i.e. single item or an array of items).
// xxx we could embed this class in cm_composite_item_descriptor, but then
// client could not create component lists at init.  The constructor for this
// class has to be exposed to the client programmer.
// Perhaps all members should be private, with cm_composite_item_descriptor
// as friend, since it has to read (but not write) them.
// 
//
class cm_aggregate
{
protected:
    
public:   
    cm_aggregate(const cm_item_descriptor * d,
                 unsigned short maxc,
                 unsigned int o):
                 pDesc(d), maxCount(maxc), offset(o){};

    const cm_item_descriptor * pDesc;     // the component's descriptor
    const unsigned short       maxCount;  // Max number of instances of the item
    const unsigned int         offset;    // Offset [bytes] of items (or pointer to items) within the composite item
    bool needIndex(const uint8_t * pParentItem) const {return getCount(pParentItem) > 1;}
    bool getIndex(int * pArgc, char *** pArgv, const uint8_t * pParentItem, unsigned int & itemIndex) const;
    virtual uint8_t * getFirstItem(const uint8_t * pParentItem) const = 0;
    virtual unsigned getCount(const uint8_t * pParentItem) const = 0;
    virtual bool isAddSupported() const = 0;
    virtual void setCount(uint8_t * pParentItem, unsigned int) const = 0;
};


// In a contained aggregate, component items are contained within the composite:
// the item memory is allocated along with that of the composite item, and the
// 'add' and 'del' operations can't be applied to the component.
class cm_contained_aggregate : public cm_aggregate
{
public:
    cm_contained_aggregate(const cm_item_descriptor * d,
                           unsigned short maxc,
                           unsigned int o):
                           cm_aggregate(d, maxc, o){}

    uint8_t * getFirstItem(const uint8_t * pParentItem) const;
    virtual unsigned getCount(const uint8_t * pParentItem) const;
    bool isAddSupported() const {return false;}
    void setCount(uint8_t * pParentItem, unsigned int) const {assert(isAddSupported());} // add operation doesn't apply

};


// In an owned aggregate, component items are owned but not contained
// by the composite: the item memory is allocated by an 'add' operation
// and freed by a 'del' operation.  By default, the number of items is 0.
class cm_owned_aggregate : public cm_aggregate
{
private:
    const cm_contained_aggregate * pCounterAggr; // the counter for this owned component
    
public:
    cm_owned_aggregate(const cm_item_descriptor * d,
                       unsigned short maxc,
                       unsigned int o,
                       const cm_contained_aggregate * cntAggr):
                       cm_aggregate(d, maxc, o), pCounterAggr(cntAggr){}

    uint8_t * getFirstItem(const uint8_t * pParentItem) const;
    virtual unsigned getCount(const uint8_t * pParentItem) const;
    bool isAddSupported() const {return true;}
    void setCount(uint8_t * pParentItem, unsigned int) const;


};


// Composite item descriptor's contain a list of components.
// xxx methods (apart from constructor) are private (not for user), but config_manager is friend?
class cm_composite_item_descriptor : public cm_item_descriptor
{ 
    const cm_aggregate * const * aggrList;  // Array of pointers to aggregates (pointers, because abstract cm_aggregate can't be instantiated)
    const unsigned short         aggrCount; // Number of aggregates in the list (number of descriptors, not items)

    virtual void writeTlv(const uint8_t * pItem, uint8_t ** ppBuf) const;
    virtual cm_item_len getTlvLen(const uint8_t * pItem) const;
    virtual unsigned int loadFromTlv(FILE * fp, uint8_t * pItem) const;
    unsigned int skipTlvItem(FILE * fp) const;
    const cm_aggregate * getAggr(const char * name) const;
    const cm_aggregate * getAggr(cm_descriptor_id id) const;
    bool getComponentItem(int * pArgc,
                          char *** pArgv,
                          const cm_aggregate ** ppAggr,
                          uint8_t * pParentItem,
                          uint8_t ** ppItem,
                          cm_context & ctxt,
                          bool & added) const;
    bool getComponentItem(cm_descriptor_id id,
                          unsigned idx,
                          const cm_aggregate ** ppAggr,
                          uint8_t * pParentItem,
                          uint8_t ** ppItem) const;

public:    
    cm_composite_item_descriptor(char * name,
                                 cm_descriptor_id id,
                                 cm_item_len l,
                                 const cm_aggregate * const * aggrList,
                                 unsigned short aggrCount):
                                 cm_item_descriptor(name, id, l), // init base class
                                 aggrList(aggrList),         // init data member
                                 aggrCount(aggrCount){};

    ~cm_composite_item_descriptor(){};
    bool handleCmd(int argc, char *argv[], uint8_t * pItem, cm_context & ctxt) const;
    void print(const uint8_t * pItem, std::string prefix) const;
    void setdef(uint8_t * pItem) const;
    virtual void help(const uint8_t * pItem) const;
    bool handleAdd(int argc, char *argv[], uint8_t * pItem) const;
    uint8_t * add(uint8_t * pParentItem, const cm_aggregate * pAggr) const;
    bool handleDel(int argc, char *argv[], uint8_t * pItem) const;
    void del(uint8_t * pParentItem,
             const cm_aggregate * pAggr,
             unsigned int itemIdx,
             unsigned int cnt) const;

};


// xxx methods (apart from constructor) are private (not for user), but config_manager is friend?
class cm_simple_item_descriptor : public cm_item_descriptor
{
    const CM_PRT_FPTR    pPrt;

public:
    cm_simple_item_descriptor(char * name,
                              cm_descriptor_id id,
                              cm_item_len l,
                              CM_PRT_FPTR pf):
                              cm_item_descriptor(name, id, l),
                              pPrt(pf){};
                              
    virtual ~cm_simple_item_descriptor() {};

    void print(const uint8_t * pItem, std::string prefix) const;
    void help(const uint8_t * pItem) const {std::cout << "len " << getLen() << std::endl;}
};


// xxx methods (apart from constructor) are private (not for user), but config_manager is friend?
class cm_basic_item_descriptor : public cm_simple_item_descriptor
{

    // populate the following ptrs when creating a descriptor
    // The config manager itself provides a set for basic types
    // of configurable items, with 'C' linkage.
    // These are only applicable to simple items:
    //  set
    //  setdef
    //
    const CM_SET_FPTR    pSet;
    const CM_SETDEF_FPTR pSetDef;

    virtual void writeTlv(const uint8_t * pItem, uint8_t ** ppBuf) const;
    virtual cm_item_len getTlvLen(const uint8_t * pItem) const;
    virtual unsigned int loadFromTlv(FILE * fp, uint8_t * pItem) const;

public:
    cm_basic_item_descriptor(char * name,
                             cm_descriptor_id id,
                             cm_item_len l,
                             CM_SET_FPTR sf,
                             CM_SETDEF_FPTR sdf,
                             CM_PRT_FPTR pf):
                             cm_simple_item_descriptor(name, id, l, pf),
                             pSet(sf),
                             pSetDef(sdf){};
                              
    ~cm_basic_item_descriptor(){};

    bool handleCmd(int argc, char *argv[], uint8_t * pItem, cm_context & ctxt) const;
    bool set(uint8_t * pItem, std::string val) const;
    void setdef(uint8_t * pItem) const;
};


// xxx methods (apart from constructor) are private (not for user), but config_manager is friend?
class cm_cntr_item_descriptor : public cm_simple_item_descriptor
{
    virtual void writeTlv(const uint8_t * pItem, uint8_t ** ppBuf) const {}
    virtual cm_item_len getTlvLen(const uint8_t * pItem) const {return 0;}
    virtual unsigned int loadFromTlv(FILE * fp, uint8_t * pItem) const {assert(0);}

public:
    cm_cntr_item_descriptor(char * name,
                            cm_descriptor_id id,
                            cm_item_len l,
                            CM_PRT_FPTR pf):
                            cm_simple_item_descriptor(name, id, l, pf){};
                              
    ~cm_cntr_item_descriptor(){};
    bool handleCmd(int argc, char *argv[], uint8_t * pItem, cm_context & ctxt) const;
    // A counter's setdef does nothing: it is set to 0 as a side-effect of
    // freeing the corresponding owned items.
    void setdef(uint8_t * pItem) const {};

};


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
    void init(const cm_item_descriptor * pDesc);
    const char * getPromptString();
    void * getConfig(){return (void *)ramBase;}
    static config_manager * getInstance();

    // xxx should only be accessible to friend classes
    void resetCtxt();
    void updateCtxt(cm_context * pC);

private:
    config_manager():base_desc(NULL), ramBase(NULL){}
    void save();
    void load();

    static config_manager * instance;
    const cm_item_descriptor * base_desc;    
    uint8_t *    ramBase;
    cm_context   currCtxt; // current context

};

#endif // CFG_MAN_H

