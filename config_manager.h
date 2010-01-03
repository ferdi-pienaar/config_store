
#ifndef CFG_MAN_H
#define CFG_MAN_H

#include <stdint.h>
#include <iostream>
using namespace std;


// xxx len (for TLV) should be a typedef so it could be modified
// xxx type (for TLV) should be a typedef so it could be modified

// xxx one of the problems with the earlier version of this code that I'd like
// to avoid this time is requiring the application developer to know that
// a component counter has to precede an OWNED component or component array.

// Number of bytes in an item; used in NVRAM
typedef unsigned short cm_item_len;

// Identifier ID, unique within its context, used to identify it in NVRFAM
typedef unsigned short cm_descriptor_id;


// Descriptor of configurable item (either simple or compound)
// xxx methods are private (not for user), but config_manager is friend?
class cm_item_descriptor
{

    string           name;     // name by which item is addressed on CLI
    cm_descriptor_id id;       // ID (unique within the context of the component's context) of item in NVRAM

public:
    // xxx remove this default constructor once the derived classes are complete
    cm_item_descriptor(){}
    cm_item_descriptor(string iname, cm_descriptor_id iid){name = iname; id = iid;}
    virtual ~cm_item_descriptor(){}
    virtual void print(unsigned char * pRam){}; // composite items use their components to help them do this
};


// The way in which a cm_item_descriptor forms part of a composite.
//  It may be CONTAINED or OWNED.
//  There may be one or more instances (i.e. single item or an array of items).
// xxx Should CONTAINED or OWNED be handled as classes?
class cm_component
{
public:
    typedef enum
    {
        CONTAINED,  // items are part of the composite (memory allocated at same time)
        OWNED       // component items are referenced by their composite
    }component_type;

private:

    component_type       type;   //    
    cm_item_descriptor * pDesc;   // the item's descriptor
    unsigned short       count;  // (Max) number of instances of the item
    unsigned int         offset; // Offset of items (or pointer to items) within the composite

public:
    
    cm_component(component_type t,
                 cm_item_descriptor * d,
                 unsigned short c,
                 unsigned int o):
                 type(t), pDesc(d), count(c), offset(o){};
};


// Composite item descriptor's contain a list of components.
class cm_composite_item_descriptor : public cm_item_descriptor
{
    cm_component  * pComp;                // List of components (simple or composite), an array
    unsigned short  count;                // Number of components in the list;

public:    
    cm_composite_item_descriptor(char * name,
                                 cm_descriptor_id id,
                                 cm_component * componentList,
                                 unsigned short componentCount):
                                 cm_item_descriptor(name, id), // init base class
                                 pComp(componentList),         // init data member
                                 count(componentCount)
                                 {};

    ~cm_composite_item_descriptor(){};

    void print(unsigned char * pRam){}; // composite items use their components to help them do this

};


// Only simple items can be set.
// xxx methods are private (not for user), but config_manager is friend?
class cm_simple_item_descriptor : public cm_item_descriptor
{
    cm_item_len len;  // Number of bytes occupied by an item in RAM

public:
    cm_simple_item_descriptor(char * name,
                              cm_descriptor_id id):
                              cm_item_descriptor(name, id){};
    ~cm_simple_item_descriptor(){};


    void set(string);

    void print(unsigned char * pRam){};

};


// xxx Should be a singleton?
class config_manager
{  
    public:
    config_manager(cm_item_descriptor * pDesc);
    void do_cmd(int argc, char *argv[]);

    cm_item_descriptor * base_desc;
};

#endif // CFG_MAN_H

