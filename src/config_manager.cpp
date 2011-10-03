/// config manager def
#include "config_manager.h"
#include "config_manager_util.h"
#include "config_manager_dbg.h"
#include "config_manager_store.h"

#include <stdlib.h> // malloc
#include <string.h> // memset, strcmp, memcpy


#include <sstream>
using namespace std;

config_manager * config_manager::instance = NULL;


////////////////////////////////////////////////////////////////////////////////
//
// config_manager
//
////////////////////////////////////////////////////////////////////////////////

config_manager::config_manager(): base_desc(NULL), ramBase(NULL)
{
    store = cm_store::getStore();
}


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

    DBG_PRT("init: ramBase, %d at %p\n", base_desc->getLen(), ramBase);

    assert(ramBase != NULL);

    // This could be done in the constructor, but I do it here to make unit tests
    // independent (since the constructor won't run at the beginning of each test).
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
    command_stack cmd(argc, argv);
    
    // First treat the commands that are only applicable at the top level
    switch (cmd.getTopOp())
    {
        case command_stack::CM_LOAD:
            return load();

        case command_stack::CM_SAVE:
            return save();

        case command_stack::CM_RESET_CTXT:
            return resetCtxt();

        default: // Other commands are passed to current context
            break;
    }

    // This may become the new context: a modifiable copy of the current context
    cm_context tempCtxt(currCtxt);

    // Pass command that doesn't apply to CM as a whole, to current context for handling
    currCtxt.getDesc()->handleCmd(&cmd, currCtxt.getItem(), tempCtxt);
}


// Set context back to base
void config_manager::resetCtxt()
{
    DBG_PRT("resetCtxt\n");
    cm_context temp("", base_desc, ramBase); // temp context with base properties
    currCtxt = temp;
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
    return currCtxt.getString().c_str();
}


// Save data in RAM to persistent storage
void config_manager::save()
{
    store->resetWrite();
    base_desc->save(ramBase);
}


// Load data in persistent storage, to configurable items in RAM.
//
void config_manager::load()
{
    if (!base_desc->isPersistent())
    {
        cout << "No persistent items." << endl;
        return;
    }

    if (!store->resetRead())
    {
        cout << "No config file." << endl;
        return;
    }
    
    cm_item_id_t id;
    store->getType(&id); // xxx What if this fails?

    if (id != base_desc->getId())
    {
        // What if the file exists, but is empty and we can extract nothing from it?
        printf("Can't load id %#x\n", id);
        return;
    }

    printf("Load id %#x\n", id);

    // Before loading, thus allocating new memory, call setDefault to free owned memory
    base_desc->setDefault(ramBase);
    unsigned int complete = 0;
    t_cm_result res = base_desc->load(ramBase, &complete);

    if (res != CM_SUCCESS)
    {
        cout << "Load failed: defaults restored." << endl;
        base_desc->setDefault(ramBase);
        return;
    }

    assert(complete == 0); // We've exited the top-level composite, so nothing can be left

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
// @param cmd - stack of strings containing name elements
// @parampItem - pointer to RAM at which item is located
//
// @return true if command was handled
//
// xxx should free memory allocated as side-effect of a non-set or
//     go-to-node command, not just an invalid command.
//
bool cm_composite_descriptor::handleCmd(command_stack * cmd,
                                        uint8_t * pItem,
                                        cm_context & ctxt) const
{
    DBG_PRT("composite::handleCmd: %s\n", cmd->getTop());

    assert(pItem != NULL);
    
    switch (cmd->getTopOp())
    {
        case command_stack::CM_OP_NONE:
            return handleIdWord(cmd, pItem, ctxt);
        
        case command_stack::CM_ADD:
            // Remove the word 'add' and pass the remainder to the method
            return handleAdd(&cmd->pop(), pItem);
            
        case command_stack::CM_DEL:
            // Remove the word 'del' and pass the remainder to the method
            return handleDel(&cmd->pop(), pItem);
            
        case command_stack::CM_PRT:
            print(pItem, "");
            return true;

        case command_stack::CM_SETDEF:
            setDefault(pItem);
            return true;

        case command_stack::CM_HELP:
            help(pItem);
            return true; // xxx true?

        default: 
            break;
    }

    // Don't use argv[0] here, because it may have been modified by getComponentItem
    cout << "Command '" << cmd->getTop() << "' not handled in composite item '" << getName() << "'" << endl;
    return false;
}


// Handle word in string that's not a command, hence presumably it identifies a component
bool cm_composite_descriptor::handleIdWord(command_stack * cmd, uint8_t * pItem, cm_context & ctxt) const
{
    const cm_aggregate * pAggr = getAggr(cmd->getTop()); // Component of this composite identified by cmd

    if (pAggr == NULL)
    {
        // Unhandled word(s): not a command, and also doesn't identify a component
        cout << "'" << cmd->getTop() << "' not in composite item '" << getName() << "'" << endl;
        return false;
    }

    ctxt.add(pAggr->pData->pDesc->getName());

    bool      added;           // Did getComponentItem create a new item?
    uint8_t * pComponentItem;  // pointer to component RAM

    if (!pAggr->getComponentItem(&cmd->pop(), pItem, &pComponentItem, ctxt, added))
    {
        // Index problems are reported by the called fn
        return false;
    }

    // A component was found
    if (cmd->getCount() == 0)
    {
        // We have a component, but there are no more words in the command
        config_manager::getInstance()->updateCtxt(&ctxt);
        return true;
    }
    
    // Pass the remainder of the command to the found component
    if (!pAggr->pData->pDesc->handleCmd(cmd, pComponentItem, ctxt))
    {
        // Component says the command is invalid
        if (added)
        {
            // Free memory allocated by an invalid command
            pAggr->del(pItem, pAggr->getCount(pItem) - 1);                        
        }
        return false;
    }
    return true;
}


// Try to add a component named by cmd to a composite.
// After verifying the operation is applicable, the item is added.
// @return true if the operation was successful, false if it failed.
bool cm_composite_descriptor::handleAdd(command_stack * cmd, uint8_t * pItem) const
{
    DBG_PRT("handleAdd %s\n", cmd->getTop());

    assert(pItem != NULL);

    if (cmd->getCount() != 1)
    {
        cout << cmd->getCount() << " parameters for 'add'." << endl;
        return false;
    }

    const cm_aggregate * pAggr = getAggr(cmd->getTop());

    if (pAggr == NULL)
    {
        cout << "No item '" << cmd->getTop() << "' in '" << getName() << "'." << endl;
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
        
    if (pAggr->add(pItem) == NULL)
    {
        return false;
    }
    return true;
}


// Del an owned component named by cmd from a composite
// @return true if the operation was successful, false if it failed.
bool cm_composite_descriptor::handleDel(command_stack * cmd, uint8_t * pItem) const
{
    assert(pItem != NULL);

    DBG_PRT("handleDel %s\n", cmd->getTop());

    if ((cmd->getCount() != 1) && (cmd->getCount() != 2))
    {
        // Provide item name and, optionally, index
        cout << cmd->getCount() << " parameters for 'del'." << endl;
        return false;
    }    

    const cm_aggregate * pAggr = getAggr(cmd->getTop());

    if (pAggr == NULL)
    {        
        cout << "No item '" << cmd->getTop() << "' in '" << getName() << "'." << endl;
        return false;
    }

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

    if (pAggr->needIndex(pItem) && !pAggr->getIndex(&cmd->pop(), pItem, itemIdx))
    {
        // An index is needed but couldn't be extracted from the command
        return false;
    }

    if (itemIdx >= cnt)
    {
        cout<<"Index "<<itemIdx<<" out of range (0.. "<<pAggr->getCount(pItem)-1<<")."<<endl;
        return false;
    }

    pAggr->del(pItem, itemIdx);
    return true;
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


/// Save item to persistent storage, if its metadata says it's a persistent item
//
void cm_composite_descriptor::save(const uint8_t *pItem) const
{
    assert(pItem != NULL);

    if (!pData->c.persistent)
    {
        return;
    }
    
    config_manager::getInstance()->store->startWriteComposite(pData);

    for (unsigned i = 0; i < getAggrCount(); i++)
    {   
        getAggrAtIndex(i)->save(pItem);
    }
    config_manager::getInstance()->store->endWriteComposite();
}


// Load item from persistent storage
//
// @pre the item's type is already read
//
// @param pComplete (output) - the number of outstanding composite completions, i.e.
//         the number of times we should return from invocations of this method.
//
// @note A failure loading any component terminates the entire load
// 
t_cm_result cm_composite_descriptor::load(uint8_t * pItem, unsigned * pComplete) const
{
    bool first = true; // Is this the first component item

    config_manager::getInstance()->store->loadComposite();

    do
    {
        unsigned             idx;
        cm_item_id_t         componentId, prevComponentId;
        t_cm_result          res;

        if ((res = config_manager::getInstance()->store->getType(&componentId)) != CM_SUCCESS)
        {
            return res;
        }

        if (first || (prevComponentId != componentId))
        {
            // First read of this component ID, i.e. first item in an array
            idx = 0;
            first = false;
        }

        const cm_aggregate * pAggr = getAggr(componentId);
        uint8_t *            pComponentItem;

        if ((pAggr != NULL) &&
            pAggr->pData->pDesc->isPersistent() &&
            pAggr->getComponentItem(idx, pItem, &pComponentItem))
        {
            if ((res = pAggr->pData->pDesc->load(pComponentItem, pComplete)) != CM_SUCCESS)
            {
                return res;
            }
            idx++;
        }
        else
        {      
            // Can't find ID, or item not persistent, or idx too big, or no memory...
            config_manager::getInstance()->store->skipItem(pComplete);
        }
        prevComponentId = componentId;
    } while (*pComplete == 0); // while this composite is incomplete

    // This composite is complete, so decrement the number of outstanding completions.
    (*pComplete)--; 
    return CM_SUCCESS;
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
        cm_prt_hexstr(stdout, pItem, getLen());
    }
    else
    {
        pData->pPrt(stdout, pItem, getLen());
    }
    cout << endl;
}


//
// cmd - array of strings containing name elements
// pItem - pointer to RAM at which item is located
//
bool cm_simple_descriptor::handleCmd(command_stack * cmd,
                                    uint8_t * pItem,
                                    cm_context & ctxt) const
{
    assert(pItem != NULL);
    
    DBG_PRT("simple cmd at %p\n", pItem);
    
    switch (cmd->getTopOp())
    {
        case command_stack::CM_PRT:
            print(pItem, "");
            return true;

        case command_stack::CM_SET:
            if (cmd->getCount() == 2)
            {
                return set(pItem, cmd->pop().getTop());
            }
            break;

        case command_stack::CM_SETDEF:
            setDefault(pItem);
            return true;

        case command_stack::CM_HELP:
            help(pItem);
            return true; // true?

        default:
            cout << "'" << cmd->getTop() << "' not handled by simple item '" << getName() << "'" << endl;
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


/// Save item to persistent storage
void cm_simple_descriptor::save(const uint8_t *pItem) const
{
    assert(pItem != NULL);
    
    if (!pData->c.persistent)
    {
        return;
    }
    config_manager::getInstance()->store->writeSimple(pData, pItem);
}


// @param pComplete (output) - the number of outstanding composite completions, i.e.
//        the number of times we should return from cm_composite_descriptor::load
t_cm_result cm_simple_descriptor::load(uint8_t * pItem, unsigned * pComplete) const
{   
    return config_manager::getInstance()->store->loadSimple(pItem, pData, pComplete);
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
bool cm_aggregate::getIndex(command_stack * cmd,
                            const uint8_t * pParentItem,
                            unsigned int & itemIdx) const
{
    assert(pParentItem != NULL);
    
    if (cmd->getCount() > 0)
    {
        char * pEnd; // pointer to char after chars accepted by strtoul

        // An index is needed, so try to extract one
        itemIdx = strtoul(cmd->getTop(), &pEnd, 0);

        if (pEnd > cmd->getTop())
        {
            // Success: strtoul read an unsigned integer from the input
            cmd->pop();
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


// From remaining command-line words, find component item of this aggregate.
// If the item does not exist, it is created in certain cases.
// cmd - command string stack
// pParentItem: (in) the owning item
// ppItem: (out) the wanted item
// ctxt:   (in/out)
// added: (out) did this function allocate memory for the item?
//
// @return true if item is returned, false if no index, or index out of range
//
bool cm_aggregate::getComponentItem(command_stack * cmd,
                                    uint8_t * pParentItem,
                                    uint8_t ** ppItem,
                                    cm_context & ctxt,
                                    bool & added) const
{
    assert(cmd != NULL);
    assert(pParentItem != NULL);
    assert(ppItem != NULL);
    
    added = false; // By default, didn't add a new component
        
    // Found matching name: now try to get index from next word
    unsigned int itemIdx = 0; // If no index needed, use offset 0

    if (pData->maxCount > 1)
    {
        // Explicit index is needed if there can be more than one instance
        if (getIndex(cmd, pParentItem, itemIdx))
        {
            // Index is available: add it to the context string
            ctxt.add(itemIdx);
        }
        else
        {
            // The necessary index was not in the command
            return false;
        }
    }

    unsigned int cnt = getCount(pParentItem); // Number of items currently in the aggregate

    DBG_PRT("getComponentItem %p offset %d idx %d cnt %d len %d\n",
            *ppItem, pData->offset, itemIdx, cnt, pData->pDesc->getLen());

    if (itemIdx >= cnt)
    {
        // Index refers to an item that doesn't exist
        if (isAddSupported() && (itemIdx == cnt) && (itemIdx < pData->maxCount))
        {
            // Index refers to an item to create
            add(pParentItem);
            added = true;
        }
        else
        {
            cout<<"Index "<<itemIdx<<" out of range"<<endl;
            return false;
        }
    }

    *ppItem = getItemAtIndex(pParentItem, itemIdx);
    ctxt.setDesc(pData->pDesc);
    ctxt.setItem(*ppItem);
    return true;
}


//
// From index, return the pointer to component item in this aggregate.
// Because this function is called during loading, items are created as
// needed: if the item is contained, the pointer to the already-allocated
// memory is returned, and if it's owned, memory for the item is allocated.
//
// idx: (in) index of wanted component, 1 larger than
//           the index previously passed to this method for the
//           same id in the same composite
// pParentItem: (in) the owning item
// ppItem: (out) the wanted item
//
// @return true if item is returned
//
bool cm_aggregate::getComponentItem(unsigned idx, uint8_t * pParentItem, uint8_t ** ppItem) const
{
    assert(pParentItem != NULL);
    assert(ppItem != NULL);
    
    if (idx >= pData->maxCount)
    {
        // Maximum number of these items already loaded: fail
        return false;
    }

    if (isAddSupported())
    {
        *ppItem = add(pParentItem); 
    }
    else
    {
        *ppItem = getItemAtIndex(pParentItem, idx);
    }

    if (*ppItem == NULL)
    {
        return false;
    }
    return true;
}


// Save to persistent storage all elements in the array
void cm_aggregate::save(const uint8_t *pItem) const
{
    for (unsigned i = 0; i < getCount(pItem); i++)
    {
        pData->pDesc->save(getItemAtIndex(pItem, i));
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
// would restrict the application developer.
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
// xxx giving a fixed size to counters would simplify this, but
// would restrict the application developer.
void cm_owned_aggregate::setCount(uint8_t * pParentItem, unsigned int count) const
{
    assert(pParentItem != NULL);

    if (pCounterAggr == NULL)
    {
        // There is no counter -- it's optional if maxCount == 1
        // xxx what happens if there's no counter but maxCount > 1?
        return;
    }

    DBG_PRT("setCount: %d, %d bytes at %p)\n",
            count, pCounterAggr->pData->pDesc->getLen(), pParentItem + pCounterAggr->pData->offset);

    switch (pCounterAggr->pData->pDesc->getLen()) 
    { 
        case sizeof(uint8_t):
        {
            assert(count <= UINT8_MAX);
            uint8_t cnt = count;
            memcpy(pParentItem + pCounterAggr->pData->offset, &cnt, sizeof(cnt));
            break;
        }
       
        case sizeof(uint16_t):
        {
            assert(count <= UINT16_MAX);
            uint16_t cnt = count;
            memcpy(pParentItem + pCounterAggr->pData->offset, &cnt, sizeof(cnt));
            break;
        }
       
        case sizeof(uint32_t):
        {
            assert(count <= UINT32_MAX);
            uint32_t cnt = count;
            memcpy(pParentItem + pCounterAggr->pData->offset, &cnt, sizeof(cnt));
            break;
        }

        default:
            assert(0);
    }
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


// Add OWNED item.
// @pre Counter is in range
// This allocates memory for the new item, sets it to default values,
// and increments the corresponding counter.
// @return pointer to new allocated memory, or NULL in case of failure
uint8_t * cm_owned_aggregate::add(uint8_t * pParentItem) const
{
    assert(pParentItem != NULL);

    // Reallocate memory, and save pointer in the same location
    unsigned   cnt     = getCount(pParentItem);
    uint8_t ** ppItems = (uint8_t **)(pParentItem + pData->offset);

    DBG_PRT("add: %d * %d at %p + %d (%p), currently %p\n",
            (cnt + 1), pData->pDesc->getLen(),
            pParentItem, pData->offset, ppItems,
            *ppItems);

    uint8_t * pNewMem = (uint8_t *)realloc(*ppItems, (cnt + 1) * pData->pDesc->getLen());

    if (pNewMem == NULL)
    {
        cout << "No " << pData->pDesc->getLen() << " for " << pData->pDesc->getName() << endl;
        return NULL;
    }

    // Memory successfully allocated, so reference the (possibly new) memory
    *ppItems = pNewMem;

    uint8_t * pNewItem = pNewMem + cnt * pData->pDesc->getLen();

    // Initialize added item with default values. First memset to ensure
    // counters, which have no setDefault fn, are 0 (also sets pointers to owned to NULL).
    memset(pNewItem, 0, pData->pDesc->getLen());
    pData->pDesc->setDefault(pNewItem);

    DBG_PRT("add at %p\n", pNewMem);

    setCount(pParentItem, cnt + 1);
    return pNewItem;
}


// Del OWNED item.
// This re-allocates the necessary memory, updates the counter if necessary,
// and sets the pointer to the memory to NULL if it's all been freed.
void cm_owned_aggregate::del(uint8_t * pParentItem, unsigned int itemIdx) const
{
    // Sanity check input parameters
    assert(pParentItem != NULL);

    uint8_t ** ppItems = (uint8_t **)(pParentItem + pData->offset);
    cm_item_len_t componentLen = pData->pDesc->getLen();
    unsigned cnt = getCount(pParentItem);

    assert(*ppItems != NULL);
    assert(cnt > 0);
    assert(itemIdx < cnt);

    DBG_PRT("del at %p index %d len %d\n", *ppItems, itemIdx, componentLen);

    // Shift down items to occupy the memory vacated by deleted item
    memmove(*ppItems + itemIdx * componentLen,
            *ppItems + (itemIdx + 1) * componentLen,
            (cnt - itemIdx - 1) * componentLen);

    // Reallocate memory, and save pointer in the same location
    *ppItems = (uint8_t *)realloc(*ppItems, (cnt - 1) * componentLen);

    // xxx realloc should return NULL if memory to be allocated is 0, but it doesn't seem to...
    if (cnt == 1)
    {
        *ppItems = NULL;
    }
    setCount(pParentItem, cnt - 1);
}


////////////////////////////////////////////////////////////////////////////////
//
// command_stack
//
////////////////////////////////////////////////////////////////////////////////

// Return the operation represented by the word at the top of the command stack
command_stack::eCmOp command_stack::getTopOp() const
{
    if (strcmp(getTop(), "add") == 0)     return CM_ADD;
    if (strcmp(getTop(), "del") == 0)     return CM_DEL;
    if (strcmp(getTop(), "prt") == 0)     return CM_PRT;
    if (strcmp(getTop(), "=") == 0)       return CM_SET;
    if (strcmp(getTop(), "setdef") == 0)  return CM_SETDEF;
    if (strcmp(getTop(), "load") == 0)    return CM_LOAD;
    if (strcmp(getTop(), "save") == 0)    return CM_SAVE;
    if (strcmp(getTop(), "<") == 0)       return CM_RESET_CTXT;
    if (strcmp(getTop(), "?") == 0)       return CM_HELP;

    // If no match, it's not an operation
    return CM_OP_NONE;
}


////////////////////////////////////////////////////////////////////////////////
//
// cm_context
//
////////////////////////////////////////////////////////////////////////////////

// Add a word to the context string
void cm_context::add(string w)
{
    str += w + " ";
}


// Add an unsigned integer to the context string
void cm_context::add(unsigned idx)
{
    char indexbuf[6]; // xxx big enough to avoid truncation in all cases?

    snprintf(indexbuf, sizeof(indexbuf), "%d ", idx);
    str += indexbuf;
}

