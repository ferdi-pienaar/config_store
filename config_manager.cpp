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
    CM_RESET_CTXT, // return context to top level
    CM_OP_NONE,

} eCmOp;

static eCmOp getOp(const char * word);


////////////////////////////////////////////////////////////////////////////////
//
// config_manager
//
////////////////////////////////////////////////////////////////////////////////

// 
config_manager::config_manager(const cm_item_descriptor * desc)
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

    // Set counters to 0 and pointers to NULL so load can work correctly.
    memset(ramBase, 0, base_desc->getLen());
    
    pWriteToNvram = pWr;
    pReadFromNvram = pRead;

    load();
}

// Reset context to top level
void config_manager::reset_ctxt()
{
    ctxt.str   = base_desc->getName();
    ctxt.pDesc = base_desc;
    ctxt.pItem = ramBase;
}


// Execute command words entered by client on CLI.
void config_manager::do_cmd(int argc, char *argv[])
{
    // First treat the commands that are only applicable at the top level
    switch (getOp(argv[0]))
    {
        case CM_LOAD:
            return load();

        case CM_SAVE:
            return save();

        case CM_RESET_CTXT:
            return reset_ctxt();

        default:
            break;
    }

    // Pass command that don't apply to CM as a whole, to current context for handling
    ctxt.pDesc->do_cmd(argc, argv, ctxt.pItem, ctxt);
}


// Get a prompt string to display to user, representing the current context
const char * config_manager::getPromptString()
{
    return ctxt.str.c_str();
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

    FILE * fp;
    fp = fopen("cfg.bin", "wb");  // open file for binary write

    fwrite(buf, 1, tlvLen, fp);

    fclose(fp);
}


// Load data in NVRAM, in TLV format, to configurable items in RAM.
void config_manager::load()
{
    FILE * fp;
    if ((fp = fopen("cfg.bin", "rb")) == NULL)  // open file for binary read
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
    reset_ctxt();

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
 void cm_composite_item_descriptor::do_cmd(int argc,
                                           char *argv[],
                                           unsigned char * pItem,
                                           cm_context & ctxt) const
{
    eCmOp op = getOp(argv[0]);

    if (op == CM_OP_NONE)
    {
        // We haven't reached an operation-word, so pass command to component item
        for (int i = 0; i < aggrCount; i++)
        {            
            const cm_aggregate * pAggr = aggrList[i];

            if (strcmp(argv[0], pAggr->pDesc->getName().c_str()) != 0)
            {
                // No match -- continue to next candidate
                continue;
            }
            
            // Found matching name: now try to get index from next word
            unsigned int itemIdx;
            cm_aggregate::CM_GET_INDEX_RESULT indexRes = pAggr->getIndex(argc - 1, argv + 1, pItem, itemIdx);

            if (indexRes == cm_aggregate::CM_FAILED)
            {
                return;
            }

            DBG_PRT("cmd pItem %p offset %d idx %d len %d\n", pItem, pAggr->offset, itemIdx, pAggr->pDesc->getLen());

            if (((indexRes == cm_aggregate::CM_GOT) && (argc == 2)) ||
                ((indexRes == cm_aggregate::CM_NO_NEED) && (argc == 1)))
            {
                // End of input
                // An item has been identified, so it becomes the context.
                ctxt.pDesc = pAggr->pDesc;
                ctxt.pItem = pAggr->getFirstItem(pItem) + itemIdx * pAggr->pDesc->getLen();

                ctxt.str  += " ";
                ctxt.str  += argv[0];

                if (indexRes == cm_aggregate::CM_GOT)
                {
                    // Add index string, if there was one
                    ctxt.str  += " ";
                    ctxt.str  += argv[1];                    
                }
                return;
            }

            if (indexRes == cm_aggregate::CM_GOT)
            {            
                // Advance if there was an index in the command
                argc--;
                argv++;
            }

            // Pass the remainder of the command to the matching component
            return pAggr->pDesc->do_cmd(argc-1,
                                        argv+1,
                                        pAggr->getFirstItem(pItem) + itemIdx * pAggr->pDesc->getLen(),
                                        ctxt);
        }
    }

    // If we're here, it means argv[0] was an operation, or there was no match to next word in item id
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

        case CM_HELP:
            return help(pItem);

        default:
            break;
    }
    // If we're here, failed to consume argv[0]
    cout << "'" << argv[0] << "' operation not applicable to composite item '" << getName() << "'" << endl;
}


// Add a component named by argc,argv to a composite.
// This allocates memory for the new item, sets it to default values,
// and increments the corresponding counter.
void cm_composite_item_descriptor::add(int argc, char *argv[], unsigned char * pItem) const
{
    DBG_PRT("add %s\n\r", argv[0]);

    if (argc != 1)
    {
        cout << argc << " parameters for 'add'." << endl;
        return;
    }

    for (int i = 0; i < aggrCount; i++)
    {            
        const cm_aggregate * pAggr = aggrList[i];

        if (strcmp(argv[0], pAggr->pDesc->getName().c_str()) != 0)
        {
            // No match, continue to next candidate
            continue;
        }
        
        if (!pAggr->isAddSupported())
        {
            cout<<"Add not supported for '"<<pAggr->pDesc->getName()<<"' in '"<<getName()<<"'."<< endl;
            return;
        }

        unsigned int cnt = pAggr->getCount(pItem); // number of items currently in array

        if (cnt >= pAggr->maxCount)
        {
            cout<<"Can't add '"<<pAggr->pDesc->getName()<<"' (max "<<pAggr->maxCount<<")."<<endl;
            return;
        }  
            
        // Reallocate memory, and save pointer in the same location
        unsigned char ** ppItems = (unsigned char **)(pItem + pAggr->offset);

        *ppItems = (unsigned char *)realloc(*ppItems, (cnt + 1) * pAggr->pDesc->getLen());

        // Initialize added item with default values. First memset to ensure
        // counters, which have no setdef fn, are 0 (also sets pointers to owned to NULL).
        memset(*ppItems + cnt * pAggr->pDesc->getLen(), 0, pAggr->pDesc->getLen());
        pAggr->pDesc->setdef(*ppItems + cnt * pAggr->pDesc->getLen());

        DBG_PRT("add at %p\n", *ppItems);

        return pAggr->setCount(pItem, cnt + 1);
    }
    cout << "No item '" << argv[0] << " in '" << getName() << "'." << endl;
}


// Del an owned component named by argc,argv from a composite
// xxx When all items deleted, set pointer to owned mem to NULL for later sanity checks?
void cm_composite_item_descriptor::del(int argc, char *argv[], unsigned char * pItem) const
{
    DBG_PRT("del %s\n\r", argv[0]);

    if ((argc != 1) && (argc != 2))
    {
        // Provide item name and, optionally, index
        cout << argc << " parameters for 'del'." << endl;
        return;
    }    

    // Find matching component name
    for (int i = 0; i < aggrCount; i++)
    {            
        const cm_aggregate * pAggr = aggrList[i];

        if (strcmp(argv[0], pAggr->pDesc->getName().c_str()) != 0)
        {
            continue; // mismatch: continue to next candidate
        }

        // Match found
        argc--;
        argv++;
        
        if (!pAggr->isAddSupported())
        {
            cout<<"Delete not supported for '"<<pAggr->pDesc->getName()<<"' in '"<<getName()<<"'."<< endl;
            return;
        }

        unsigned int cnt = pAggr->getCount(pItem); // number of items currently in array

        if (cnt == 0)
        {
            cout << "'Currently no '" <<pAggr->pDesc->getName()<<"' in '"<<getName()<<"'."<< endl;
            return;
        }

        unsigned int itemIdx;

        if (pAggr->getIndex(argc, argv, pItem, itemIdx) == cm_aggregate::CM_FAILED)
        {
            // An index is needed but couldn't be extracted from the command
            return;
        }

        // Reallocate memory, and save pointer in the same location
        unsigned char ** ppItems = (unsigned char **)(pItem + pAggr->offset);

        // Shift down items to occupy the memory vacated by deleted item
        memcpy(*ppItems + itemIdx * pAggr->pDesc->getLen(),
               *ppItems + (itemIdx + 1) * pAggr->pDesc->getLen(),
               (cnt - itemIdx - 1) * pAggr->pDesc->getLen());

        *ppItems = (unsigned char *)realloc(*ppItems, (cnt - 1) * pAggr->pDesc->getLen());

        DBG_PRT("del item base %p offset %d index %d len %d\n",
                pItem, pAggr->offset, itemIdx, pAggr->pDesc->getLen());

        return pAggr->setCount(pItem, cnt - 1);
    }
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
// xxx after each read, check how much was read.
// xxx shouldn't the i < aggrCount test come BEFORE allocating memory?
int cm_composite_item_descriptor::loadFromTlv(FILE * fp, unsigned char * pItem) const
{
    cm_item_len  tlvLen;
    cm_item_len  bytesRead;
    unsigned int itemIdx; // number of items read of a given type, i.e. offset in the item array
    bool         firstComp = true;
    
    fread(&tlvLen, sizeof(tlvLen), 1, fp);
    bytesRead = sizeof(tlvLen);

    DBG_PRT("composite load %d bytes to %p\n", tlvLen, pItem);

    while (bytesRead < tlvLen) // xxx 
    {    
        const cm_aggregate * pAggr;
        unsigned char *      pFirstItem;
        unsigned char **     ppItems;
        cm_descriptor_id     compId, prevCompId;
        int                  i;

        fread(&compId, sizeof(compId), 1, fp);
        bytesRead += sizeof(compId);

        DBG_PRT("load component ID %d\n", compId);

        // Look for a component with ID matching the one read from NVRAM
        for (i = 0; i < aggrCount; i++)
        {            
            pAggr = aggrList[i];
            
            if (compId == pAggr->pDesc->id)
            {
                break;
            }
        }

        // When we start with a new item type (or the 1st one), reset array index and pFirstItem,
        // and, if necessary, allocate memory for expected number of items
        if (firstComp || (compId != prevCompId))
        {
            // xxx here, check if itemIdx matches the count, i.e. did we read as many items as we should have?
            firstComp = false;

            // Allocate memory for owned items -- xxx note we assume the count has been populated correctly
            if (pAggr->isAddSupported() && (pAggr->getCount(pItem) > 0))
            {
                ppItems = (unsigned char **)(pItem + pAggr->offset);

                *ppItems = (unsigned char *)malloc(pAggr->pDesc->getLen() * pAggr->getCount(pItem));

                DBG_PRT("load: for %d items, alloc %p to %p\n", pAggr->getCount(pItem), *ppItems, ppItems);
            }

            pFirstItem = pAggr->getFirstItem(pItem);
            itemIdx = 0;
        }

        if (i < aggrCount)
        {            
            // Found a match, so delegate the reading to the corresponding component
            DBG_PRT("component '%s' matches, itemIdx %d\n", pAggr->pDesc->getName().c_str(), itemIdx);

            bytesRead += pAggr->pDesc->loadFromTlv(fp, pFirstItem + itemIdx++ * pAggr->pDesc->getLen());
        }
        else
        {            
            // We got to the end of the list without finding a match: skip over the unrecognized item
            cout << "Couldn't load unknown component ID " << compId << endl;

            cm_item_len componentTlvLen;
            fread(&componentTlvLen, sizeof(componentTlvLen), 1, fp);
            bytesRead += sizeof(componentTlvLen);
            fseek(fp, componentTlvLen, SEEK_CUR);
            bytesRead += componentTlvLen;
        }
        DBG_PRT("composite '%s': %d read\n", getName().c_str(), bytesRead);
        prevCompId = compId;
    }
    return bytesRead;
}


// Delegate print command to components
// 
void cm_composite_item_descriptor::print(const unsigned char * pItem, string prefix) const
{
    char indexbuf[6]; // xxx big enough to avoid truncation in all cases?

    DBG_PRT("print composite %s with len %d\n", name.c_str(), len);

    // For each component, and for each of the array of items under it...
    for (int i = 0; i < aggrCount; i++)
    {            
        const cm_aggregate *  pAggr      = aggrList[i];
        const unsigned char * pFirstItem = pAggr->getFirstItem(pItem);
        unsigned int          itemCount  = pAggr->getCount(pItem);

        for (unsigned j = 0; j < itemCount; j++)
        {
            if (pAggr->getCount(pItem) > 1)
            {
                // There's more than one item, so print the index to distinguish among them
                snprintf(indexbuf, sizeof(indexbuf), " %d", j);
            }
            else
            {
                // There's only one item, so we needn't print an index
                indexbuf[0] = 0;
            }
            pAggr->pDesc->print(pFirstItem + j * pAggr->pDesc->getLen(), prefix + pAggr->pDesc->getName() + indexbuf + " ");
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

            assert(*ppItems != NULL);
            
            DBG_PRT("setdef free %p\n", *ppItems);

            free(*ppItems);

            *ppItems = NULL; // xxx for future sanity checks

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
                                       unsigned char * pItem,
                                       cm_context & ctxt) const
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
            return print(pItem, "");

        case CM_SET:
            return set(pItem, argv[1]);

        case CM_SETDEF:
            return setdef(pItem);

        case CM_HELP:
            return help(pItem);

        default:
            cout << "Unknown operation '" << argv[0] << "'" << endl;
    }   
}


// An item does not print its own name, since
// it may be preceded by an index, which is known
// to the item's composite but not to the item.
void cm_simple_item_descriptor::print(const unsigned char * pItem, string prefix) const
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
void cm_simple_item_descriptor::set(unsigned char * pItem, string val) const
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
void cm_simple_item_descriptor::setdef(unsigned char * pItem) const
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
void cm_simple_item_descriptor::writeTlv(const unsigned char *pItem, unsigned char ** ppBuf) const
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
cm_item_len cm_simple_item_descriptor::getTlvLen(const unsigned char * pItem) const
{
    return sizeof(cm_descriptor_id) + sizeof(cm_item_len) + getLen();
}


// Calling function called this function because the ID matches,
// so it's not checked here again.
// This method reads L, and moves forward in the file by that many bytes,
// using what it finds in the file to initialize the object's configurable items.
// xxx after each read, check how much was read.
int cm_simple_item_descriptor::loadFromTlv(FILE * fp, unsigned char * pItem) const
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
    fread(pItem, tlvLen, 1, fp); // xxx ptr, size, count, stream

    return sizeof(tlvLen) + tlvLen;
}


////////////////////////////////////////////////////////////////////////////////
//
// cm_aggregate
//
////////////////////////////////////////////////////////////////////////////////

// Utility method to extract in index from an array of command words
// Returns CM_FAILED if unable to extract an index when one is required.
// Returns CM_GOT if able to return a valid (in-range) index, or 
// Return CM_NO_NEED no index is needed from user, in which case the index returned is 0.
//
cm_aggregate::CM_GET_INDEX_RESULT cm_aggregate::getIndex(int argc,
                                                         char ** argv,
                                                         unsigned char * pParentItem,
                                                         unsigned int & itemIdx) const
{   
    bool   gotIndex = false;
    char * pEnd;

    if (getCount(pParentItem) <= 1)
    {
        // No index from user is needed, since there are 0 or 1 items present
        itemIdx = 0;
        return CM_NO_NEED;
    }

    if (argc > 0)
    {
        // An index is needed, so try to extract one
        itemIdx = strtoul(argv[0], &pEnd, 0);

        if (pEnd > argv[0])
        {
            gotIndex = true;
        }
    }

    if (!gotIndex)
    {
        cout << "'" << pDesc->getName() << "' needs index." <<endl;
        return CM_FAILED;
    }

    if (itemIdx >= getCount(pParentItem))
    {
        cout<<"'"<<pDesc->getName()<<"' index "<<itemIdx<<" out of range (0.. "<<getCount(pParentItem)-1<<")."<<endl;
        return CM_FAILED;
    }
    return CM_GOT;
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
    unsigned int c;

    switch (pCounterAggr->pDesc->getLen()) 
    { 
        case 1:
            {
                unsigned char v;

                memcpy(&v, pParentItem + pCounterAggr->offset, sizeof(v));
                c = v;
            }
            break;
       
        case 2:
            { 
                unsigned short v;

                memcpy(&v, pParentItem + pCounterAggr->offset, sizeof(v));
                c = v;
            }
            break;
       
        case 4:
            memcpy(&c, pParentItem + pCounterAggr->offset, sizeof(c)); 
            break;

        default:
            assert(0);
       
    }
    return c;
}


// Set value in RAM that records the number of items in the array of items
// xxx enforce, run-time of compile-time, that counters are unsigned int sized.
void cm_owned_aggregate::setCount(unsigned char * pParentItem, unsigned int count) const
{
    // Sanity check: if add/del operation not supported, the setCount() is meaningless
    assert(isAddSupported());
    
    memcpy(pParentItem + pCounterAggr->offset, &count, sizeof(count));
}

// During iteration, return name of current item -- by default, just convert the iteration index
// to a string, but xxx
#if 0
string cm_aggregate::getCurrentItemName()
{
}
#endif


// Helper function that returns what kind of operation (if any) a word is
eCmOp getOp(const char * word)
{
    if (strcmp(word, "add") == 0)     return CM_ADD;
    if (strcmp(word, "del") == 0)     return CM_DEL;
    if (strcmp(word, "prt") == 0)     return CM_PRT;
    if (strcmp(word, "set") == 0)     return CM_SET;
    if (strcmp(word, "setdef") == 0)  return CM_SETDEF;
    if (strcmp(word, "load") == 0)    return CM_LOAD;
    if (strcmp(word, "save") == 0)    return CM_SAVE;
    if (strcmp(word, "<") == 0)       return CM_RESET_CTXT;
    if (strcmp(word, "?") == 0)       return CM_HELP;

    // If no match, it's not an operation
    return CM_OP_NONE;
}

