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
    CM_HELP,       //
    CM_RESET_CTXT, // return context to top level
    CM_OP_NONE,

} eCmOp;

static eCmOp getOp(const char * word);
config_manager * config_manager::instance = NULL;


////////////////////////////////////////////////////////////////////////////////
//
// config_manager
//
////////////////////////////////////////////////////////////////////////////////

// Singleton's single access point
config_manager * config_manager::getInstance()
{
    if (instance == NULL)
    {
        instance = new config_manager();
    }
    return instance;
}


// Initialize config manager: allocate and populate item memory in RAM.
// xxx We could presumably do these things in the constructor, but
// having an init method gives us more flexibility in delaying certain
// actions until later; this allows us to create the CM early in the 
// init cycle, but delay malloc and NVRAM reads until later.
void config_manager::init(const cm_item_descriptor * desc)
{
    base_desc = desc;
    ramBase = (unsigned char *)malloc(base_desc->getLen());

    // Set counters to 0 and pointers to NULL so load can work correctly.
    memset(ramBase, 0, base_desc->getLen());
    
    // xxx initialize with a constructor
    baseCtxt.pDesc = base_desc;
    baseCtxt.str = "";
    baseCtxt.pItem = ramBase;
    // This could be done in the constructor, but I do it here to make
    // unit tests independent (since the constructor can't be forced
    // to run at the beginning of each unit tests).
    pCtxt = &baseCtxt;
    load();
}


// Execute command words entered by client on CLI.
void config_manager::handleCmd(int argc, char *argv[])
{
    // First treat the commands that are only applicable at the top level
    switch (getOp(argv[0]))
    {
        case CM_LOAD:
            return load();

        case CM_SAVE:
            return save();

        case CM_RESET_CTXT:
            pCtxt = &baseCtxt;
            return;

        default:
            break;
    }

    // Start with the current base, then add to it
    tempCtxt = *pCtxt;

    // Pass command that don't apply to CM as a whole, to current context for handling
    pCtxt->pDesc->handleCmd(argc, argv, pCtxt->pItem, tempCtxt);
}


// Modify context, i.e. current location in the tree of nodes
void config_manager::setCtxt(cm_context * pC)
{
    DBG_PRT("setCtxt\n");
    pCtxt = pC;
}


// Get a prompt string to display to user, representing the current context
const char * config_manager::getPromptString()
{
    return pCtxt->str.c_str();
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

    #ifdef DEBUG_PRINT
    for (int i = 0; i < tlvLen; i++)
    {
        printf("%02x\n", buf[i]);
    }
    #endif

    FILE * fp = fopen(CFG_FILE_NAME, "wb");  // open file for binary write

    fwrite(buf, 1, tlvLen, fp);
    fclose(fp);
}


// Load data in NVRAM, in TLV format, to configurable items in RAM.
void config_manager::load()
{
    FILE * fp = fopen(CFG_FILE_NAME, "rb");  // open file for binary read
    
    if (fp == NULL)
    {
        cout << "No config file." << endl;
        return;
    }

    cm_descriptor_id id;    
    fread(&id, sizeof(id), 1, fp);

    printf("Load id %#x\n", id);

    // xxx if top-level ID is unexpected, stop?

    // Before loading, thus allocating new memory, call setdef to free owned memory
    base_desc->setdef(ramBase);
    base_desc->loadFromTlv(fp, ramBase);

    // Reset context, since a reload re-allocates memory and makes current context invalid
    pCtxt = &baseCtxt;

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
//
 void cm_composite_item_descriptor::handleCmd(int argc,
                                              char *argv[],
                                              unsigned char * pItem,
                                              cm_context & ctxt) const
{
    DBG_PRT("composite::handleCmd: %s\n", argv[0]);
    switch (getOp(argv[0]))
    {
        case CM_OP_NONE:
        {
            cm_item_descriptor * pComponent; // Component of this composite identified by argc, argv

            getComponentItem(&argc, &argv, &pComponent, &pItem, ctxt);

            if (pComponent == NULL)
            {
                // Unhandled word(s): not a command, and also doesn't identify a component
                DBG_PRT("composite::handleCmd: unhandled\n");
                break;
            }
            if (argc > 0)
            {                
                // Pass the remainder of the command to the found component
                return pComponent->handleCmd(argc, argv, pItem, ctxt);
            }
            else
            {
                // We have a component, but there's nothing left of the command
                config_manager::getInstance()->setCtxt(&ctxt);
                return;
            }
        }
        break;
        
        case CM_ADD:
            // Remove the word 'add' and pass the remainder to the method
            return handleAdd(argc - 1, &(argv[1]), pItem);
            
        case CM_DEL:
            // Remove the word 'del' and pass the remainder to the method
            return handleDel(argc - 1, &(argv[1]), pItem);
            
        case CM_PRT:
            return print(pItem, "");

        case CM_SETDEF:
            return setdef(pItem);

        case CM_HELP:
            return help(pItem);

        default: 
            break;
    }

    // Don't use argv[0] here, because it may have been modified by getComponentItem
    cout << "Not handled in '" << name << "'" << endl;
}


// Try to add a component named by argc,argv to a composite.
// After verifying the operation is applicable, the item is added.
void cm_composite_item_descriptor::handleAdd(int argc, char *argv[], unsigned char * pItem) const
{
    DBG_PRT("handleAdd %s\n", argv[0]);

    if (argc != 1)
    {
        cout << argc << " parameters for 'add'." << endl;
        return;
    }

    const cm_aggregate * pAggr = getAggr(argv[0]);

    if (pAggr == NULL)
    {
        cout << "No item '" << argv[0] << "' in '" << name << "'." << endl;
        return;
    }
    
    if (!pAggr->isAddSupported())
    {
        cout<<"Add not supported for '"<<pAggr->pDesc->getName()<<"' in '"<<name<<"'."<< endl;
        return;
    }

    if (pAggr->getCount(pItem) >= pAggr->maxCount)
    {
        cout<<"Can't add '"<<pAggr->pDesc->getName()<<"' (max "<<pAggr->maxCount<<")."<<endl;
        return;
    }  
        
    add(pItem, pAggr);
    return;
}


// Add OWNED item.
// @pre Add operation is supported on pAggr, and counter is in range
// This allocates memory for the new item, sets it to default values,
// and increments the corresponding counter.
unsigned char * cm_composite_item_descriptor::add(unsigned char * pParentItem,
                                                  const cm_aggregate * pAggr) const
{
    // Reallocate memory, and save pointer in the same location
    unsigned cnt = pAggr->getCount(pParentItem);
    unsigned char ** ppItems = (unsigned char **)(pParentItem + pAggr->offset);

    uint8_t * pNewMem = (uint8_t *)realloc(*ppItems, (cnt + 1) * pAggr->pDesc->getLen());

    if (pNewMem == NULL)
    {
        cout << "No " << pAggr->pDesc->getLen() << " for " << pAggr->pDesc->getName() << endl;
        return NULL;
    }

    // Memory successfully allocated, so reference the (possibly new) memory
    *ppItems = pNewMem;

    unsigned char * pNewItem = pNewMem + cnt * pAggr->pDesc->getLen();

    // Initialize added item with default values. First memset to ensure
    // counters, which have no setdef fn, are 0 (also sets pointers to owned to NULL).
    memset(pNewItem, 0, pAggr->pDesc->getLen());
    pAggr->pDesc->setdef(pNewItem);

    DBG_PRT("add at %p\n", pNewMem);

    pAggr->setCount(pParentItem, cnt + 1);
    return pNewItem;
}


// Del an owned component named by argc,argv from a composite
void cm_composite_item_descriptor::handleDel(int argc, char *argv[], unsigned char * pItem) const
{
    DBG_PRT("handleDel %s\n", argv[0]);

    if ((argc != 1) && (argc != 2))
    {
        // Provide item name and, optionally, index
        cout << argc << " parameters for 'del'." << endl;
        return;
    }    

    const cm_aggregate * pAggr = getAggr(argv[0]);

    if (pAggr == NULL)
    {        
        cout << "No item '" << argv[0] << "' in '" << name << "'." << endl;
        return;
    }

    // Match found
    argc--;
    argv++;
    
    if (!pAggr->isAddSupported())
    {
        cout<<"Delete not supported for '"<<pAggr->pDesc->getName()<<"' in '"<<name<<"'."<< endl;
        return;
    }

    unsigned int cnt = pAggr->getCount(pItem); // number of items currently in array

    if (cnt == 0)
    {
        cout << "Currently no '" <<pAggr->pDesc->getName()<<"' in '"<<name<<"'."<< endl;
        return;
    }

    unsigned int itemIdx = 0; // If no explicit index is needed, use 0 offset

    if (pAggr->needIndex(pItem) && !pAggr->getIndex(&argc, &argv, pItem, itemIdx))
    {
        // An index is needed but couldn't be extracted from the command
        return;
    }

    if (itemIdx >= pAggr->getCount(pItem))
    {
        cout<<"Index "<<itemIdx<<" out of range (0.. "<<pAggr->getCount(pItem)-1<<")."<<endl;
        return;
    }

    return del(pItem, pAggr, itemIdx, cnt);
}


// Del OWNED item.
// @pre Add operation is supported on pAggr, and counter is in range
// This re-allocates the necessary memory, updates the counter if necessary,
// and sets the pointer to the memory to NULL if it's all been freed.
void cm_composite_item_descriptor::del(unsigned char * pParentItem,
                                       const cm_aggregate * pAggr,
                                       unsigned int itemIdx,
                                       unsigned int cnt) const
{
    // Reallocate memory, and save pointer in the same location
    unsigned char ** ppItems = (unsigned char **)(pParentItem + pAggr->offset);
    cm_item_len      componentLen = pAggr->pDesc->getLen();    

    DBG_PRT("del item base %p offset %d index %d len %d\n",
            pParentItem, pAggr->offset, itemIdx, componentLen);

    // Shift down items to occupy the memory vacated by deleted item
    memmove(*ppItems + itemIdx * componentLen,
            *ppItems + (itemIdx + 1) * componentLen,
            (cnt - itemIdx - 1) * componentLen);

    *ppItems = (unsigned char *)realloc(*ppItems, (cnt - 1) * componentLen);

    // xxx realloc should return NULL if memory to be allocated is 0, but it doesn't seem to...
    if (cnt == 1)
    {
        *ppItems = NULL;
    }

    return pAggr->setCount(pParentItem, cnt - 1);
}


/// Return total length of TLV item:
//  The number of bytes taken up by T + L + V.
cm_item_len cm_composite_item_descriptor::getTlvLen(const unsigned char * pItem) const
{
    cm_item_len tlvLen = sizeof(cm_descriptor_id) + sizeof(cm_item_len);
    
    // For each aggregate, and for each of the array of items under it...
    for (int i = 0; i < aggrCount; i++)
    {            
        const cm_aggregate *  pAggr      = aggrList[i];
        const unsigned char * pFirstItem = pAggr->getFirstItem(pItem);
        unsigned int          itemCount  = pAggr->getCount(pItem);

        for (unsigned j = 0; j < itemCount; j++)
        {
            tlvLen += pAggr->pDesc->getTlvLen(pFirstItem + j * pAggr->pDesc->getLen());
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
void cm_composite_item_descriptor::writeTlv(const unsigned char *pItem, unsigned char ** ppBuf) const
{
    // The L field in TLV excludes this item's T+L fields
    cm_item_len componentLen = getTlvLen(pItem) - sizeof(len) - sizeof(id);
    
    // First write own Type and Length
    memcpy(*ppBuf, &id, sizeof(id));             // write Type (i.e. the ID)
    *ppBuf += sizeof(id);                        // advance the memory pointer
    memcpy(*ppBuf, &componentLen, sizeof(len));  // write Length (of Value to follow)
    *ppBuf += sizeof(len);                       // advance the memory pointer
    
    // Now write V, which is the TLVs of all components
    for (int i = 0; i < aggrCount; i++)
    {            
        const cm_aggregate *  pAggr      = aggrList[i];
        const unsigned char * pFirstItem = pAggr->getFirstItem(pItem);
        unsigned int          itemCount  = pAggr->getCount(pItem);

        for (unsigned j = 0; j < itemCount; j++)
        {
            pAggr->pDesc->writeTlv(pFirstItem + j * pAggr->pDesc->getLen(), ppBuf);
        }
    }
}


// Calling function called this function because the ID matches,
// so it's not checked here again.
// This method reads L, and moves forward in the file by that many bytes,
// using what it finds in the file to initialize the object's configurable items.
// @return number of bytes read
//
// xxx after each read, check how much was read.
// xxx shouldn't the i < aggrCount test come BEFORE allocating memory?
//
unsigned int cm_composite_item_descriptor::loadFromTlv(FILE * fp, unsigned char * pItem) const
{
    cm_item_len  tlvLen;
    
    fread(&tlvLen, sizeof(tlvLen), 1, fp);
    cm_item_len bytesRead = sizeof(tlvLen);

    DBG_PRT("composite load %d bytes to %p\n", tlvLen, pItem);

    unsigned char * pFirstItem = NULL; // first item in an aggregate's array

    while (bytesRead < tlvLen) // xxx 
    {        
        unsigned int      itemIdx; // number of items read of a given type, i.e. offset in the item array
        cm_descriptor_id  compId, prevCompId;

        fread(&compId, sizeof(compId), 1, fp);
        bytesRead += sizeof(compId);

        DBG_PRT("load component ID %d\n", compId);

        const cm_aggregate * pAggr = getAggr(compId);

        if (pAggr != NULL)
        {            
            // Found a match, so delegate the reading to the corresponding component
            DBG_PRT("component '%s' matches, itemIdx %d\n", pAggr->pDesc->getName().c_str(), itemIdx);

            // When we start with a new item type (or the 1st one),
            // reset array index and pFirstItem
            if ((pFirstItem == NULL) || (compId != prevCompId))
            {
                pFirstItem = pAggr->getFirstItem(pItem);
                itemIdx = 0;
            }

            unsigned char * pCompItem;

            if (pAggr->isAddSupported())
            {
                pCompItem = add(pItem, pAggr); 
            }
            else
            {
                pCompItem = pFirstItem + itemIdx++ * pAggr->pDesc->getLen();
            }

            if (pCompItem != NULL)
            {
                bytesRead += pAggr->pDesc->loadFromTlv(fp, pCompItem);
            }
            else
            {
                // Couldn't allocate memory for the item, so skip the file contents
                bytesRead += skipTlvItem(fp);
            }

            // xxx itemIdx may not exceed pAggr->maxCount
        }
        else
        {            
            // ID read from file matches no component: skip over the unrecognized item
            cout << "Couldn't load unknown component ID " << compId << endl;
            bytesRead += skipTlvItem(fp);
        }
        DBG_PRT("composite '%s': %d read\n", name.c_str(), bytesRead);
        prevCompId = compId;
    }
    return bytesRead;
}


// Having read the Type (ID) of an item from the TLV file, skip over the L
// and V.
// @return number of bytes moved ahead in the file
//
unsigned int cm_composite_item_descriptor::skipTlvItem(FILE * fp) const
{
    cm_item_len tlvLen;
    
    fread(&tlvLen, sizeof(tlvLen), 1, fp);
    cm_item_len bytesRead = sizeof(tlvLen);

    fseek(fp, tlvLen, SEEK_CUR);
    bytesRead += tlvLen;

    return bytesRead;
}


// Delegate print command to components
// 
void cm_composite_item_descriptor::print(const unsigned char * pItem, string prefix) const
{
    char indexbuf[6]; // xxx big enough to avoid truncation in all cases?

    DBG_PRT("print composite %s len %d\n", name.c_str(), len);

    // For each component, and for each of the array of items under it...
    for (int i = 0; i < aggrCount; i++)
    {            
        const cm_aggregate *  pAggr      = aggrList[i];
        const unsigned char * pFirstItem = pAggr->getFirstItem(pItem);
        unsigned int          itemCount  = pAggr->getCount(pItem);

        for (unsigned j = 0; j < itemCount; j++)
        {
            if (pAggr->maxCount > 1)
            {
                // There's more than one item, so print the index to distinguish among them
                snprintf(indexbuf, sizeof(indexbuf), " %d", j);
            }
            else
            {
                // There's only one item, so we needn't print an index
                indexbuf[0] = 0;
            }
            pAggr->pDesc->print(pFirstItem + j * pAggr->pDesc->getLen(),
                                prefix + pAggr->pDesc->getName() + indexbuf + " ");
        }
    }
}


// Delegate setdef command to components
// Preconditions: item contains valid data, i.e. if there are OWNED
// components, the corresponding counter > 0 (so we can know to free them).
//
// For OWNED components, free owned memory before setting
// the corresponding counter to 0.
// This means that we should not clear the counter first
// (which is why no setdef fn is installed for a counter), since pAggr->getCount
// for an owned component depends on the counter still being set.
void cm_composite_item_descriptor::setdef(unsigned char * pItem) const
{    
    // For each component, and for each of the array of items under it...
    for (int i = 0; i < aggrCount; i++)
    {            
        const cm_aggregate * pAggr      = aggrList[i];
        unsigned char *      pFirstItem = pAggr->getFirstItem(pItem);
        unsigned int         itemCount  = pAggr->getCount(pItem);

        // Set each item to default
        for (unsigned j = 0; j < itemCount; j++)
        {           
            pAggr->pDesc->setdef(pFirstItem + j * pAggr->pDesc->getLen());
        }

        // If necessary, free the block of memory where the items were, and set counter to 0
        if ((itemCount > 0) && pAggr->isAddSupported())
        {
            unsigned char ** ppItems = (unsigned char **)(pItem + pAggr->offset);

            DBG_PRT("setdef free %p\n", *ppItems);

            assert(*ppItems != NULL);
            free(*ppItems);
            *ppItems = NULL;
            pAggr->setCount(pItem, 0);
        }
    }
}


// Give name of each component
void cm_composite_item_descriptor::help(const unsigned char * pItem) const
{
    for (int i = 0; i < aggrCount; i++)
    {   
        const cm_aggregate * pAggr = aggrList[i];

        cout << pAggr->pDesc->getName() << " [" << pAggr->getCount(pItem);

        if (pAggr->isAddSupported())
        {
            cout << "/" << pAggr->maxCount;
        }
        cout << "]" << endl;
    }
}


// Look for the aggregate whose component has a matching name
const cm_aggregate * cm_composite_item_descriptor::getAggr(const char * name) const
{
    for (int i = 0; i < aggrCount; i++)
    {            
        const cm_aggregate * pAggr = aggrList[i];

        if (strcmp(name, pAggr->pDesc->getName().c_str()) == 0)
        {
            return pAggr;
        }
    }
    return NULL;
}


// Look for the aggregate whose component has a matching ID
const cm_aggregate * cm_composite_item_descriptor::getAggr(cm_descriptor_id id) const
{
    for (int i = 0; i < aggrCount; i++)
    {            
        const cm_aggregate * pAggr = aggrList[i];

        if (pAggr->pDesc->id == id)
        {
            return pAggr;
        }
    }
    return NULL;
}


// From remaining command-line words, find matching component of this composite.
// If the component does not exist, it may be created in certain cases.
// pArgc: (input/output) number of command words
// pArgv: (input/output) command word pointer
// ppComponent: output, the wanted component, or 0 if command identifies none
// ppItem: on input, the owning item
//         on output, the wanted item
// 
//
void cm_composite_item_descriptor::getComponentItem(int * pArgc,
                                                    char *** pArgv,
                                                    cm_item_descriptor ** ppComponent,
                                                    unsigned char ** ppItem,
                                                    cm_context & ctxt) const
{
    *ppComponent = NULL; // By default, found nothing
    const cm_aggregate * pAggr = getAggr(*pArgv[0]);

    if (pAggr == NULL)
    {
        return;
    }

    ctxt.str += pAggr->pDesc->getName() + " ";
    
    // Found matching name: now try to get index from next word
    *pArgc -= 1;
    *pArgv += 1;
    unsigned int itemIdx = 0; // If no index needed, use offset 0

    if (pAggr->maxCount > 1)
    {
        // Explicit index is needed if there can be more than one instance
        if (pAggr->getIndex(pArgc, pArgv, *ppItem, itemIdx))
        {
            // Index available, it becomes part of the context string
            char indexbuf[6]; // xxx big enough to avoid truncation in all cases?

            snprintf(indexbuf, sizeof(indexbuf), "%d ", itemIdx);
            ctxt.str = ctxt.str + indexbuf;
        }
        else
        {
            // The necessary index was not in the command
            return;
        }
    }

    unsigned int cnt = pAggr->getCount(*ppItem); // Number of items currently in the aggregate

    DBG_PRT("getComponentItem %p offset %d idx %d cnt %d len %d\n",
            *ppItem, pAggr->offset, itemIdx, cnt, pAggr->pDesc->getLen());

    if (itemIdx >= cnt)
    {
        // Index refers to an item that doesn't exist
        if (pAggr->isAddSupported() && (itemIdx == cnt) && (itemIdx < pAggr->maxCount))
        {
            // Index refers to an item to create
            add(*ppItem, pAggr);
        }
        else
        {
            cout<<"Index "<<itemIdx<<" out of range"<<endl;
            return;
        }
    }

    *ppComponent = (cm_item_descriptor *)pAggr->pDesc; // xxx fix constness issues: no typecasting should be needed here
    *ppItem = pAggr->getFirstItem(*ppItem) + itemIdx * pAggr->pDesc->getLen();
    ctxt.pDesc = *ppComponent;
    ctxt.pItem = *ppItem;
}


////////////////////////////////////////////////////////////////////////////////
//
// cm_simple_item_descriptor
//
////////////////////////////////////////////////////////////////////////////////

// An item does not print its own name, since
// it may be preceded by an index, which is known
// to the item's composite but not to the item.
void cm_simple_item_descriptor::print(const unsigned char * pItem, string prefix) const
{
    cout << prefix << "= ";

    DBG_PRT("print simple %s len %d at %p\n", name.c_str(), len, pItem);
    
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


////////////////////////////////////////////////////////////////////////////////
//
// cm_basic_item_descriptor
//
////////////////////////////////////////////////////////////////////////////////

//
// argc number of items in argv
// argv array of strings containing name elements
// pItem - pointer to RAM at which item is located
//
void cm_basic_item_descriptor::handleCmd(int argc,
                                         char *argv[],
                                         unsigned char * pItem,
                                         cm_context & ctxt) const
{
    DBG_PRT("simple cmd at %p\n", pItem);
    
    switch (getOp(argv[0]))
    {
        case CM_PRT:
            return print(pItem, "");

        case CM_SET:
            return set(pItem, argv[1]);

        case CM_SETDEF:
            return setdef(pItem);

        case CM_HELP:
            return help(pItem);

        default:
            cout << "'" << argv[0] << "' not handled by basic item '" << name << "'" << endl;
    }   
}


// Set.
void cm_basic_item_descriptor::set(unsigned char * pItem, string val) const
{
    DBG_PRT("set simple %s at %p to value %s\n",
            name.c_str(), pItem, val.c_str());

    if (pSet != NULL)
    {
        pSet(pItem, len, val);
    }
    else
    {
        cout << "'" << name << "' can't be set." << endl;
    }
    cout << endl;
}


// Set configurable item to its default value.
void cm_basic_item_descriptor::setdef(unsigned char * pItem) const
{
    if (pSetDef != NULL)
    {
        pSetDef(pItem, len);
    }
    else
    {
        // The fefault default is all bits set to 0
        memset(pItem, 0, len);
    }
}


/// Write TLV to a buffer, and advance the ptr to the end of memory written to.
//  This is useful for writing to a RAM buffer first, for subsequent write
//  to NVRAM.
//  xxx if we want to write directly to NVRAM, we need to implement a method
//  that does that...
void cm_basic_item_descriptor::writeTlv(const unsigned char *pItem, unsigned char ** ppBuf) const
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
cm_item_len cm_basic_item_descriptor::getTlvLen(const unsigned char * pItem) const
{
    return sizeof(cm_descriptor_id) + sizeof(cm_item_len) + getLen();
}


// Calling function called this function because the ID matches,
// so it's not checked here again.
// This method reads L, and moves forward in the file by that many bytes,
// using what it finds in the file to initialize the object's configurable items.
// xxx after each read, check how much was read.
unsigned int cm_basic_item_descriptor::loadFromTlv(FILE * fp, unsigned char * pItem) const
{
    cm_item_len tlvLen;
    
    fread(&tlvLen, sizeof(tlvLen), 1, fp);

    DBG_PRT("load simple %d bytes to %p\n", tlvLen, pItem);

    if (tlvLen != getLen())
    {
        // Item larger than expected: we don't truncate, we leave
        // the item as unchanged, but move forward in the file.
        cout << "TLV len " << tlvLen << ", expected " << getLen() << endl;

        fseek(fp, tlvLen, SEEK_CUR);
    }

    // xxx Could we handle the case where len > tlvLen?  Is there a reasonable
    // action to perform in this case?  Yes, but it would be dependent on
    // endian-ness, and for big-endian systems we'd have to know if we were
    // reading an integer or not.
    fread(pItem, tlvLen, 1, fp);

    return sizeof(tlvLen) + tlvLen;
}


////////////////////////////////////////////////////////////////////////////////
//
// cm_cntr_item_descriptor
//
////////////////////////////////////////////////////////////////////////////////

//
// argc number of items in argv
// argv array of strings containing name elements
// pItem - pointer to RAM at which item is located
//
void cm_cntr_item_descriptor::handleCmd(int argc,
                                        char *argv[],
                                        unsigned char * pItem,
                                        cm_context & ctxt) const
{
    DBG_PRT("cntr cmd at %p\n", pItem);
    
    switch (getOp(argv[0]))
    {
        case CM_PRT:
            return print(pItem, "");

        case CM_HELP:
            return help(pItem);

        default:
            cout << "'" << argv[0] << "' not handled by counter '" << name << "'" << endl;
    }   
}


////////////////////////////////////////////////////////////////////////////////
//
// cm_aggregate
//
////////////////////////////////////////////////////////////////////////////////

// Utility method to extract in index from an array of command words
// Returns false if unable to extract a valid (in-range) index
// Returns true if returning a valid (in-range) index.
//
bool cm_aggregate::getIndex(int * pArgc,
                            char *** pArgv,
                            const unsigned char * pParentItem,
                            unsigned int & itemIdx) const
{
    if (*pArgc > 0)
    {
        char * pEnd; // pointer to char after chars accepted by strtoul

        // An index is needed, so try to extract one
        itemIdx = strtoul((*pArgv)[0], &pEnd, 0);

        if (pEnd > (*pArgv)[0])
        {
            *pArgc -= 1;
            *pArgv += 1;
            return true;
        }
    }

    cout << "'" << pDesc->getName() << "' needs index." <<endl;
    return false;
}


////////////////////////////////////////////////////////////////////////////////
//
// cm_contained_aggregate
//
////////////////////////////////////////////////////////////////////////////////

// Return address of the first item in the item array.
// pParentItem: pointer to parent item; from this the aggregate obtains the
//              address of the first item in the array that it links to the parent.
//
unsigned char * cm_contained_aggregate::getFirstItem(const unsigned char * pParentItem) const
{
    return (unsigned char *)(pParentItem + offset);
}


// Return the number of items in the component's array.
// For a contained component, the count is fixed at maxCount.
unsigned cm_contained_aggregate::getCount(const unsigned char * pParentItem) const
{
    return maxCount;
}


////////////////////////////////////////////////////////////////////////////////
//
// cm_owned_aggregate
//
////////////////////////////////////////////////////////////////////////////////

// Return address of the first item in the item array.
// pParentItem: pointer to parent item; from this the aggregate obtains the
//              address of the first item in the array that it links to the parent.
//
unsigned char * cm_owned_aggregate::getFirstItem(const unsigned char * pParentItem) const
{
    return *(unsigned char **)(pParentItem + offset); // location is a pointer to the OWNED item
}


// Return the number of items in the component's array
// xxx giving a fixed size to counters would simplify this, but
// introduces a dependency on the application programmer doing the right thing
unsigned cm_owned_aggregate::getCount(const unsigned char * pParentItem) const
{
    if (pCounterAggr == NULL)
    {
        // If there's no counter, then count is just 0 (absence) or 1 (presence)
        return (getFirstItem(pParentItem) == NULL) ? 0 : 1;
    }
    
    uint32_t c;

    switch (pCounterAggr->pDesc->getLen()) 
    { 
        case sizeof(uint8_t):
            {
                uint8_t v;

                memcpy(&v, pParentItem + pCounterAggr->offset, sizeof(v));
                c = v;
            }
            break;
       
        case sizeof(uint16_t):
            { 
                uint16_t v;

                memcpy(&v, pParentItem + pCounterAggr->offset, sizeof(v));
                c = v;
            }
            break;
       
        case sizeof(uint32_t):
            memcpy(&c, pParentItem + pCounterAggr->offset, sizeof(c)); 
            break;

        default:
            assert(0);
    }
    return c;
}


// Set value in RAM that records the number of items in the array of items
// xxx enforce, run-time or compile-time, that counters are unsigned int sized.
void cm_owned_aggregate::setCount(unsigned char * pParentItem, unsigned int count) const
{
    // Sanity check: if add/del operation not supported, the setCount() is meaningless
    assert(isAddSupported());

    if (pCounterAggr == NULL)
    {
        // There may not be a counter -- don't need one if maxCount == 0
        return;
    }   
    memcpy(pParentItem + pCounterAggr->offset, &count, sizeof(count));
}


// Helper function that returns what kind of operation (if any) a word is
eCmOp getOp(const char * word)
{
    if (strcmp(word, "add") == 0)     return CM_ADD;
    if (strcmp(word, "del") == 0)     return CM_DEL;
    if (strcmp(word, "prt") == 0)     return CM_PRT;
    if (strcmp(word, "=") == 0)       return CM_SET;
    if (strcmp(word, "setdef") == 0)  return CM_SETDEF;
    if (strcmp(word, "load") == 0)    return CM_LOAD;
    if (strcmp(word, "save") == 0)    return CM_SAVE;
    if (strcmp(word, "<") == 0)       return CM_RESET_CTXT;
    if (strcmp(word, "?") == 0)       return CM_HELP;

    // If no match, it's not an operation
    return CM_OP_NONE;
}

