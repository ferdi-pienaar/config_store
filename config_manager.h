
#ifndef CFG_MAN_H
#define CFG_MAN_H

#include <stdint.h> // uint8_t, etc
#include <iostream>
using namespace std;

// xxx throughout I've provisionally avoided the use of references; revise this.
// xxx All descriptors, maybe also components, should be const.
// xxx revise the name component; it's just too confusing => aggregate? container?


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

// Function pointers -- types registered by user when initializing CM
typedef void (*CM_READ_FROM_NVRAM)(unsigned char *pBuf, cm_item_len * pLen);
typedef void (*CM_WRITE_TO_NVRAM)(const unsigned char *pBuf, cm_item_len len);

// Function pointers -- types registered by user when descriptor is created
typedef void (*CM_SET_FPTR)(unsigned char *pItem, cm_item_len len, string val);
typedef void (*CM_SETDEF_FPTR)(unsigned char *pItem, cm_item_len len);
typedef void (*CM_PRT_FPTR)(const unsigned char *pItem, cm_item_len len);


// We eliminate the getItem method, and pass the command string recursively down the
// hierarchy of descriptors, until we either consume the whole command
// or reach a command keyword (set, setdef, prt, add, del).
// 'save' and 'load' commands are intercepted by the CM itself, since they
// have global applicability only.
//
// The recursion code should then be present in only 1 place:
// cm_composite_item_descriptor::do_cmd
//  Following commands are executed by a simple:    set, setdef, prt
//  Following commands are executed by a composite: setdef, add, del, prt

// Descriptor of configurable item (either simple or compound)
// xxx methods are private (not for user), but config_manager is friend?
class cm_item_descriptor
{
private:
    
protected: // xxx these are protected because I want to access them from the derived classes
    string           name;     // name by which item is addressed on CLI
    cm_descriptor_id id;       // ID (unique within the context of the component's context) of item in NVRAM    
    cm_item_len len;  // Number of bytes occupied by an item in RAM


public: 
    cm_item_descriptor(string iname, cm_descriptor_id iid, cm_item_len ilen):
                       name(iname), id(iid), len(ilen){}
    virtual ~cm_item_descriptor(){}

    virtual void do_cmd(int argc, char *argv[], unsigned char * pItem) = 0;
    virtual cm_item_len getLen(){return len;}
    virtual cm_item_len getTlvLen(unsigned char * pItem) = 0;
    virtual void writeTlv(unsigned char * pItem, unsigned char ** ppBuf) = 0;
    virtual void print(unsigned char * pItem, string prefix) = 0;
    virtual void setdef(unsigned char * pItem) = 0;

    string getName(){return name;}

};

///
// The way in which a cm_item_descriptor forms part of a composite.
// Within a composite descriptor, there's one of these for each
// component descriptor (i.e. one for each array of component items).
//  It may be CONTAINED or OWNED.
//  There may be one or more instances (i.e. single item or an array of items).
// xxx we could embed this class in cm_composite_item_descriptor, but then
// client could not create component lists at init.  The constructor for this
// class has to be exposed to the client programmer.
// Perhaps all members should be private, with cm_composite_item_descriptor
// as friend, since it has to read (but not write) them.
// 
//
class cm_component
{
protected:
    
public:
    cm_component(cm_item_descriptor * d,
                 unsigned short c,
                 unsigned int o):
                 pDesc(d), maxCount(c), offset(o){};

    cm_item_descriptor * pDesc;     // the component's descriptor
    unsigned short       maxCount;  // Max number of instances of the item
    unsigned int         offset;    // Offset [bytes] of items (or pointer to items) within the composite item

    bool getIndex(int & argc, char ** & argv, unsigned char * pParentItem, unsigned int & itemIndex);
    virtual unsigned char * getFirstItem(unsigned char * pParentItem) = 0;
    virtual unsigned getCount(unsigned char * pParentItem) = 0;
    virtual bool isAddSupported() = 0;
    virtual void setCount(unsigned char * pParentItem, unsigned int) = 0;
};


// Component items are contained within the composite: the item memory is allocated along
// with that of the composite item, and the 'add' and 'del' operations don't apply.
class cm_contained_component : public cm_component
{
public:
    cm_contained_component(cm_item_descriptor * d,
                           unsigned short c,
                           unsigned int o):
                           cm_component(d,c,o){}

    unsigned char * getFirstItem(unsigned char * pParentItem);
    virtual unsigned getCount(unsigned char * pParentItem);
    bool isAddSupported(){return false;}
    void setCount(unsigned char * pParentItem, unsigned int){assert(isAddSupported());} // add operation doesn't apply

};

// Component items are owned but not contained by the composite: the item memory is allocated
// by an 'add' operation and freed by a 'del' operation.  By default, the number
// of items is 0.
class cm_owned_component : public cm_component
{
private:
    cm_contained_component * pCounterComp; // the counter for this owned component
    
public:
    cm_owned_component(cm_item_descriptor * d,
                       unsigned short c,
                       unsigned int o,
                       cm_contained_component * cComp):
                       cm_component(d,c,o), pCounterComp(cComp){}

    unsigned char * getFirstItem(unsigned char * pParentItem);
    virtual unsigned getCount(unsigned char * pParentItem);
    bool isAddSupported(){return true;}
    void setCount(unsigned char * pParentItem, unsigned int);


};


// Composite item descriptor's contain a list of components.
// xxx methods (apart from constructor) are private (not for user), but config_manager is friend?
class cm_composite_item_descriptor : public cm_item_descriptor
{ 
    cm_component ** compList;           // List of ptrs to components, an array
    unsigned short  compCount;          // Number of components in the list (number of descriptors, not items)

    virtual void writeTlv(unsigned char * pItem, unsigned char ** ppBuf);
    virtual cm_item_len getTlvLen(unsigned char * pItem);    


public:    
    cm_composite_item_descriptor(char * name,
                                 cm_descriptor_id id,
                                 cm_item_len l,
                                 cm_component ** componentList,
                                 unsigned short componentCount):
                                 cm_item_descriptor(name, id, l), // init base class
                                 compList(componentList),         // init data member
                                 compCount(componentCount)
                                 {};

    ~cm_composite_item_descriptor(){};
    void do_cmd(int argc, char *argv[], unsigned char * pItem);
    void print(unsigned char * pItem, string prefix);
    void setdef(unsigned char * pItem);
    void add(int argc, char *argv[], unsigned char * pItem);
    void del(int argc, char *argv[], unsigned char * pItem);

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
    CM_SET_FPTR    pSet;
    CM_SETDEF_FPTR pSetDef;
    CM_PRT_FPTR    pPrt;

    virtual void writeTlv(unsigned char * pItem, unsigned char ** ppBuf);
    virtual cm_item_len getTlvLen(unsigned char * pItem);
    

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

    void do_cmd(int argc, char *argv[], unsigned char * pItem);
    void print(unsigned char * pItem, string prefix);
    void set(unsigned char * pItem, string val);
    void setdef(unsigned char * pItem);
};


// xxx Should be a singleton?
class config_manager
{  
public:
    config_manager(cm_item_descriptor * pDesc);
    void do_cmd(int argc, char *argv[]);
    void init(CM_READ_FROM_NVRAM pRead, CM_WRITE_TO_NVRAM pWr);

private:
    void save();
    void load();
    CM_READ_FROM_NVRAM pReadFromNvram; // fn installed by user to do read for CM
    CM_WRITE_TO_NVRAM  pWriteToNvram;  // fn installed by user to do write for CM 
    
    cm_item_descriptor * base_desc;    
    unsigned char *      ramBase; // xxx initialize during init

    // Current context
    string               contextString;
    cm_item_descriptor * contextDesc;
    unsigned char *      contextRam;
    
};

#endif // CFG_MAN_H

