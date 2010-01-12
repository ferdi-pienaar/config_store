#include "config_manager.h"
#include <sstream>
using namespace std;

#ifdef DEBUG_PRINT
#define DBG_PRT(fmt,...) printf(fmt, ##__VA_ARGS__)
#else
#define DBG_PRT(fmt,...)
#endif

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


////////////////////////////////////////////////////////////////////////////////
//
// config_manager
//
////////////////////////////////////////////////////////////////////////////////


config_manager::config_manager(cm_item_descriptor * desc)
{
    base_desc = desc;
}


// Initialize config manager: allocate and populate item memory in RAM.
//
void config_manager::init(void)
{
    ramBase = (unsigned char *)malloc(base_desc->getLen());

    base_desc->setdef(ramBase);

    // base_desc->load(ramBase);
}


// Execute command words entered by client on CLI.
void config_manager::do_cmd(int argc, char *argv[])
{
    unsigned char      * pItem;
    cm_item_descriptor * pDesc;


    // First treat the commands that are only applicable at the top level
    switch (getOp(argv[0]))
    {
        case CM_LOAD:
            load();
            return;

        case CM_SAVE:
            save();
            return;

        default:
            break;
    }

    pDesc = base_desc;
    pItem  = ramBase;
    
    // Pass command to top level item for handling
    base_desc->do_cmd(argc, argv, pItem);
}


// Save data in RAM to NVRAM, in TLV format.
void config_manager::save()
{
    cm_item_len tlvLen = base_desc->getTlvLen(ramBase);

    
    // Allocate a temporary buffer to contain TLV format data
    unsigned char * buf = new unsigned char[tlvLen];

    // xxx debug
    memset(buf, 0x5a, tlvLen);

    cout << "Allocated buffer " << tlvLen << endl;

    unsigned char * pBuf = buf; // this copy is modified by writeTlv

    base_desc->writeTlv(ramBase, &pBuf);

    //  xxx debug
    for (int i = 0; i < tlvLen; i++)
    {
        printf("%02x\n", buf[i]);
    }
}


// Load data in NVRAM, in TLV format, to configurable items in RAM.
void config_manager::load()
{
    cout << "load." << endl;
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
 void cm_composite_item_descriptor::do_cmd(int argc, char *argv[], unsigned char * pItem)
{
    eCmOp op = getOp(argv[0]);

    if (op == CM_OP_NONE)
    {
        // We haven't reached an operation-word, so pass command to component item
        for (int i = 0; i < compCount; i++)
        {            
            cm_component * pComp = &(compList[i]);
            int            itemIndex = 0;

            if (strcmp(argv[0], pComp->pDesc->getName().c_str()) == 0)
            {
                // Found matching name, now determine if the next word should be an index
                // xxx this works only for CONTAINED components...
                argc--;
                argv++;

                
                
                if (pComp->count > 1)
                {
                    char * pEnd;
                    
                    itemIndex = strtol(argv[0], &pEnd, 0);

                    if (pEnd == argv[0])
                    {
                        cout << "'"<<pComp->pDesc->getName()<<"' needs index."<<endl;
                        return;
                    }

                    if (itemIndex >= pComp->count)
                    {
                        cout << "'"<<pComp->pDesc->getName()<<"' index out of range."<<endl;
                        return;
                    }
                    
                    argc--;
                    argv++;
                }

                DBG_PRT("cmd item base %p offset %d index %d len %d\n", pItem, pComp->offset, itemIndex, pComp->pDesc->getLen());

                // Pass the remainder of the command to the matching component
                return pComp->pDesc->do_cmd(argc, argv, pItem + pComp->offset + itemIndex * pComp->pDesc->getLen());
            }
        }
    }

    switch (op)
    {
        case CM_ADD:
            // xxx implement
            break;
            
        case CM_DEL:
            // xxx implement
            break;
            
        case CM_PRT:
            print(pItem, "");
            break;

        case CM_SET:
            cout << "'Set' operation not applicable to composite item '" << getName() << "'" << endl;
            break;

        case CM_SETDEF:
            setdef(pItem);
            break;

        default:
            cout<<"Unknown command or item '"<<argv[0]<<"'"<<endl;
    }
}

//
// Return the length in bytes of a TLV item.
//
cm_item_len cm_composite_item_descriptor::getTlvLen(unsigned char * pItem)
{
    cm_item_len tlvLen = sizeof(cm_descriptor_id) + sizeof(cm_item_len);
    
    // For each component, and for each of the array of items under it...
    for (int i = 0; i < compCount; i++)
    {            
        cm_component * pComp = &(compList[i]);

        for (pComp->firstItem(pItem); !pComp->isLastItem(); pComp->nextItem())
        {
            tlvLen += pComp->pDesc->getTlvLen(pComp->getCurrentItem());
        }
    }    
    return tlvLen;
}


/// Write item's TLV to a buffer, and advance the ptr to the end of memory written to.
//  This is useful for writing to a RAM buffer first, for subsequent write
//  to NVRAM.
//  xxx add param, bufsize, to do a buffer overflow check.
//  xxx if we want to write directly to NVRAM, we need to implement a method
//  that does that...
void cm_composite_item_descriptor::writeTlv(unsigned char *pItem, unsigned char ** ppBuf)
{
    // The L field in TLV excludes this item's T+L fields
    cm_item_len componentLen = getTlvLen(pItem) - sizeof(len) - sizeof(id);
    
    // First write own Type and Length
    memcpy(*ppBuf, &id, sizeof(id));             // write Type (i.e. the ID)
    *ppBuf += sizeof(id);                        // advance the memory pointer
    memcpy(*ppBuf, &componentLen, sizeof(len));  // write Length (of Value to follow)
    *ppBuf += sizeof(len);                       // advance the memory pointer
    
    // Now write V, which is the TLVs of all components
    for (int i = 0; i < compCount; i++)
    {            
        cm_component * pComp = &(compList[i]);

        for (pComp->firstItem(pItem); !pComp->isLastItem(); pComp->nextItem())
        {
            pComp->pDesc->writeTlv(pComp->getCurrentItem(), ppBuf);
        }
    }
}


// Delegate print command to components
// 
void cm_composite_item_descriptor::print(unsigned char * pItem, string prefix)
{
    char indexbuf[4];

    DBG_PRT("print composite %s with len %d\n", name.c_str(), len);

    // For each component, and for each of the array of items under it...
    for (int i = 0; i < compCount; i++)
    {            
        cm_component * pComp = &(compList[i]);

        int itemIndex = 0;

        for (pComp->firstItem(pItem); !pComp->isLastItem(); pComp->nextItem())
        {
            // xxx need pComp->indexNeeded to know if we an index to print, or during command parsing.
            snprintf(indexbuf, sizeof(indexbuf), "%d", itemIndex++);
            pComp->pDesc->print(pComp->getCurrentItem(), prefix + pComp->pDesc->getName() + " " + indexbuf + " ");
        }
    }
}

// Delegate setdef command to components
// xxx TBD: for OWNED components, free owned memory before setting
// the corresponding counter to 0.
void cm_composite_item_descriptor::setdef(unsigned char * pItem)
{    
    // For each component, and for each of the array of items under it...
    for (int i = 0; i < compCount; i++)
    {            
        cm_component * pComp = &(compList[i]);

        for (pComp->firstItem(pItem); !pComp->isLastItem(); pComp->nextItem())
        {
            pComp->pDesc->setdef(pComp->getCurrentItem());
        }
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
void cm_simple_item_descriptor::do_cmd(int argc, char *argv[], unsigned char * pItem)
{
    DBG_PRT("simple cmd at %p\n", pItem);
    
    switch (getOp(argv[0]))
    {
        case CM_OP_NONE:
        case CM_ADD:
        case CM_DEL:
            cout << argv[0] << " is not an operation supported by " << getName() << endl;
            break;
            
        case CM_PRT:
            print(pItem, "");
            break;

        case CM_SET:
            set(pItem, argv[1]);
            break;

        case CM_SETDEF:
            setdef(pItem);
            break;

        default:
            cout << "Unknown operation '" << argv[0] << "'" << endl;
    }
    
}

// An item does not print its own name, since
// it may be preceded by an index, which is known
// to the item's composite but not to the item.
void cm_simple_item_descriptor::print(unsigned char * pItem, string prefix)
{
    cout << prefix;

    DBG_PRT("print simple %s with len %d at %p\n", name.c_str(), len, pItem);
    
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


// Set.
void cm_simple_item_descriptor::set(unsigned char * pItem, string val)
{
    DBG_PRT("set simple %s at %p to value %s\n", name.c_str(), pItem, val.c_str());

    if (pSet != NULL)
    {
        pSet(pItem, len, val);
    }
    else
    {
        cout << "'" << getName() << "' can't be set." << endl;
    }
    cout << endl;
}

// Set configurable item to its default value.
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
    memcpy(*ppBuf, &id, sizeof(id));   // write Type (i.e. the ID)
    *ppBuf += sizeof(id);              // advance the memory pointer
    memcpy(*ppBuf, &len, sizeof(len)); // write Length
    *ppBuf += sizeof(len);             // advance the memory pointer

    memcpy(*ppBuf, pItem, len);        // write Value
    *ppBuf += len;
}

/// Return total length of TLV item:
//  The number of bytes taken up by T + L + V.
//  (For a simple item, there's no dependency on pItem, the RAM contents.)
cm_item_len cm_simple_item_descriptor::getTlvLen(unsigned char * pItem)
{
    return sizeof(cm_descriptor_id) + sizeof(cm_item_len) + getLen();
}


////////////////////////////////////////////////////////////////////////////////
//
// cm_component
//
////////////////////////////////////////////////////////////////////////////////

// Traverse the items in the array.
// pParentItem: pointer to parent item; from this component can calculate
//              the addresses of the items it links to the composite.
//
void cm_component::firstItem(unsigned char * pParentItem)
{
    if (type == CONTAINED)
    {
        itemIndex = 0;                 // initialize counter member used in getNextItem
        pFirstItem = pParentItem + offset;
    }
    else
    {
        itemIndex = 0;                                    // initialize counter member used in getNextItem
        pFirstItem = *(unsigned char **)(pParentItem + offset); // location is a pointer to the OWNED item
        assert(0);
    }
}

// Get next item in array handled by component.
//
const unsigned char * cm_component::nextItem()
{
    return pFirstItem + (pDesc->getLen() * itemIndex++);
}

unsigned char * cm_component::getCurrentItem()
{
    return pFirstItem + (pDesc->getLen() * itemIndex);
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

