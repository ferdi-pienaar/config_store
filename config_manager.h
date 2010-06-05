
#ifndef CFG_MAN_H
#define CFG_MAN_H

#include <stdint.h> // uint8_t, etc
#include <iostream>

#define CFG_FILE_NAME "cfg.bin"

using namespace std;

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
typedef unsigned short cm_item_len;

// Identifier ID, unique within its context, used to identify it in NVRFAM
typedef unsigned short cm_descriptor_id;

// Function pointers -- types registered by user when descriptor is created
typedef void (*CM_SET_FPTR)(unsigned char *pItem, cm_item_len len, string val);
typedef void (*CM_SETDEF_FPTR)(unsigned char *pItem, cm_item_len len);
typedef void (*CM_PRT_FPTR)(const unsigned char *pItem, cm_item_len len);

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
    const string           name;     // name by which item is addressed on CLI
    const cm_item_len      len;      // Number of bytes occupied by an item in RAM


public:     
    const cm_descriptor_id id;       // ID (unique within the context of the component's composite) of item in NVRAM    


    cm_item_descriptor(string iname, cm_descriptor_id iid, cm_item_len ilen):
                       name(iname), len(ilen), id(iid){}
    virtual ~cm_item_descriptor(){}

    virtual void handleCmd(int argc, char *argv[], unsigned char * pItem, struct t_cm_context & ctxt ) const = 0;
    virtual cm_item_len getLen() const {return len;}
    virtual cm_item_len getTlvLen(const unsigned char * pItem) const = 0;
    virtual void writeTlv(const unsigned char * pItem, unsigned char ** ppBuf) const = 0;
    virtual int loadFromTlv(FILE * fp, unsigned char * pItem) const = 0;
    virtual void print(const unsigned char * pItem, string prefix) const = 0;
    virtual void setdef(unsigned char * pItem) const = 0;
    virtual void help(const unsigned char * pItem) const = 0;

    string getName() const {return name;}

};


// xxx should not be exported
typedef struct t_cm_context
{
    string                     str;
    const cm_item_descriptor * pDesc;
    unsigned char *            pItem;
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
                 unsigned short c,
                 unsigned int o):
                 pDesc(d), maxCount(c), offset(o){};

    const cm_item_descriptor * pDesc;     // the component's descriptor
    const unsigned short       maxCount;  // Max number of instances of the item
    const unsigned int         offset;    // Offset [bytes] of items (or pointer to items) within the composite item
    bool needIndex(const unsigned char * pParentItem) const {return getCount(pParentItem) > 1;}
    bool getIndex(int * pArgc, char *** pArgv, const unsigned char * pParentItem, unsigned int & itemIndex) const;
    virtual unsigned char * getFirstItem(const unsigned char * pParentItem) const = 0;
    virtual unsigned getCount(const unsigned char * pParentItem) const = 0;
    virtual bool isAddSupported() const = 0;
    virtual void setCount(unsigned char * pParentItem, unsigned int) const = 0;
};


// In a contained aggregate, component items are contained within the composite:
// the item memory is allocated along with that of the composite item, and the
// 'add' and 'del' operations can't be applied to the component.
class cm_contained_aggregate : public cm_aggregate
{
public:
    cm_contained_aggregate(const cm_item_descriptor * d,
                           unsigned short c,
                           unsigned int o):
                           cm_aggregate(d,c,o){}

    unsigned char * getFirstItem(const unsigned char * pParentItem) const;
    virtual unsigned getCount(const unsigned char * pParentItem) const;
    bool isAddSupported() const {return false;}
    void setCount(unsigned char * pParentItem, unsigned int) const {assert(isAddSupported());} // add operation doesn't apply

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
                       unsigned short c,
                       unsigned int o,
                       const cm_contained_aggregate * cntAggr):
                       cm_aggregate(d,c,o), pCounterAggr(cntAggr){}

    unsigned char * getFirstItem(const unsigned char * pParentItem) const;
    virtual unsigned getCount(const unsigned char * pParentItem) const;
    bool isAddSupported() const {return true;}
    void setCount(unsigned char * pParentItem, unsigned int) const;


};


// Composite item descriptor's contain a list of components.
// xxx methods (apart from constructor) are private (not for user), but config_manager is friend?
class cm_composite_item_descriptor : public cm_item_descriptor
{ 
    const cm_aggregate * const * aggrList;  // Array of pointers to aggregates (pointers, because abstract cm_aggregate can't be instantiated)
    const unsigned short         aggrCount; // Number of aggregates in the list (number of descriptors, not items)

    virtual void writeTlv(const unsigned char * pItem, unsigned char ** ppBuf) const;
    virtual cm_item_len getTlvLen(const unsigned char * pItem) const;
    virtual int loadFromTlv(FILE * fp, unsigned char * pItem) const;
    const cm_aggregate * getAggr(const char * name) const;
    const cm_aggregate * getAggr(cm_descriptor_id id) const;
    void getComponentItem(int * pArgc,
                          char *** pArgv,
                          cm_item_descriptor ** ppComponent,
                          unsigned char ** ppItem,
                          cm_context & ctxt) const;

public:    
    cm_composite_item_descriptor(char * name,
                                 cm_descriptor_id id,
                                 cm_item_len l,
                                 const cm_aggregate * const * aggrList,
                                 unsigned short aggrCount):
                                 cm_item_descriptor(name, id, l), // init base class
                                 aggrList(aggrList),         // init data member
                                 aggrCount(aggrCount)
                                 {};

    ~cm_composite_item_descriptor(){};
    void handleCmd(int argc, char *argv[], unsigned char * pItem, cm_context & ctxt) const;
    void print(const unsigned char * pItem, string prefix) const;
    void setdef(unsigned char * pItem) const;
    virtual void help(const unsigned char * pItem) const;
    void handleAdd(int argc, char *argv[], unsigned char * pItem) const;
    void add(unsigned char * pParentItem, const cm_aggregate * pAggr, unsigned int cnt) const;
    void del(int argc, char *argv[], unsigned char * pItem) const;

};


// xxx methods (apart from constructor) are private (not for user), but config_manager is friend?
class cm_simple_item_descriptor : public cm_item_descriptor
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
    const CM_PRT_FPTR    pPrt;

    virtual void writeTlv(const unsigned char * pItem, unsigned char ** ppBuf) const;
    virtual cm_item_len getTlvLen(const unsigned char * pItem) const;
    virtual int loadFromTlv(FILE * fp, unsigned char * pItem) const;

public:
        
    cm_simple_item_descriptor(char * name,
                              cm_descriptor_id id,
                              cm_item_len l,
                              CM_SET_FPTR sf,
                              CM_SETDEF_FPTR sdf,
                              CM_PRT_FPTR pf):
                              cm_item_descriptor(name, id, l),
                              pSet(sf),
                              pSetDef(sdf),
                              pPrt(pf){};
                              
    ~cm_simple_item_descriptor(){};

    void handleCmd(int argc, char *argv[], unsigned char * pItem, cm_context & ctxt) const;
    void print(const unsigned char * pItem, string prefix) const;
    void set(unsigned char * pItem, string val) const;
    void setdef(unsigned char * pItem) const;
    virtual void help(const unsigned char * pItem) const {cout << "len " << getLen() << endl;}
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
    static config_manager * getInstance();

    // xxx should only be accessible to friend classes
    void setCtxt(cm_context * pC) {pCtxt = pC;}


private:
    config_manager(){};
    void save();
    void load();

    static config_manager * instance;
    const cm_item_descriptor * base_desc;    
    unsigned char *            ramBase;
    cm_context * pCtxt;    // current context
    cm_context   tempCtxt; // context being updated to possibly replace current one
    cm_context   baseCtxt; // context representing the base, to return to


};

#endif // CFG_MAN_H

