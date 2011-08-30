/// config manager def
#include "config_manager.h"
#include "config_manager_util.h"
#include "config_manager_dbg.h"

#include <stdlib.h> // malloc
#include <string.h> // memset, strcmp, memcpy


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
void config_manager::init(const cm_descriptor * desc)
{
    assert(desc != NULL);
    
    base_desc = desc;
    ramBase = (uint8_t *)malloc(base_desc->getLen());

    assert(ramBase != NULL);

    // This could be done in the constructor, but I do it here to make
    // unit tests independent (since the constructor can't be forced
    // to run at the beginning of each unit tests).
    resetCtxt();

    // Set counters to 0 and pointers to NULL on fresh memory before setDefault
    memset(ramBase, 0, base_desc->getLen());
    base_desc->setDefault(ramBase);
    load();
}


/// Execute command words entered by client on CLI cpp file
/// @param argc number of command words
/// @param argv command word array
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
            return resetCtxt();

        default: // Other commands are passed to current context
            break;
    }

    // This may become the new context: a modifiable copy of the current context
    cm_context tempCtxt = currCtxt;

    // Pass command that don't apply to CM as a whole, to current context for handling
    currCtxt.pDesc->handleCmd(argc, argv, currCtxt.pItem, tempCtxt);
}


// Set context back to base
void config_manager::resetCtxt()
{
    DBG_PRT("resetCtxt\n");

    currCtxt.pDesc = base_desc;
    currCtxt.str = "";
    currCtxt.pItem = ramBase;
}


// Modify the current context
void config_manager::updateCtxt(cm_context * pC)
{
    DBG_PRT("updateCtxt\n");
    assert(pC != NULL);
    currCtxt = *pC;
}


// Get a prompt string to display to user, representing the current context
const char * config_manager::getPromptString()
{
    return currCtxt.str.c_str();
}


// Save data in RAM to NVRAM, in TLV format.
void config_manager::save()
{
    tlv.resetWrite();
    base_desc->writeTlv(ramBase);
}


// Load data in NVRAM, in TLV format, to configurable items in RAM.
//
void config_manager::load()
{
    if (!tlv.resetRead())
    {
        cout << "No config file." << endl;
        return;
    }
    
    cm_item_id_t id; 

    tlv.getType(&id); // xxx check if successful??

    if (id != base_desc->getId())
    {
        // What if the file exists, but is empty and we can extract nothing from it?
        printf("Can't load id %#x\n", id);
        return;
    }

    printf("Load id %#x\n", id);

    // Before loading, thus allocating new memory, call setDefault to free owned memory
    base_desc->setDefault(ramBase);
    unsigned int complete;
    base_desc->loadFromTlv(ramBase, &complete);

#if 0 // xxx maybe check "complete" also?
    if (res != CM_SUCCESS)
    {
        cout << "Load failed: defaults restored." << endl;
        base_desc->setDefault(ramBase);
        return;
    }
#endif
    // Reset context, since a reload re-allocates memory and makes current context invalid
    resetCtxt();
}


////////////////////////////////////////////////////////////////////////////////
//
// cm_descriptor
//
////////////////////////////////////////////////////////////////////////////////


////////////////////////////////////////////////////////////////////////////////
//
// cm_composite_descriptor
//
////////////////////////////////////////////////////////////////////////////////

cm_composite_descriptor::cm_composite_descriptor(const cm_composite_metadata * pMeta):
pData(pMeta)
{
    assert(pData != NULL);
}


//
// argc number of items in argv
// argv array of strings containing name elements
// pItem - pointer to RAM at which item is located
//
// @return true if command was handled
//
// xxx should free memory allocated as side-effect of a non-set or
//     go-to-node command, not just an invalid command.
//
bool cm_composite_descriptor::handleCmd(int argc,
                                        char *argv[],
                                        uint8_t * pItem,
                                        cm_context & ctxt) const
{
    char * word = argv[0];
    DBG_PRT("composite::handleCmd: %s\n", argv[0]);

    assert(pItem != NULL);
    
    switch (getOp(word))
    {
        case CM_OP_NONE:
            return handleIdWord(argc, argv, pItem, ctxt);
        
        case CM_ADD:
            // Remove the word 'add' and pass the remainder to the method
            return handleAdd(argc - 1, &(argv[1]), pItem);
            
        case CM_DEL:
            // Remove the word 'del' and pass the remainder to the method
            return handleDel(argc - 1, &(argv[1]), pItem);
            
        case CM_PRT:
            print(pItem, "");
            return true;

        case CM_SETDEF:
            setDefault(pItem);
            return true;

        case CM_HELP:
            help(pItem);
            return true; // xxx true?

        default: 
            break;
    }

    // Don't use argv[0] here, because it may have been modified by getComponentItem
    cout << "Command '" << word << "' not handled in composite item '" << getName() << "'" << endl;
    return false;
}


// Handle word in string that's not a command, hence presumably it identifies a component
bool cm_composite_descriptor::handleIdWord(int argc, char *argv[], uint8_t * pItem, cm_context & ctxt) const
{
    const cm_aggregate * pAggr;           // Component of this composite identified by argc, argv
    uint8_t *            pComponentItem;  // pointer to component RAM
    bool                 added;           // Did getComponentItem create a new item?
    char *               word = argv[0];

    if (!getComponentItem(&argc, &argv, &pAggr, pItem, &pComponentItem, ctxt, added))
    {
        // Unhandled word(s): not a command, and also doesn't identify a component
        cout << "'" << word << "' not handled in composite item '" << getName() << "'" << endl;
        return false;
    }
    
    // A component was found
    if (argc == 0)
    {
        // We have a component, but there are no more words in the command
        config_manager::getInstance()->updateCtxt(&ctxt);
        return true;
    }
    
    // Pass the remainder of the command to the found component
    if (!pAggr->pData->pDesc->handleCmd(argc, argv, pComponentItem, ctxt))
    {
        // Component says the command is invalid
        if (added)
        {
            // Free memory allocated by an invalid command
            del(pItem, pAggr, pAggr->getCount(pItem) - 1, pAggr->getCount(pItem));                        
        }
        return false;
    }
    return true;
}


// Try to add a component named by argc,argv to a composite.
// After verifying the operation is applicable, the item is added.
bool cm_composite_descriptor::handleAdd(int argc, char *argv[], uint8_t * pItem) const
{
    DBG_PRT("handleAdd %s\n", argv[0]);

    assert(pItem != NULL);

    if (argc != 1)
    {
        cout << argc << " parameters for 'add'." << endl;
        return false;
    }

    const cm_aggregate * pAggr = getAggr(argv[0]);

    if (pAggr == NULL)
    {
        cout << "No item '" << argv[0] << "' in '" << getName() << "'." << endl;
        return false;
    }
    
    if (!pAggr->isAddSupported())
    {
        cout<<"Add not supported for '"<<pAggr->pData->pDesc->getName()<<"' in '"<<getName()<<"'."<< endl;
        return false;
    }

    if (pAggr->getCount(pItem) >= pAggr->pData->maxCount)
    {
        cout<<"Can't add '"<<pAggr->pData->pDesc->getName()<<"' (max "<<pAggr->pData->maxCount<<")."<<endl;
        return false;
    }  
        
    if (add(pItem, pAggr) == NULL)
    {
        return false;
    }
    return true;
}


// Add OWNED item.
// @pre Add operation is supported on pAggr, and counter is in range
// This allocates memory for the new item, sets it to default values,
// and increments the corresponding counter.
// @return pointer to new allocated memory, or NULL in case of failure
uint8_t * cm_composite_descriptor::add(uint8_t * pParentItem,
                                       const cm_aggregate * pAggr) const
{
    assert(pParentItem != NULL);
    assert(pAggr != NULL);

    // Reallocate memory, and save pointer in the same location
    unsigned   cnt     = pAggr->getCount(pParentItem);
    uint8_t ** ppItems = (uint8_t **)(pParentItem + pAggr->pData->offset);

    assert(pAggr->isAddSupported());

    uint8_t * pNewMem = (uint8_t *)realloc(*ppItems, (cnt + 1) * pAggr->pData->pDesc->getLen());

    if (pNewMem == NULL)
    {
        cout << "No " << pAggr->pData->pDesc->getLen() << " for " << pAggr->pData->pDesc->getName() << endl;
        return NULL;
    }

    // Memory successfully allocated, so reference the (possibly new) memory
    *ppItems = pNewMem;

    uint8_t * pNewItem = pNewMem + cnt * pAggr->pData->pDesc->getLen();

    // Initialize added item with default values. First memset to ensure
    // counters, which have no setDefault fn, are 0 (also sets pointers to owned to NULL).
    memset(pNewItem, 0, pAggr->pData->pDesc->getLen());
    pAggr->pData->pDesc->setDefault(pNewItem);

    DBG_PRT("add at %p\n", pNewMem);

    pAggr->setCount(pParentItem, cnt + 1);
    return pNewItem;
}


// Del an owned component named by argc,argv from a composite
bool cm_composite_descriptor::handleDel(int argc, char *argv[], uint8_t * pItem) const
{
    assert(pItem != NULL);

    DBG_PRT("handleDel %s\n", argv[0]);

    if ((argc != 1) && (argc != 2))
    {
        // Provide item name and, optionally, index
        cout << argc << " parameters for 'del'." << endl;
        return false;
    }    

    const cm_aggregate * pAggr = getAggr(argv[0]);

    if (pAggr == NULL)
    {        
        cout << "No item '" << argv[0] << "' in '" << getName() << "'." << endl;
        return false;
    }

    // Match found
    argc--;
    argv++;
    
    if (!pAggr->isAddSupported())
    {
        cout<<"Delete not supported for '"<<pAggr->pData->pDesc->getName()<<"' in '"<<getName()<<"'."<< endl;
        return false;
    }

    unsigned int cnt = pAggr->getCount(pItem); // number of items currently in array

    if (cnt == 0)
    {
        cout << "Currently no '" <<pAggr->pData->pDesc->getName()<<"' in '"<<getName()<<"'."<< endl;
        return false;
    }

    unsigned int itemIdx = 0; // If no explicit index is needed, use 0 offset

    if (pAggr->needIndex(pItem) && !pAggr->getIndex(&argc, &argv, pItem, itemIdx))
    {
        // An index is needed but couldn't be extracted from the command
        return false;
    }

    if (itemIdx >= cnt)
    {
        cout<<"Index "<<itemIdx<<" out of range (0.. "<<pAggr->getCount(pItem)-1<<")."<<endl;
        return false;
    }

    del(pItem, pAggr, itemIdx, cnt);
    return true;
}


// Del OWNED item.
// This re-allocates the necessary memory, updates the counter if necessary,
// and sets the pointer to the memory to NULL if it's all been freed.
void cm_composite_descriptor::del(uint8_t * pParentItem,
                                  const cm_aggregate * pAggr,
                                  unsigned int itemIdx,
                                  unsigned int cnt) const
{
    // Sanity check input parameters
    assert(pParentItem != NULL);
    assert(pAggr != NULL);
    assert(cnt > 0);
    assert(itemIdx < cnt);
    assert(pAggr->isAddSupported());

    cm_item_len_t componentLen = pAggr->pData->pDesc->getLen();
    // Reallocate memory, and save pointer in the same location
    uint8_t ** ppItems = (uint8_t **)(pParentItem + pAggr->pData->offset);

    DBG_PRT("del at %p index %d len %d\n", *ppItems, itemIdx, componentLen);

    assert(*ppItems != NULL);

    // Shift down items to occupy the memory vacated by deleted item
    memmove(*ppItems + itemIdx * componentLen,
            *ppItems + (itemIdx + 1) * componentLen,
            (cnt - itemIdx - 1) * componentLen);

    *ppItems = (uint8_t *)realloc(*ppItems, (cnt - 1) * componentLen);

    // xxx realloc should return NULL if memory to be allocated is 0, but it doesn't seem to...
    if (cnt == 1)
    {
        *ppItems = NULL;
    }
    pAggr->setCount(pParentItem, cnt - 1);
}


// Delegate print command to components
// 
void cm_composite_descriptor::print(const uint8_t * pItem, string prefix) const
{
    assert(pItem != NULL);

    DBG_PRT("print composite %s len %d\n", getName().c_str(), getLen());

    for (unsigned i = 0; i < pData->aggrCount; i++)
    {            
        getAggrAtIndex(i)->print(pItem, prefix);
    }
}


// Delegate setDefault command to components
//
// @pre: item contains valid data, i.e. if an OWNED
// component has no items allocated, the pointer to the items 
// is NULL, so we can know not to try to free them.
//
// For OWNED components, we free owned memory before setting
// the corresponding counter to 0.
//
void cm_composite_descriptor::setDefault(uint8_t * pItem) const
{    
    assert(pItem != NULL);

    // Set each component to default
    for (unsigned i = 0; i < pData->aggrCount; i++)
    {            
        getAggrAtIndex(i)->setDefault(pItem);
    }
}


// Give name of each component, current count, and maxcount if OWNed.
void cm_composite_descriptor::help(const uint8_t * pItem) const
{
    assert(pItem != NULL);

    for (unsigned i = 0; i < pData->aggrCount; i++)
    {   
        cout << getAggrAtIndex(i)->pData->pDesc->getName() << " [" << getAggrAtIndex(i)->getCount(pItem);

        if (getAggrAtIndex(i)->isAddSupported())
        {
            cout << "/" << getAggrAtIndex(i)->pData->maxCount;
        }
        cout << "]" << endl;
    }
}


// Look for the aggregate whose component has a matching name
const cm_aggregate * cm_composite_descriptor::getAggr(const char * name) const
{
    assert(name != NULL);

    for (unsigned i = 0; i < pData->aggrCount; i++)
    {            
        if (strcmp(name, getAggrAtIndex(i)->pData->pDesc->getName().c_str()) == 0)
        {
            return getAggrAtIndex(i);
        }
    }
    return NULL;
}


// Look for the aggregate whose component has a matching ID
const cm_aggregate * cm_composite_descriptor::getAggr(cm_item_id_t id) const
{
    for (unsigned i = 0; i < pData->aggrCount; i++)
    {            
        if (getAggrAtIndex(i)->pData->pDesc->getId() == id)
        {
            return getAggrAtIndex(i);
        }
    }
    return NULL;
}


/// Write item's TLV to a buffer, and advance the ptr to the end of memory written to.
//  This is useful for writing to a RAM buffer first, for subsequent write
//  to NVRAM.
//  xxx if we want to write directly to NVRAM, we need to implement a method
//  that does that...
void cm_composite_descriptor::writeTlv(const uint8_t *pItem) const
{
    assert(pItem != NULL);

    if (!pData->c.persistent)
    {
        return;
    }
    
    config_manager::getInstance()->tlv.startWriteComposite(pData->c.id);

    for (unsigned i = 0; i < getAggrCount(); i++)
    {            
        const cm_aggregate * pAggr = getAggrAtIndex(i);

        for (unsigned j = 0; j < pAggr->getCount(pItem); j++)
        {
            pAggr->pData->pDesc->writeTlv(pAggr->getItemAtIndex(pItem, j));
        }
    }

    config_manager::getInstance()->tlv.endWriteComposite();
}


// From remaining command-line words, find matching component of this composite.
// If the component does not exist, it is created in certain cases.
// pArgc: (in/out) number of command words
// pArgv: (in/out) command word pointer
// ppAggr: out, the wanted aggregate, or 0 if command identifies none
// pParentItem: (in) the owning item
// ppItem: (out) the wanted item
// ctxt:   (in/out)
// added: (out) did this function allocate memory for the item?
//
// @return true if item is returned
//
bool cm_composite_descriptor::getComponentItem(int * pArgc,
                                               char *** pArgv,
                                               const cm_aggregate ** ppAggr,
                                               uint8_t * pParentItem,
                                               uint8_t ** ppItem,
                                               cm_context & ctxt,
                                               bool & added) const
{
    assert(pArgc != NULL);
    assert(pArgv != NULL);
    assert(ppAggr != NULL);
    assert(pParentItem != NULL);
    assert(ppItem != NULL);
    
    added = false; // By default, didn't add a new component
    
    *ppAggr = getAggr(*pArgv[0]);

    if (*ppAggr == NULL)
    {
        return false;
    }

    ctxt.str += (*ppAggr)->pData->pDesc->getName() + " ";
    
    // Found matching name: now try to get index from next word
    *pArgc -= 1;
    *pArgv += 1;
    unsigned int itemIdx = 0; // If no index needed, use offset 0

    if ((*ppAggr)->pData->maxCount > 1)
    {
        // Explicit index is needed if there can be more than one instance
        if ((*ppAggr)->getIndex(pArgc, pArgv, pParentItem, itemIdx))
        {
            // Index available, it becomes part of the context string
            char indexbuf[6]; // xxx big enough to avoid truncation in all cases?

            snprintf(indexbuf, sizeof(indexbuf), "%d ", itemIdx);
            ctxt.str = ctxt.str + indexbuf;
        }
        else
        {
            // The necessary index was not in the command
            return false;
        }
    }

    unsigned int cnt = (*ppAggr)->getCount(pParentItem); // Number of items currently in the aggregate

    DBG_PRT("getComponentItem %p offset %d idx %d cnt %d len %d\n",
            *ppItem, (*ppAggr)->pData->offset, itemIdx, cnt, (*ppAggr)->pData->pDesc->getLen());

    if (itemIdx >= cnt)
    {
        // Index refers to an item that doesn't exist
        if ((*ppAggr)->isAddSupported() && (itemIdx == cnt) && (itemIdx < (*ppAggr)->pData->maxCount))
        {
            // Index refers to an item to create
            add(pParentItem, *ppAggr);
            added = true;
        }
        else
        {
            cout<<"Index "<<itemIdx<<" out of range"<<endl;
            return false;
        }
    }

    *ppItem = (*ppAggr)->getItemAtIndex(pParentItem, itemIdx);
    ctxt.pDesc = (*ppAggr)->pData->pDesc;
    ctxt.pItem = *ppItem;
    return true;
}


//
// From ID and index, return aggregate and pointer to component item in this composite.
// Because this function is called during loading, items are created as
// needed: if the item is contained, the pointer to the already-allocated
// memory is returned, and if it's owned, memory for the item is allocated.
// id: (in) component item's ID
// idx: (in) index of wanted component, 1 larger than
//           the index previously passed to this method for the
//           same id in the same composite
// ppAggr: out, the wanted aggregate, or 0 if command identifies none
// pParentItem: (in) the owning item
// ppItem: (out) the wanted item
//
// @return true if item is returned
//
bool cm_composite_descriptor::getComponentItem(cm_item_id_t id,
                                               unsigned idx,
                                               const cm_aggregate ** ppAggr,
                                               uint8_t * pParentItem,
                                               uint8_t ** ppItem) const
{
    assert(ppAggr != NULL);
    assert(pParentItem != NULL);
    assert(ppItem != NULL);
    
    *ppAggr = getAggr(id);

    if (*ppAggr == NULL)
    {
        return false;
    }

    if (idx == (*ppAggr)->pData->maxCount)
    {
        // Maximum number of these item already loaded: fail
        return false;
    }

    if ((*ppAggr)->isAddSupported())
    {
        *ppItem = add(pParentItem, *ppAggr); 
    }
    else
    {
        *ppItem = (*ppAggr)->getItemAtIndex(pParentItem, idx);
    }

    if (*ppItem == NULL)
    {
        return false;
    }
    return true;
}


// T already read
unsigned int cm_composite_descriptor::loadFromTlv(uint8_t * pItem, unsigned * pComplete) const
{
    unsigned idx;
    bool     first = true; // Is this the first component item

    config_manager::getInstance()->tlv.loadComposite();

    do
    {
        cm_item_id_t componentId, lastComponentId;
        uint8_t * pComponentItem;
        const cm_aggregate * pAggr;

        config_manager::getInstance()->tlv.getType(&componentId);

        if (first || (lastComponentId != componentId))
        {
            // First read of this component ID, i.e. first item in an array
            idx = 0;
            first = false;
        }

        if (getComponentItem(componentId, idx, &pAggr, pItem, &pComponentItem))
        {
            if (pAggr->pData->pDesc->loadFromTlv(pComponentItem, pComplete) == CM_SUCCESS)
            {
                idx++;
            }
        }
        else
        {            
            // Unable to find the ID, or idx too big, or no memory: skip the item.
            config_manager::getInstance()->tlv.skipItem(pComplete);
        }
        lastComponentId = componentId;

    } while (*pComplete == 0); // while this composite is incomplete

    (*pComplete)--;
    
    return 0; // xxx
}


////////////////////////////////////////////////////////////////////////////////
//
// cm_simple_descriptor
//
////////////////////////////////////////////////////////////////////////////////

cm_simple_descriptor::cm_simple_descriptor(const cm_simple_metadata * pMeta):
pData(pMeta)
{
    assert(pData != NULL);
}


// An item does not print its own name, since
// it may be preceded by an index, which is known
// to the item's composite but not to the item.
void cm_simple_descriptor::print(const uint8_t * pItem, string prefix) const
{
    assert(pItem != NULL);
    
    cout << prefix << "= ";

    DBG_PRT("print simple %s len %d at %p\n", getName().c_str(), getLen(), pItem);
    
    if (pData->pPrt == NULL)
    {
        // No function installed so default print function: hex chars
        cm_prt_hexstr(pItem, getLen());
    }
    else
    {
        pData->pPrt(pItem, getLen());
    }
    cout << endl;
}


//
// argc number of items in argv
// argv array of strings containing name elements
// pItem - pointer to RAM at which item is located
//
bool cm_simple_descriptor::handleCmd(int argc,
                                    char *argv[],
                                    uint8_t * pItem,
                                    cm_context & ctxt) const
{
    assert(pItem != NULL);
    
    DBG_PRT("simple cmd at %p\n", pItem);
    
    switch (getOp(argv[0]))
    {
        case CM_PRT:
            print(pItem, "");
            return true;

        case CM_SET:
            if (argc == 2)
            {
                return set(pItem, argv[1]);
            }
            break;

        case CM_SETDEF:
            setDefault(pItem);
            return true;

        case CM_HELP:
            help(pItem);
            return true; // true?

        default:
            cout << "'" << argv[0] << "' not handled by simple item '" << getName() << "'" << endl;
    }
    return false;
}


// Set item to a value input as string on command line
bool cm_simple_descriptor::set(uint8_t * pItem, string val) const
{
    assert(pItem != NULL);

    DBG_PRT("set simple %s at %p to '%s'\n", getName().c_str(), pItem, val.c_str());

    if (pData->pSet != NULL)
    {
        return pData->pSet(pItem, getLen(), val);
    }
    else
    {
        cout << "'" << getName() << "' can't be set." << endl;
        return false;
    }
}


// Set configurable item to its default value.
void cm_simple_descriptor::setDefault(uint8_t * pItem) const
{
    assert(pItem != NULL);

    if (pData->pSetDefault != NULL)
    {
        pData->pSetDefault(pItem, getLen());
    }
}


/// Write item's TLV to a buffer, and advance the ptr to the end of memory written to.
//  This is useful for writing to a RAM buffer first, for subsequent write
//  to NVRAM.
//  xxx if we want to write directly to NVRAM, we need to implement a method
//  that does that...
void cm_simple_descriptor::writeTlv(const uint8_t *pItem) const
{
    assert(pItem != NULL);
    
    if (!pData->c.persistent)
    {
        return;
    }
    
    config_manager::getInstance()->tlv.writeSimple(pData->c.id, pData->c.len, pItem);
}


unsigned int cm_simple_descriptor::loadFromTlv(uint8_t * pItem, unsigned * pComplete) const
{
    cm_item_len_t len = pData->c.len;
    t_cm_result res = config_manager::getInstance()->tlv.loadSimple(pItem, &len, pComplete);
    return 0; // xxx
}


////////////////////////////////////////////////////////////////////////////////
//
// cm_aggregate
//
////////////////////////////////////////////////////////////////////////////////

// Utility method to extract in index from an array of command words
// @return false if unable to extract a valid (in-range) index,
//         true if returning a valid (in-range) index.
//
bool cm_aggregate::getIndex(int * pArgc,
                            char *** pArgv,
                            const uint8_t * pParentItem,
                            unsigned int & itemIdx) const
{
    assert(pParentItem != NULL);
    
    if (*pArgc > 0)
    {
        char * pEnd; // pointer to char after chars accepted by strtoul

        // An index is needed, so try to extract one
        itemIdx = strtoul((*pArgv)[0], &pEnd, 0);

        if (pEnd > (*pArgv)[0])
        {
            // Success: strtoul read an unsigned integer from the input
            *pArgc -= 1;
            *pArgv += 1;
            return true;
        }
    }

    cout << "'" << pData->pDesc->getName() << "' needs index." <<endl;
    return false;
}


// Return pointer to item, given parent item and index
uint8_t * cm_aggregate::getItemAtIndex(const uint8_t * pParentItem, unsigned idx) const
{
    return getFirstItem(pParentItem) + idx * pData->pDesc->getLen();
}


// Set items to default (and free the memory they occupied, if OWNed)
// @param pItem - item this aggregate belongs to
void cm_aggregate::setDefault(uint8_t * pItem) const
{
    // Set each item to default (thus freeing memory of OWNed sub-components)
    for (unsigned i = 0; i < getCount(pItem); i++)
    {           
        pData->pDesc->setDefault(getItemAtIndex(pItem, i));
    }

    // Free item memory (for OWNed aggregates, no effect on CONTAINed)
    freeItems(pItem);
}


// Print, appending index to prefix (if necessary) and delegating to items
// @param prefix string to be pre-pended to the value, representing its context
void cm_aggregate::print(const uint8_t * pItem, std::string prefix) const
{
    char indexbuf[6]; // xxx big enough to avoid truncation in all cases?

    for (unsigned i = 0; i < getCount(pItem); i++)
    {
        if (pData->maxCount > 1)
        {
            // There's more than one item, so print the index to distinguish among them
            snprintf(indexbuf, sizeof(indexbuf), " %d", i);
        }
        else
        {
            // There's only one item, so we needn't print an index
            indexbuf[0] = 0;
        }
        pData->pDesc->print(getItemAtIndex(pItem, i),
                            prefix + pData->pDesc->getName() + indexbuf + " ");
    }
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
uint8_t * cm_contained_aggregate::getFirstItem(const uint8_t * pParentItem) const
{
    assert(pParentItem != NULL);

    return (uint8_t *)(pParentItem + pData->offset);
}


// Return the number of items in the component's array.
// For a contained component, the count is fixed at maxCount.
unsigned cm_contained_aggregate::getCount(const uint8_t * pParentItem) const
{
    return pData->maxCount;
}


////////////////////////////////////////////////////////////////////////////////
//
// cm_owned_aggregate
//
////////////////////////////////////////////////////////////////////////////////

// Return address of the first item in the item array, or NULL if there are
// no items allocated.
// pParentItem: pointer to parent item; from this the aggregate obtains the
//              address of the first item in the array that it links to the parent.
//
uint8_t * cm_owned_aggregate::getFirstItem(const uint8_t * pParentItem) const
{
    assert(pParentItem != NULL);

    return *(uint8_t **)(pParentItem + pData->offset); // location is a pointer to the OWNED item
}


// Return the number of items in the component's array
// xxx giving a fixed size to counters would simplify this, but
// introduces a dependency on the application programmer doing the right thing
unsigned cm_owned_aggregate::getCount(const uint8_t * pParentItem) const
{
    assert(pParentItem != NULL);

    if (pCounterAggr == NULL)
    {
        // If there's no counter, then count is just 0 (absence) or 1 (presence)
        return (getFirstItem(pParentItem) == NULL) ? 0 : 1;
    }
    
    switch (pCounterAggr->pData->pDesc->getLen()) 
    { 
        case sizeof(uint8_t):
            return (unsigned)(*(pParentItem + pCounterAggr->pData->offset));
       
        case sizeof(uint16_t):
            return (unsigned)(*((uint16_t *)(pParentItem + pCounterAggr->pData->offset)));
       
        case sizeof(uint32_t):
            return (unsigned)(*((uint32_t *)(pParentItem + pCounterAggr->pData->offset)));

        default:
            assert(0);
    }
}


// Set value in RAM that records the number of items in the array of items
// xxx enforce, run-time or compile-time, that counters are unsigned int sized.
void cm_owned_aggregate::setCount(uint8_t * pParentItem, unsigned int count) const
{
    assert(pParentItem != NULL);

    // Sanity check: if add/del operation not supported, the setCount() is meaningless
    assert(isAddSupported());

    if (pCounterAggr == NULL)
    {
        // There is no counter -- it's optional if maxCount == 1
        // xxx what happens if there's no counter but maxCount > 1?
        return;
    }   
    memcpy(pParentItem + pCounterAggr->pData->offset, &count, sizeof(count));
}


// Free memory of items, if any.  This is called when setting the parent
// item to default -- the default for OWNed components is that there are none.
//
void cm_owned_aggregate::freeItems(uint8_t * pParentItem) const
{
    uint8_t ** ppItems = (uint8_t **)(pParentItem + pData->offset);

    if (*ppItems != NULL)
    {
        // Sanity check
        assert(getCount(pParentItem) > 0);

        // There are items, so free their block of memory, and set counter to 0
        DBG_PRT("freeItems %p\n", *ppItems);

        free(*ppItems);
        *ppItems = NULL;
        setCount(pParentItem, 0);
    }
    else
    {
        // Sanity check: if the pointer is NULL, there are no items.
        assert(getCount(pParentItem) == 0);
    }
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

