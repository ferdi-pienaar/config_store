
#ifndef CFG_MAN_H
#define CFG_MAN_H

#include <stdint.h>
#include <iostream>
using namespace std;


// xxx one of the problems with the earlier version of this code that I'd like
// to avoid this time is requiring the application developer to know that
// a component counter has to precede an OWNED component or component array.

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
typedef void (*CM_PRT_FPTR)(unsigned char *pItem, cm_item_len len);


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
    virtual void writeTlv(unsigned char * pItem, unsigned char ** ppBuf) = 0;    
    
protected: // xxx these are protected because I want to access them from the derived classes
    string           name;     // name by which item is addressed on CLI
    cm_descriptor_id id;       // ID (unique within the context of the component's context) of item in NVRAM    
    cm_item_len len;  // Number of bytes occupied by an item in RAM

    virtual cm_item_len getTlvLen() = 0;

public:        


    virtual cm_item_len getLen(){return len;}

    cm_item_descriptor(string iname, cm_descriptor_id iid, cm_item_len ilen):
          name(iname), id(iid), len(ilen){}
    virtual ~cm_item_descriptor(){}

    virtual void do_cmd(int argc, char *argv[], unsigned char * pItem) = 0;

    virtual void print(unsigned char * pItem, string prefix) = 0;
    string getName(){return name;}

    
};

// The way in which a cm_item_descriptor forms part of a composite.
//  It may be CONTAINED or OWNED.
//  There may be one or more instances (i.e. single item or an array of items).
// xxx Should CONTAINED or OWNED be handled as classes?
// xxx we could embed this class in cm_composite_item_descriptor, but then
// client could not create component lists at init.  The constructor for this
// class has to be exposed to the client programmer.
// Perhaps all members should be private, with cm_composite_item_descriptor
// as friend, since it has to read (but not write) them.
class cm_component
{
    unsigned int    itemIndex;  // index used when traversing the array of component items 
    unsigned char * pItem;      // pointer used to traverse array (base value can change during add, del & setdef!)
    
public:
    typedef enum
    {
        CONTAINED,  // component items are part of the composite (memory allocated at same time)
        OWNED       // component items are referenced by their composite
    }component_type;

    virtual void startItem(unsigned char * pItem);
    virtual unsigned char * getNextItem(int * pIdx);
    virtual bool checkLastItem();

    cm_component(component_type t,
                 cm_item_descriptor * d,
                 unsigned short c,
                 unsigned int o):
                 type(t), pDesc(d), count(c), offset(o){};

    component_type       type;   //    
    cm_item_descriptor * pDesc;  // the component's descriptor
    unsigned short       count;  // (Max) number of instances of the item
    unsigned int         offset; // Offset of items (or pointer to items) within the composite

};



// Composite item descriptor's contain a list of components.
// xxx methods (apart from constructor) are private (not for user), but config_manager is friend?
class cm_composite_item_descriptor : public cm_item_descriptor
{
    unsigned int    compIndex; // index used when traversing pCompLost 
    unsigned char * pItem;     // current item in RAM (not fixed, since there may be several items per descriptor)

    
    cm_component  * compList;           // List of components (simple or composite), an array
    unsigned short  compCount;          // Number of components in the list (number of descriptors, not items)

    void startItem(unsigned char * pParentItem);
    void getNextItem(cm_item_descriptor ** ppDesc, int * pIdx, unsigned char ** ppItem);
    bool checkLastItem();
    virtual void writeTlv(unsigned char * pItem, unsigned char ** ppBuf);
    virtual cm_item_len getTlvLen();    



public:    
    cm_composite_item_descriptor(char * name,
                                 cm_descriptor_id id,
                                 cm_item_len l,
                                 cm_component * componentList,
                                 unsigned short componentCount):
                                 cm_item_descriptor(name, id, l), // init base class
                                 compList(componentList),         // init data member
                                 compCount(componentCount)
                                 {};

    ~cm_composite_item_descriptor(){};
    void do_cmd(int argc, char *argv[], unsigned char * pItem);
    void print(unsigned char * pItem, string prefix);

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
    virtual cm_item_len getTlvLen();
    

public:
    #if 1
    // Further subclassing?  Two types of behaviour: if COUNT, the value is
    // used by CM as a counter.
    // The derived class
    #else
    typedef enum
    {
        STANDARD,    // item has no meaning to config manager
        OWNED_COUNT, // item is used by CM as counter for following array of OWNED components
        NAME         // item is name distinguishing an item from others in the array (instead of idx)
    }item_type;
    #endif
        
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
    void set(unsigned char * pItem, string str);
    void setdef(unsigned char * pItem);
};


// xxx Should be a singleton?
class config_manager
{  
public:
    config_manager(cm_item_descriptor * pDesc);
    void do_cmd(int argc, char *argv[]);

    void init();

private:
    
    cm_item_descriptor * base_desc;
    unsigned char *      ramBase; // xxx initialize during init

    // Current context
    string               contextString;
    cm_item_descriptor * contextDesc;
    unsigned char *      contextRam;
    
};

#endif // CFG_MAN_H

