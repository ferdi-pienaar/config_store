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
    CM_HELP,     // xxx
    CM_OP_NONE,

} eCmOp;

static eCmOp getOp(char * word);


////////////////////////////////////////////////////////////////////////////////
//
// config_manager
//
////////////////////////////////////////////////////////////////////////////////

// xxx Install the NVRAMwrite function.
config_manager::config_manager(cm_item_descriptor * desc)
{
    base_desc = desc;
}


// Initialize config manager: allocate and populate item memory in RAM.
// xxx We could presumably do these things in the constructor, but
// having an init method gives us more flexibility in delaying certain
// actions until later; this allows us to create the CM early in the 
// init cycle, but delay malloc and NVRAM reads until later.
void config_manager::init(CM_READ_FROM_NVRAM pRead, CM_WRITE_TO_NVRAM pWr)
{
    ramBase = (unsigned char *)malloc(base_desc->getLen());

    base_desc->setdef(ramBase);

    pWriteToNvram = pWr;
    pReadFromNvram = pRead;

    //load(ramBase);
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

    FILE * fp;
    fp = fopen("cfg.bin", "wb");  // open file for binary write

    fwrite(buf, 1, tlvLen, fp);

    fclose(fp);
}


// Load data in NVRAM, in TLV format, to configurable items in RAM.
void config_manager::load()
{
    cout << "load." << endl;

    FILE * fp;
    if ((fp = fopen("cfg.bin", "rb")) == NULL)  // open file for binary read
    {
        cout << "No config file." << endl;
        return;
    }

    cm_descriptor_id id;
    cm_item_len      tlvLen;
    
    fread(&id, sizeof(id), 1, fp);
    fread(&tlvLen, sizeof(tlvLen), 1, fp);

    printf("id %x len %d\n", id, tlvLen);

    // xxx if top-level ID is unexpected, stop?
    
    fclose(fp);
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
            cm_component * pComp = compList[i];

            if (strcmp(argv[0], pComp->pDesc->getName().c_str()) == 0)
            {            
                // Found matching name, now determine if the next word should be an index
                argc--;
                argv++;

                unsigned int itemIdx;

                if (pComp->getIndex(argc, argv, pItem, itemIdx) == false)
                {
                    // An index is needed but couldn't be extracted from the command
                    return;
                }

                DBG_PRT("cmd item base %p offset %d index %d len %d\n", pItem, pComp->offset, itemIdx, pComp->pDesc->getLen());

                // Pass the remainder of the command to the matching component
                return pComp->pDesc->do_cmd(argc, argv, pComp->getFirstItem(pItem) + itemIdx * pComp->pDesc->getLen());
            }
        }
    }

    // If we're here, it means argv[0] was an operation, or there was no match to next part of item id
    switch (op)
    {
        case CM_ADD:
            // Remove the word 'add' and pass the remainder to the method
            return add(argc - 1, &(argv[1]), pItem);
            
        case CM_DEL:
            // Remove the word 'del' and pass the remainder to the method
            return del(argc - 1, &(argv[1]), pItem);
            
        case CM_PRT:
            return print(pItem, "");

        case CM_SETDEF:
            return setdef(pItem);

        default:
            break;
    }
    // If we're here, failed to consume argv[0]
    cout << "'" << argv[0] << "' operation not applicable to composite item '" << getName() << "'" << endl;
}

// Add a component to a composite.
// This allocates memory for the new item, sets it to default values,
// and increments the corresponding counter.
void cm_composite_item_descriptor::add(int argc, char *argv[], unsigned char * pItem)
{
    DBG_PRT("add %s\n\r", argv[0]);

    if (argc != 1)
    {
        cout << argc << " parameters for 'add'." << endl;
        return;
    }

    for (int i = 0; i < compCount; i++)
    {            
        cm_component *  pComp = compList[i];

        if (strcmp(argv[0], pComp->pDesc->getName().c_str()) == 0)
        {        
            if (!pComp->isAddSupported())
            {
                cout<<"Add not supported for '"<<pComp->pDesc->getName()<<"' in '"<<getName()<<"'."<< endl;
                return;
            }

            unsigned int cnt = pComp->getCount(pItem); // number of items currently in array

            if (cnt >= pComp->maxCount)
            {
                cout<<"Can't add '"<<pComp->pDesc->getName()<<"' (max "<<pComp->maxCount<<")."<<endl;
                return;
            }  
                
            // Reallocate memory, and save pointer in the same location
            unsigned char ** ppItems = (unsigned char **)(pItem + pComp->offset);

            *ppItems = (unsigned char *)realloc(*ppItems, (cnt + 1) * pComp->pDesc->getLen());

            // Initialize added item with default values. First memset to ensure
            // counters, which have no setdef fn, are 0 (also sets pointers to owned to NULL).
            memset(*ppItems + cnt * pComp->pDesc->getLen(), 0, pComp->pDesc->getLen());
            pComp->pDesc->setdef(*ppItems + cnt * pComp->pDesc->getLen());

            DBG_PRT("add at %p\n", *ppItems);

            return pComp->setCount(pItem, cnt + 1);
        }
    }
    cout << "No item '" << argv[0] << " in '" << getName() << "'." << endl;
}

// Del an owned component from a composite
// xxx When all items deleted, set pointer to owned mem to NULL for later sanity checks?
void cm_composite_item_descriptor::del(int argc, char *argv[], unsigned char * pItem)
{
    DBG_PRT("del %s\n\r", argv[0]);

    if ((argc != 1) && (argc != 2))
    {
        // Provide item name and, optionally, index
        cout << argc << " parameters for 'del'." << endl;
        return;
    }    

    // Find matching component name
    for (int i = 0; i < compCount; i++)
    {            
        cm_component *  pComp = compList[i];

        if (strcmp(argv[0], pComp->pDesc->getName().c_str()) == 0)
        {
            argc--;
            argv++;
            
            if (!pComp->isAddSupported())
            {
                cout<<"Delete not supported for '"<<pComp->pDesc->getName()<<"' in '"<<getName()<<"'."<< endl;
                return;
            }

            unsigned int cnt = pComp->getCount(pItem); // number of items currently in array

            if (cnt == 0)
            {
                cout << "'Currently no '" <<pComp->pDesc->getName()<<"' in '"<<getName()<<"'."<< endl;
                return;
            }

            unsigned int itemIdx;

            if (pComp->getIndex(argc, argv, pItem, itemIdx) == false)
            {
                // An index is needed but couldn't be extracted from the command
                return;
            }

            // Reallocate memory, and save pointer in the same location
            unsigned char ** ppItems = (unsigned char **)(pItem + pComp->offset);

            // Shift down items to occupy the memory vacated by deleted item
            memcpy(*ppItems + itemIdx * pComp->pDesc->getLen(),
                   *ppItems + (itemIdx + 1) * pComp->pDesc->getLen(),
                   (cnt - itemIdx - 1) * pComp->pDesc->getLen());

            *ppItems = (unsigned char *)realloc(*ppItems, (cnt - 1) * pComp->pDesc->getLen());

            DBG_PRT("del item base %p offset %d index %d len %d\n", pItem, pComp->offset, itemIdx, pComp->pDesc->getLen());

            return pComp->setCount(pItem, cnt - 1);
        }
    }
}


/// Return total length of TLV item:
//  The number of bytes taken up by T + L + V.
cm_item_len cm_composite_item_descriptor::getTlvLen(unsigned char * pItem)
{
    cm_item_len tlvLen = sizeof(cm_descriptor_id) + sizeof(cm_item_len);
    
    // For each component, and for each of the array of items under it...
    for (int i = 0; i < compCount; i++)
    {            
        cm_component *  pComp = compList[i];
        unsigned char * pFirstItem = pComp->getFirstItem(pItem);
        unsigned int    itemCount = pComp->getCount(pItem);

        for (unsigned j = 0; j < itemCount; j++)
        {
            tlvLen += pComp->pDesc->getTlvLen(pFirstItem + j * pComp->pDesc->getLen());
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
        cm_component *  pComp = compList[i];
        unsigned char * pFirstItem = pComp->getFirstItem(pItem);
        unsigned int    itemCount = pComp->getCount(pItem);

        for (unsigned j = 0; j < itemCount; j++)
        {
            pComp->pDesc->writeTlv(pFirstItem + j * pComp->pDesc->getLen(), ppBuf);
        }
    }
}


// Delegate print command to components
// 
void cm_composite_item_descriptor::print(unsigned char * pItem, string prefix)
{
    char indexbuf[6]; // xxx big enough to avoid truncation in all cases?

    DBG_PRT("print composite %s with len %d\n", name.c_str(), len);

    // For each component, and for each of the array of items under it...
    for (int i = 0; i < compCount; i++)
    {            
        cm_component *  pComp = compList[i];
        unsigned char * pFirstItem = pComp->getFirstItem(pItem);
        unsigned int    itemCount = pComp->getCount(pItem);

        for (unsigned j = 0; j < itemCount; j++)
        {
            if (pComp->getCount(pItem) > 1)
            {
                // There's more than one item, so print the index to distinguish among them
                snprintf(indexbuf, sizeof(indexbuf), " %d", j);
            }
            else
            {
                // There's only one item, so we needn't print an index
                indexbuf[0] = 0;
            }
            pComp->pDesc->print(pFirstItem + j * pComp->pDesc->getLen(), prefix + pComp->pDesc->getName() + indexbuf + " ");
        }
    }
}


// Delegate setdef command to components
// For OWNED components, free owned memory before setting
// the corresponding counter to 0.
// This means that we should not clear the counter first
// (which is why no setdef is installed for a counter), since pComp->getCount
// for an owned component depends on the counter stil being set.
// xxx set pointer to owned mem to NULL for later sanity checks?
void cm_composite_item_descriptor::setdef(unsigned char * pItem)
{    
    // For each component, and for each of the array of items under it...
    for (int i = 0; i < compCount; i++)
    {            
        cm_component *  pComp = compList[i];
        unsigned char * pFirstItem = pComp->getFirstItem(pItem);
        unsigned int    itemCount = pComp->getCount(pItem);

        for (unsigned j = 0; j < itemCount; j++)
        {           
            pComp->pDesc->setdef(pFirstItem + j * pComp->pDesc->getLen());

            // After setting to default, free owned memory and set counter to 0
            if (pComp->isAddSupported() && (pComp->getCount(pItem) > 0))
            {
                DBG_PRT("setdef free %p\n" pItem + pComp->offset);

                free(pItem + pComp->offset);

                return pComp->setCount(pItem, 0);
            }
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
// xxx for owned counters, no modification should be allowed.
// But we should check that for a counter, no setdef or set
// is installed.
void cm_simple_item_descriptor::setdef(unsigned char * pItem)
{
    if (pSetDef != NULL)
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

// Utility function to extract in index from an array of command words
// Returns false if unable to extract an index.
// Returns true of able to return an index
// If no index is required, the index is set to 0, and true is returned.
bool cm_component::getIndex(int & argc, char ** & argv, unsigned char * pParentItem, unsigned int & itemIdx)
{      
    // If there's more than one item in the array, an index must be provided
    if (getCount(pParentItem) <= 1)
    {
        // No index is needed, since there are 1 or 0 items present
        itemIdx = 0;
        return true;
    }

    // An index is needed
    char * pEnd;
    
    itemIdx = strtoul(argv[0], &pEnd, 0);

    if (pEnd == argv[0])
    {
        cout << "'" << pDesc->getName() << "' needs index." <<endl;
        return false;
    }

    if (itemIdx >= getCount(pParentItem))
    {
        cout<<"'"<<pDesc->getName()<<"' index out of range (there are "<<getCount(pParentItem)<<")."<<endl;
        return false;
    }
    
    argc--;
    argv++;

    return true;
}

////////////////////////////////////////////////////////////////////////////////
//
// cm_contained_component
//
////////////////////////////////////////////////////////////////////////////////

// Traverse the items in the array.
// pParentItem: pointer to parent item; from this component can calculate
//              the addresses of the items it links to the composite.
//
unsigned char * cm_contained_component::getFirstItem(unsigned char * pParentItem)
{
   return pParentItem + offset;
}


// Return the number of items in the component's array.
// For a contained component, the count is fixed at maxCount.
unsigned cm_contained_component::getCount(unsigned char * pParentItem)
{
    return maxCount;
}


////////////////////////////////////////////////////////////////////////////////
//
// cm_owned_component
//
////////////////////////////////////////////////////////////////////////////////

// Traverse the items in the array.
// pParentItem: pointer to parent item; from this component can calculate
//              the addresses of the items it links to the composite.
//
unsigned char * cm_owned_component::getFirstItem(unsigned char * pParentItem)
{
    return *(unsigned char **)(pParentItem + offset); // location is a pointer to the OWNED item
}


// Return the number of items in the component's array
// xxx giving a fixed size to counters would simplify this, but
// introduces a dependency on the application programmer doing the right thing
unsigned cm_owned_component::getCount(unsigned char * pParentItem)
{
    unsigned int c;

    switch (pCounterComp->pDesc->getLen()) 
    { 
        case 1:
            {
                unsigned char v;

                memcpy(&v, pParentItem + pCounterComp->offset, sizeof(v));
                c = v;
            }
            break;
       
        case 2:
            { 
                unsigned short v;

                memcpy(&v, pParentItem + pCounterComp->offset, sizeof(v));
                c = v;
            }
            break;
       
        case 4:
            memcpy(&c, pParentItem + pCounterComp->offset, sizeof(c)); 
            break;

        default:
            assert(0);
       
    }
    return c;
}

// Set value in RAM that records the number of items in the array of items
// xxx enforce, run-time of compile-time, that counters are unsigned int sized.
void cm_owned_component::setCount(unsigned char * pParentItem, unsigned int count)
{
    // Sanity check: if add/del operation not supported, the setCount() is meaningless
    assert(isAddSupported());
    
    memcpy(pParentItem + pCounterComp->offset, &count, sizeof(count));
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

