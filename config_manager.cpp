#include "config_manager.h"
#include <sstream>
using namespace std;


typedef enum
{
    CM_ADD,
    CM_DEL,
    CM_PRT,
    CM_SET,
    CM_SETDEF,
    CM_LOAD,
    CM_SAVE,    
    CM_OP_NONE,

} eCmOp;

static eCmOp getOp(char * word);


config_manager::config_manager(cm_item_descriptor * desc)
{
    base_desc = desc;
}


// xxx calls setdef for base descriptor, then load
void config_manager::init(void)
{
    ramBase = (unsigned char *)malloc(base_desc->getLen());  
}

// Execute command words from client.
void config_manager::do_cmd(int argc, char *argv[])
{
    int i;
    eCmOp op = CM_OP_NONE;
    
    for (i = 0; i < argc; i++)
    {
        cout << i << ": " << argv[i] << "." << endl;

        if (getOp(argv[i]) != CM_OP_NONE)
        {
            break;
        }
    }


    if (op != CM_OP_NONE)
    {
        cout << "command at " << i << endl;
    }

    unsigned char      * pItem;
    cm_item_descriptor * pDesc;

    if (1)
    {
        pDesc = base_desc;
        pItem  = ramBase;
        
        // xxx for certain ops, find the item and descriptor to which to apply the op
        // xxx in some cases, we might not want to start
        base_desc->do_cmd(argc, argv, pItem);
    }

    switch (op)
    {
        case CM_LOAD:
            cout << "load at " << i;
            break;

        case CM_SAVE:
            cout << "save at " << i;
            break;

        default:
            break;
    }
}

////////////////////////////////////////////////////////////////////////////////
//
// cm_composite_item_descriptor
//
////////////////////////////////////////////////////////////////////////////////

//
// argc number of items in argv
// argv array of strings containing name elements
// pItem - pointer to RAM at which item is located
//
void cm_composite_item_descriptor::do_cmd(int argc,
                                          char *argv[],
                                          unsigned char * pItem)
{
    eCmOp op = getOp(argv[0]);

    if (op == CM_OP_NONE)
    {
        // If we haven't reached an operation-word in the input, look for the component to which it may apply
        for (int i = 0; i < compCount; i++)
        {
            cm_component * pComp = &(compList[i]);

            if (pComp->count > 1)
            {
            }
        }
    }

    switch (op)
    {
        case CM_ADD:
            break;
        case CM_DEL:
            break;
            
        case CM_PRT:
            print(pItem, "");
            break;

        case CM_SET:
            cout << "'Set' operation not applicable to composite item " << getName() << endl;
            break;

        case CM_SETDEF:
            break;

        default:
            cout << "Internal error op " << op;
    }
}


// Traverse the items in xxx
// pItem: pointer to current item
// @return pointer to next item
void cm_composite_item_descriptor::firstItem(unsigned char * pThisItem)
{
    compIndex = 0;
    pItem = pThisItem;
    compList[compIndex].firstItem(pItem);
}

// Returns next component item, along with the applicable descriptor.
// These may members of successive arrays, each handled by a different component object.
// xxx this should return a name (the index) too, rather than having a separate getCurrentItemName method.
//
void cm_composite_item_descriptor::getNextItem(cm_item_descriptor ** ppDesc, int * pIdx, unsigned char ** ppItem)
{
    cm_component * pComp = &(compList[compIndex]);

    *ppDesc = pComp->pDesc;
    *ppItem = pComp->getNextItem(pIdx);

    if (pComp->isLastItem())
    { 
        // No more items in this component; go to next
        pComp = &(compList[++compIndex]);
        pComp->firstItem(pItem);
    }
}

// The last item of a composite has been reached when we've
// reached the last item in the last component aggregation.
bool cm_composite_item_descriptor::isLastItem()
{
    return (compIndex == compCount) && compList[compIndex].isLastItem();
}


cm_item_len cm_composite_item_descriptor::getTlvLen()
{
    firstItem(pItem);

    // xxx traverse components
}

/// Write item's TLV to a buffer, and advance the ptr to the end of memory written to.
//  This is useful for writing to a RAM buffer first, for subsequent write
//  to NVRAM.
//  xxx if we want to write directly to NVRAM, we need to implement a method
//  that does that...
void cm_composite_item_descriptor::writeTlv(unsigned char *pItem, unsigned char ** ppBuf)
{
    
}

// Delegate print command to components
// 
void cm_composite_item_descriptor::print(unsigned char * pItem, string prefix)
{
    int                  itemIndex;        
    cm_item_descriptor * pCompDesc;
    unsigned char *      pCompItem;

    char indexbuf[4];

    cout << "xxx debug: " << "print composite " << name << " with len " << len << endl;

    for(firstItem(pItem); !isLastItem(); getNextItem(&pCompDesc, &itemIndex, &pCompItem))
    {

        snprintf(indexbuf, sizeof(indexbuf), "%d", itemIndex);
        pCompDesc->print(pCompItem, prefix + pCompDesc->getName() + " " + indexbuf + " ");
    }
}


////////////////////////////////////////////////////////////////////////////////
//
// cm_simple_item_descriptor
//
////////////////////////////////////////////////////////////////////////////////

//
// argc number of items in argv
// argv array of strings containing name elements
// pItem - pointer to RAM at which item is located
//
void cm_simple_item_descriptor::do_cmd(int argc,
                                       char *argv[],
                                       unsigned char * pItem)
{
    
    
}

// An item does not print its own name, since
// it may be preceded by an index, which is known
// to the item's composite but not to the item.
void cm_simple_item_descriptor::print(unsigned char * pItem, string prefix)
{
    cout << prefix;

    cout << "xxx debug: " << "print simple " << name << " with len " << len << endl;
    
    if (pPrt == NULL)
    {
        // No function installed so default print function: hex chars
        for (int i = 0; i < len; i++)
        {
            printf("%02x", pItem[i]);
        }
    }
    else
    {
        pPrt(pItem, len);
    }
    cout << endl;
}

void cm_simple_item_descriptor::setdef(unsigned char * pItem)
{
    if (pSetDef == NULL)
    {
        // No function installed, so use default default value: 0
        memset(pItem, 0, len);
    }
    else
    {
        pSetDef(pItem, len);
    }
}

/// Write TLV to a buffer, and advance the ptr to the end of memory written to.
//  This is useful for writing to a RAM buffer first, for subsequent write
//  to NVRAM.
//  xxx if we want to write directly to NVRAM, we need to implement a method
//  that does that...
void cm_simple_item_descriptor::writeTlv(unsigned char *pItem, unsigned char ** ppBuf)
{
    **ppBuf = id;                      // write Type (i.e. the ID)
    *ppBuf += sizeof(id);              // advance the memory pointer
    **ppBuf = getTlvLen();             // write Length
    *ppBuf += sizeof(cm_item_len);     // advance the memory pointer

    memcpy(*ppBuf, pItem, len);
    *ppBuf += len;
    
}

/// Return total length of TLV item:
//  The number of bytes taken up by T + L + V.
cm_item_len cm_simple_item_descriptor::getTlvLen()
{
    return sizeof(cm_descriptor_id) + sizeof(cm_item_len) + getLen();
}


////////////////////////////////////////////////////////////////////////////////
//
// cm_component
//
////////////////////////////////////////////////////////////////////////////////

// Traverse the items in an array.
// pItem: pointer to current item
// @return pointer to next item
void cm_component::firstItem(unsigned char * pParentItem)
{
    if (type == CONTAINED)
    {
        itemIndex = 0;                 // initialize counter member used in getNextItem
        pItem = pParentItem + offset;
    }
    else
    {
        itemIndex = 0;                                    // initialize counter member used in getNextItem
        pItem = (unsigned char *)*(pParentItem + offset); // location is a pointer to the OWNED item
        assert(0);
    }
}

// Get next item in array owned by component.
// Also return the index if pIdx is not NULL; this is used e.g. when printing the index before the item value.
//
unsigned char * cm_component::getNextItem(int * pIdx)
{
    unsigned char * ret = pItem;

    if (pIdx != NULL)
    {
        *pIdx = itemIndex;
    }
    itemIndex++;
    pItem += pDesc->getLen();
    return ret;
}

bool cm_component::isLastItem()
{
    if (type == CONTAINED)
    {    
        return (itemIndex == count);
    }
    else
    {        
        // xxx Wrong -- Here, count is dynamic, and saved in a previous CONTAINED item
        return (itemIndex == count);
    }
}


// During iteration, return name of current item -- by default, just convert the iteration index
// to a string, but xxx
#if 0
string cm_component::getCurrentItemName()
{
}
#endif


// Helper function that returns what kind of operation (if any) a word is
eCmOp getOp(char * word)
{
    if (strcmp(word, "add") == 0) return CM_ADD;
    if (strcmp(word, "del") == 0) return CM_DEL;
    if (strcmp(word, "prt") == 0) return CM_PRT;
    if (strcmp(word, "set") == 0) return CM_SET;
    if (strcmp(word, "setdef") == 0) return CM_SETDEF;
    if (strcmp(word, "load") == 0) return  CM_LOAD;
    if (strcmp(word, "save") == 0) return CM_SAVE;

    // If no match, it's not an operation
    return CM_OP_NONE;
}

