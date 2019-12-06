/// config manager def
#include "config_manager.h"
#include "config_manager_util.h"
#include "config_manager_dbg.h"
#include "config_manager_store.h"
#include "config_manager_printf.h"

#include <stdlib.h> // malloc
#include <string.h> // memset, strcmp, memcpy
#include <stdint.h> // UINT8_MAX, etc

#include <sstream>
using namespace std;

////////////////////////////////////////////////////////////////////////////////
//
// config_manager
//
////////////////////////////////////////////////////////////////////////////////

config_manager::config_manager(): base_desc(NULL), ramBase(NULL)
{
    store = cm_store::getStore();
}


// Initialize config manager: allocate and populate item memory in RAM.
// xxx We could presumably do these things in the constructor, but
// having an init method gives us more flexibility in delaying certain
// actions until later; this allows us to create the CM early in the
// init cycle, but delay malloc and NVRAM reads until later.
// xxx there are things duplicated here and in load()
void config_manager::init(const cm_descriptor * desc)
{
    base_desc = desc;
    ramBase = (uint8_t *)malloc(base_desc->getLen());

    DBG_PRT("init: ramBase, %d at %p\n", base_desc->getLen(), ramBase);

    assert(ramBase != NULL);

    // This could be done in the constructor, but I do it here to make unit tests
    // independent (since the constructor won't run at the beginning of each test).
    resetCtxt();

    // Set counters to 0 and pointers to NULL on fresh memory before setDefault during load.
    memset(ramBase, 0, base_desc->getLen());
    load();
}


/// Execute command words entered by client on CLI cpp file
/// @param argc number of command words
/// @param argv command word array
void config_manager::handleCmd(int argc, char *argv[])
{
    command_stack cmd(argc, argv);
    bool setCtxt = false;

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

    // The candidate context starts as a copy of the current context
    candidateCtxt = currCtxt;

    // Pass command that doesn't apply to CM as a whole, to current context for handling
    currCtxt.getDesc()->handleCmd(&cmd, currCtxt.getItem(), &candidateCtxt, setCtxt);
    if (setCtxt)
    {
        updateCtxt();
    }
}


// Set context back to base
void config_manager::resetCtxt()
{
    DBG_PRT("resetCtxt\n");
    cm_context temp("", base_desc, ramBase); // temp context with base properties
    currCtxt = temp;
}


// Get a prompt string to display to user, representing the current context
const char * config_manager::getPromptString() const
{
    return currCtxt.getString().c_str();
}


// Save data in RAM to persistent storage
void config_manager::save()
{
    if (!base_desc->isPersistent())
    {
        return;
    }
    store->initForWrite();
    base_desc->save(ramBase, store);
}


// Load data in persistent storage, to configurable items in RAM.
// Resets context, since a reload re-allocates memory and makes current context invalid
void config_manager::load()
{
    if (!loadBaseId()) return;

    // Before loading, thus allocating new memory, call setDefault to free owned memory
    base_desc->setDefault(ramBase);
    t_cm_result res = base_desc->startLoad(store);
    // xxx this is where we should check if base ID is in store -- are we doing it elsewhere??
    res = base_desc->endLoad(ramBase, store);

    if (res != CM_SUCCESS)
    {
        cm_printf("Load failed: defaults restored.\n");
        base_desc->setDefault(ramBase);
        return;
    }
    resetCtxt();
}


// Return true if expected base ID is loaded from the store
// xxx I think I don't need this anymore, except for initForRead.
bool config_manager::loadBaseId()
{
    if (!base_desc->isPersistent())
    {
        cm_printf("No persistent items.\n");
        return false;
    }

    if (!store->initForRead())
    {
        cm_printf("No config file.\n");
        return false;
    }
    cm_printf("Load id %#x.\n", base_desc->getId());
    return true;
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
// @param pItem - pointer to RAM where item is located
//
// @return true if command was handled
//
// xxx should free memory allocated as side-effect of a non-set or
//     go-to-node command, not just an invalid command.
//
bool cm_composite_descriptor::handleCmd(command_stack * cmd,
                                        uint8_t * pItem,
                                        cm_context * candidateContext,
                                        bool & setCtxt) const
{
    DBG_PRT("composite::handleCmd: %s\n", cmd->getTop());

    assert(pItem != NULL);

    switch (cmd->getTopOp())
    {
    case command_stack::CM_OP_NONE:
        return handleIdWord(cmd, pItem, candidateContext, setCtxt);

    case command_stack::CM_ADD:
        // Remove the word 'add' and pass the remainder to the method
        return handleAdd(&cmd->pop(), pItem);

    case command_stack::CM_DEL:
        // Remove the word 'del' and pass the remainder to the method
        return handleDel(&cmd->pop(), pItem);

    case command_stack::CM_PRT:
        print(pItem, "", true);
        return true;

    case command_stack::CM_PRT_CFG:
        print(pItem, "", false);
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
    cm_printf("Command '%s' not handled in composite item '%s'\n", cmd->getTop(), getName());
    return false;
}


// Handle word in command string that's not a reserved command word,
// hence presumably it identifies a component
//
// @return true if a word was from cmd was parsed
//
bool cm_composite_descriptor::handleIdWord(command_stack * cmd, uint8_t * pItem, cm_context * candidateCtxt, bool & setCtxt) const
{
    const cm_aggregate * pAggr = getAggr(cmd->getTop()); // Component that is identified by cmd

    if (pAggr == NULL)
    {
        // Unhandled word(s): not a command, and also doesn't identify a component
        cm_printf("'%s' not in composite '%s'.\n", cmd->getTop(), getName());
        return false;
    }

    candidateCtxt->add(pAggr->pData->pDesc->getName());

    bool      added;           // Did getComponentItem create a new item?
    uint8_t * pComponentItem;  // pointer to component RAM

    if (!pAggr->getComponentItem(&cmd->pop(), pItem, &pComponentItem, added, candidateCtxt))
    {
        // Index problems are reported by the called fn
        return false;
    }

    // A component was found
    if (cmd->getCount() == 0)
    {
        // We have a component, but there are no more words in the command
        setCtxt = true;
        return true;
    }

    // Pass the remainder of the command to the found component
    if (!pAggr->pData->pDesc->handleCmd(cmd, pComponentItem, candidateCtxt, setCtxt))
    {
        // Component says the command is invalid
        if (added)
        {
            // Free memory allocated by a command that turns out to be invalid
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

    if (cmd->getCount() != 1)
    {
        cm_printf("%u parameters for 'add'.\n", cmd->getCount());
        return false;
    }

    const cm_aggregate * pAggr = getAggr(cmd->getTop());
    if (pAggr == NULL)
    {
        cm_printf("'%s' not in composite '%s'.\n", cmd->getTop(), getName());
        return false;
    }
    return pAggr->handleAdd(pItem);
}


// Del an owned component named by cmd from a composite
// @return true if the operation was successful, false if it failed.
bool cm_composite_descriptor::handleDel(command_stack * cmd, uint8_t * pItem) const
{
    DBG_PRT("handleDel %s\n", cmd->getTop());

    if ((cmd->getCount() != 1) && (cmd->getCount() != 2))
    {
        // Provide item name and, optionally, index
        cm_printf("%u parameters for 'del'.\n", cmd->getCount());
        return false;
    }

    const cm_aggregate * pAggr = getAggr(cmd->getTop());

    if (pAggr == NULL)
    {
        cm_printf("'%s' not in composite '%s'.\n", cmd->getTop(), getName());
        return false;
    }
    return pAggr->handleDel(cmd, pItem);
}


// Delegate print command to components
//
void cm_composite_descriptor::print(const uint8_t * pItem, string prefix, bool include_state) const
{
    DBG_PRT("print composite %s len %d include_state=%d\n", getName(), getLen(), include_state);

    if (!include_state && !pData->c.persistent)
    {
        // The item is not persistent, i.e. state, so exclude it because not required
        return;
    }

    for (unsigned i = 0; i < pData->aggrCount; i++)
    {
        getAggrAtIndex(i)->print(pItem, prefix, include_state);
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
    // Set each component to default
    for (unsigned i = 0; i < pData->aggrCount; i++)
    {
        getAggrAtIndex(i)->setDefault(pItem);
    }
}


// Give help for each component.
void cm_composite_descriptor::help(const uint8_t * pItem) const
{
    for (unsigned i = 0; i < pData->aggrCount; i++)
    {
        getAggrAtIndex(i)->help(pItem);
    }
}


// Look for the aggregate whose component has a matching name
const cm_aggregate * cm_composite_descriptor::getAggr(const char * name) const
{
    for (unsigned i = 0; i < pData->aggrCount; i++)
    {
        if (strcmp(name, getAggrAtIndex(i)->pData->pDesc->getName()) == 0)
        {
            return getAggrAtIndex(i);
        }
    }
    return NULL;
}


// Look for the aggregate whose component has a matching ID
// @return aggregate, or NULL if ID does not identify an aggregate in this context
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


/// Save item to persistent storage
//
void cm_composite_descriptor::save(const uint8_t *pItem, cm_store * store) const
{
    DBG_PRT("%s: %s (%hx)\n", __PRETTY_FUNCTION__, pData->c.name, pData->c.id);
    store->startWriteComposite(pData);

    for (unsigned i = 0; i < getAggrCount(); i++)
    {
        getAggrAtIndex(i)->save(pItem, store);
    }
    store->endWriteComposite();
}


// Prepare to load item from persistent storage -- check if it exists in store.
//
//
t_cm_result cm_composite_descriptor::startLoad(cm_store * store) const
{
    t_cm_result ret = store->startLoadComposite(pData);
    DBG_PRT("%s: %s (%hx) res=%d\n", __PRETTY_FUNCTION__, pData->c.name, pData->c.id, ret);
    return ret;
}

// Load item from persistent storage.
// For a composite item, this means loading the components, then closing.
//
// @param pItem (input) - pointer to the RAM memory where loaded values will be saved.
//
// @pre -- this items startLoad was successful, i.e. an unread instance of this
//         remains in the store.
//
t_cm_result cm_composite_descriptor::endLoad(uint8_t * pItem, cm_store * store) const
{
    for (unsigned i = 0; i < getAggrCount(); i++)
    {
        t_cm_result ret = getAggrAtIndex(i)->load(pItem, store);
        if (!((ret == CM_SUCCESS) || (ret == CM_NOT_FOUND)))
        {
            // An unexpected error, such as unexpected end of store
            // or INCOHERENT (L of simple item in store did not match
            // the amount we tried to read).
            return ret;
        }
    }
    DBG_PRT("%s: %s (%hx)\n", __PRETTY_FUNCTION__, pData->c.name, pData->c.id);
    return store->endLoadComposite();
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
void cm_simple_descriptor::print(const uint8_t * pItem, string prefix, bool include_state) const
{
    DBG_PRT("print simple %s len %d at %p include_state=%d\n", getName(), getLen(), pItem, include_state);

    cm_printf("%s= ", prefix.c_str());

    if (pData->pPrt == NULL)
    {
        // No function installed so use default print function: hex chars
        cm_printf("%s", cm_prt_hexstr(pItem, getLen()).c_str());
    }
    else
    {
        cm_printf("%s", pData->pPrt(pItem, getLen()).c_str());
    }
    cm_printf("\n");
}


//
// cmd - array of strings containing name elements
// pItem - pointer to RAM where item is located
//
bool cm_simple_descriptor::handleCmd(command_stack * cmd, uint8_t * pItem, cm_context *candidateCtxt, bool & setCtxt) const
{
    DBG_PRT("simple cmd at %p\n", pItem);

    switch (cmd->getTopOp())
    {
    case command_stack::CM_PRT:
        print(pItem, "", true);
        return true;

    case command_stack::CM_PRT_CFG:
        print(pItem, "", false);
        return true;

    case command_stack::CM_SET:
        if (cmd->pop().getCount() == 1)
        {
            return set(pItem, cmd->getTop());
        }
        break;

    case command_stack::CM_SETDEF:
        setDefault(pItem);
        return true;

    case command_stack::CM_HELP:
        help(pItem);
        return true; // true?

    default:
        cm_printf("'%s' not in simple '%s'.\n", cmd->getTop(), getName());
    }
    return false;
}


// Set item to a value input as string on command line
bool cm_simple_descriptor::set(uint8_t * pItem, string val) const
{
    DBG_PRT("set simple %s at %p to '%s'\n", getName(), pItem, val.c_str());

    if (pData->pSet != NULL)
    {
        return pData->pSet(pItem, getLen(), val);
    }
    cm_printf("'%s' can't be set.\n", getName());
    return false;
}


// Set configurable item to its default value.
void cm_simple_descriptor::setDefault(uint8_t * pItem) const
{
    if (pData->pSetDefault != NULL)
    {
        pData->pSetDefault(pItem, getLen());
    }
}


/// Save item to persistent storage
void cm_simple_descriptor::save(const uint8_t *pItem, cm_store * store) const
{
    DBG_PRT("%s: %s (%hx)\n", __PRETTY_FUNCTION__, pData->c.name, pData->c.id);
    store->writeSimple(pData, pItem);
}


// @param pItem
t_cm_result cm_simple_descriptor::startLoad(cm_store * store) const
{
    t_cm_result ret = store->startLoadSimple(pData)
    DBG_PRT("%s: %s (%hx) res=%d\n", __PRETTY_FUNCTION__, pData->c.name, pData->c.id, ret);
    return ret;
}

// @param pItem
t_cm_result cm_simple_descriptor::endLoad(uint8_t * pItem, cm_store * store) const
{
    t_cm_result ret = store->endLoadSimple(pItem, pData);
    DBG_PRT("%s: %s (%hx) res=%d\n", __PRETTY_FUNCTION__, pData->c.name, pData->c.id, ret);
    return ret;
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
bool cm_aggregate::getIndex(command_stack * cmd, unsigned int & itemIdx) const
{
    if (cmd->getIndex(itemIdx)) return true;
    cm_printf("'%s' needs index.\n", pData->pDesc->getName());
    return false;
}


// Return pointer to item, given parent item and index
uint8_t * cm_aggregate::getItemAtIndex(const uint8_t * pParentItem, unsigned idx) const
{
    if (idx >= getCount(pParentItem))
    {
        return NULL;
    }
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
void cm_aggregate::print(const uint8_t * pItem, std::string prefix, bool include_state) const
{
    if (!include_state && !pData->pDesc->isPersistent())
    {
        // The item is not persistent, i.e. state, so exclude it because not required
        return;
    }

    char indexbuf[6] = {0}; // xxx big enough to avoid truncation in all cases?

    for (unsigned i = 0; i < getCount(pItem); i++)
    {
        if (pData->maxCount > 1)
        {
            // There can be more than one item, so print the index to distinguish among them
            snprintf(indexbuf, sizeof(indexbuf), " %d", i);
        }
        pData->pDesc->print(getItemAtIndex(pItem, i),
                            prefix + pData->pDesc->getName() + indexbuf + " ",
                            include_state);
    }
}


// From remaining command-line words, find component item of this aggregate.
// If the item does not exist, it is created in certain cases.
// First, look in the command words for an integer representing an index.
//
// @param cmd - command string stack
// @param pParentItem: (in) the owning item
// @param ppItem: (out) the wanted item
// @param added: (out) did this function allocate memory for the item?
//
// @return true if item is returned, false if no index, or index out of range
//
bool cm_aggregate::getComponentItem(command_stack * cmd,
                                    uint8_t * pParentItem,
                                    uint8_t ** ppItem,
                                    bool & added, cm_context * candidateCtxt) const
{
    added = false; // By default, didn't add a new component
    unsigned int itemIdx = 0; // If no index is needed, we'll use offset 0

    if (pData->maxCount > 1)
    {
        // There can be more than one instance, so we need an explicit index
        if (!getIndex(cmd, itemIdx))
        {
            // The necessary index was not in the command
            return false;
        }
        // Index is available: add it to the context string
        candidateCtxt->add(itemIdx);
    }

    DBG_PRT("getComponentItem %p offset %d idx %d cnt %d len %d\n",
            *ppItem, pData->offset, itemIdx, getCount(pParentItem), pData->pDesc->getLen());

    if (itemIdx >= getCount(pParentItem))
    {
        // We may add a new RAM item, depending on index and aggregate type
        if ((*ppItem = addImplicit(itemIdx, pParentItem)) == NULL)
        {
            cm_printf("Index %u out of range.\n", itemIdx);
            return false;
        }
        added = true;
    }

    *ppItem = getItemAtIndex(pParentItem, itemIdx);
    candidateCtxt->setDesc(pData->pDesc);
    candidateCtxt->setItem(*ppItem);
    return true;
}


// Save to persistent storage all elements in the array, if its metadata says it's a persistent item
void cm_aggregate::save(const uint8_t *pItem, cm_store * store) const
{
    if (!pData->pDesc->isPersistent())
    {
        return;
    }
    for (unsigned i = 0; i < getCount(pItem); i++)
    {
        pData->pDesc->save(getItemAtIndex(pItem, i), store);
    }
}


// Load an item (or array) from persistent storage into RAM, which may be
// allocated (if owned) or retrieved (if contained, thus already allocated).
//
// @param pParentItem - base RAM address where loaded items are stored.
//
// @return CM_SUCCESS if item successfully loaded from store (it may have
//           been saved into RAM or dumped)
//         else an indication of why store load failed
t_cm_result cm_aggregate::load(uint8_t * pParentItem, cm_store * store) const
{
    if (!pData->pDesc->isPersistent())
    {
        return CM_SUCCESS;
    }

    for (unsigned idx = 0; idx < pData->maxCount; idx++)
    {
        // This fails if there isn't an item in the store to load.
        t_cm_result res = pData->pDesc->startLoad(store);
        if (res != CM_SUCCESS)
        {
            return res;
        }
        
        // There is an item in the store, so get RAM for it --
        // in case of an owned aggregate, this allocates memory.
        uint8_t * pItem = getComponentItem(idx, pParentItem);
        if (pItem == NULL)
        {
            // Memory couldn't be allocated for the item.
            return CM_SUCCESS; //xxxx success??
        }
        
        // We have memory to load the item into, so complete the load.
        res = pData->pDesc->endLoad(pItem, store);
        if (res != CM_SUCCESS)
        {
            return res;
        }
    }
    return CM_SUCCESS;
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
    return (uint8_t *)(pParentItem + pData->offset);
}


// Return the number of items in the component's array.
// For a contained component, the count is fixed at maxCount.
unsigned cm_contained_aggregate::getCount(const uint8_t * pParentItem) const
{
    return pData->maxCount;
}


// Handle command 'add' on command line
bool cm_contained_aggregate::handleAdd(uint8_t * pItem) const
{
    cm_printf("'add' not supported for contained '%s'.\n", pData->pDesc->getName());
    return false;
}


// Handle command 'del' on command line
bool cm_contained_aggregate::handleDel(command_stack * cmd, uint8_t * pItem) const
{
    cm_printf("'del' not supported for contained '%s'.\n", pData->pDesc->getName());
    return false;
}


// Implicit add is not supported for contained components, because add isn't supported
uint8_t * cm_contained_aggregate::addImplicit(unsigned int itemIdx, uint8_t * pParentItem) const
{
    return NULL;
}


//
// From index, return the pointer to component item in this aggregate.
//
// idx: (in) index of wanted component
// pParentItem: (in) the owning item
//
// @return the wanted item, or NULL
//
uint8_t * cm_contained_aggregate::getComponentItem(unsigned idx, uint8_t * pParentItem) const
{
    return getItemAtIndex(pParentItem, idx);
}


// Give name, count
void cm_contained_aggregate::help(const uint8_t * pItem) const
{
    cm_printf("%s [%u]\n", pData->pDesc->getName(), getCount(pItem));
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
    return *(uint8_t **)(pParentItem + pData->offset); // location is a pointer to the OWNED item
}


// Return the number of items in the component's array
// xxx giving a fixed size to counters would simplify this, but
// would restrict the application developer.
unsigned cm_owned_aggregate::getCount(const uint8_t * pParentItem) const
{
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
    if (pCounterAggr == NULL)
    {
        // There is no counter -- it's optional if maxCount == 1
        // xxx what happens if there's no counter but maxCount > 1?
        return;
    }

    DBG_PRT("%s: %s count=%d, %d bytes at %p\n",
            __PRETTY_FUNCTION__, pData->pDesc->getName(), count,
            pCounterAggr->pData->pDesc->getLen(), pParentItem + pCounterAggr->pData->offset);

    assert(count <= pData->maxCount);

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

    if (*ppItems == NULL)
    {
        // Sanity check: if the pointer is NULL, there are no items.
        assert(getCount(pParentItem) == 0);
        return;
    }

    // Sanity check: there's a pointer to items, so there are items to free
    assert(getCount(pParentItem) > 0);
    DBG_PRT("freeItems: %u at %p\n", getCount(pParentItem), *ppItems);
    free(*ppItems);
    *ppItems = NULL;
    setCount(pParentItem, 0);
}


// Handle command 'add' on command line
bool cm_owned_aggregate::handleAdd(uint8_t * pItem) const
{
    if (getCount(pItem) >= pData->maxCount)
    {
        cm_printf("Can't add '%s' (max %u).\n", pData->pDesc->getName(), pData->maxCount);
        return false;
    }

    if (add(pItem) == NULL)
    {
        return false;
    }
    return true;
}


// Handle command 'del' on command line
bool cm_owned_aggregate::handleDel(command_stack * cmd, uint8_t * pItem) const
{
    unsigned int cnt = getCount(pItem); // number of items currently in array

    if (cnt == 0)
    {
        cm_printf("Currently no '%s'.\n", pData->pDesc->getName());
        return false;
    }

    unsigned int itemIdx = 0; // If no explicit index is needed, use 0 offset

    if (needIndex(pItem) && !getIndex(&cmd->pop(), itemIdx))
    {
        // An index is needed but couldn't be extracted from the command
        return false;
    }

    if (itemIdx >= cnt)
    {
        cm_printf("Index %u out of range (0.. %u).\n", itemIdx, cnt-1);
        return false;
    }
    del(pItem, itemIdx);
    return true;
}


// Add OWNED item in RAM.
// @pre Counter is in range
// This allocates memory for the new item, sets it to default values,
// and increments the corresponding counter.
// @return pointer to new allocated memory, or NULL in case of failure
uint8_t * cm_owned_aggregate::add(uint8_t * pParentItem) const
{
    // Reallocate memory, and save pointer in the same location
    unsigned   cnt     = getCount(pParentItem);
    uint8_t ** ppItems = (uint8_t **)(pParentItem + pData->offset);

    assert(cnt < pData->maxCount);

    DBG_PRT("%s: %d * %d at %p + %d (%p), currently %p\n",
            __PRETTY_FUNCTION__, cnt+1, pData->pDesc->getLen(), pParentItem, pData->offset, ppItems, *ppItems);

    uint8_t * pNewMem = (uint8_t *)realloc(*ppItems, (cnt + 1) * pData->pDesc->getLen());
    if (pNewMem == NULL)
    {
        cm_printf("No %u for %s\n", pData->pDesc->getLen(), pData->pDesc->getName());
        return NULL;
    }

    // Memory successfully allocated, so reference the (possibly new) memory
    *ppItems = pNewMem;

    uint8_t * pNewItem = pNewMem + cnt * pData->pDesc->getLen();

    // Initialize added item with default values. First memset to ensure
    // counters, which have no setDefault fn, are 0 (also sets pointers to owned to NULL).
    memset(pNewItem, 0, pData->pDesc->getLen());
    pData->pDesc->setDefault(pNewItem);

    DBG_PRT("%s: pNewMem=%p\n", __PRETTY_FUNCTION__, pNewMem);

    setCount(pParentItem, cnt + 1);
    return pNewItem;
}


// Del OWNED item.
// This re-allocates the necessary memory, updates the counter if necessary,
// and sets the pointer to the memory to NULL if it's all been freed.
void cm_owned_aggregate::del(uint8_t * pParentItem, unsigned int itemIdx) const
{
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


// Implicit add a RAM item, i.e. add an item because it is referenced by
// a command that is not an explicit 'add'. This allows the client to re-create
// configuration by "playing back" the output from command "prt".
// @return a pointer to the new item, if the index is one larger than the current
// largest item index, and in-range; else NULL.
//
uint8_t * cm_owned_aggregate::addImplicit(unsigned int itemIdx, uint8_t * pParentItem) const
{
    DBG_PRT("%s: %s idx=%d cnt=%d\n",
            __PRETTY_FUNCTION__, pData->pDesc->getName(), itemIdx, getCount(pParentItem));

    if ((itemIdx == getCount(pParentItem)) && (itemIdx < pData->maxCount))
    {
        // Index refers to an item to create
        return add(pParentItem);
    }
    return NULL;
}


// From index, return the pointer to component item in this aggregate.
// Because this function is called during loading, items are created as needed.
//
// idx: (in) index of wanted component, 1 larger than the index previously
//           passed to this method for the same id in the same composite
// pParentItem: (in) the owning item
//
// @return the wanted item, or NULL
//
uint8_t * cm_owned_aggregate::getComponentItem(unsigned idx, uint8_t * pParentItem) const
{
    return addImplicit(idx, pParentItem);
}


// Give name, current count, and maxcount.
void cm_owned_aggregate::help(const uint8_t * pItem) const
{
    cm_printf("%s [%u/%u]\n", pData->pDesc->getName(), getCount(pItem), pData->maxCount);
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
    if (strcmp(getTop(), "prtc") == 0)    return CM_PRT_CFG;
    if (strcmp(getTop(), "=") == 0)       return CM_SET;
    if (strcmp(getTop(), "setdef") == 0)  return CM_SETDEF;
    if (strcmp(getTop(), "load") == 0)    return CM_LOAD;
    if (strcmp(getTop(), "save") == 0)    return CM_SAVE;
    if (strcmp(getTop(), "<") == 0)       return CM_RESET_CTXT;
    if (strcmp(getTop(), "?") == 0)       return CM_HELP;
    return CM_OP_NONE;
}


// extract index from top word in command stack and pop it
bool command_stack::getIndex(unsigned int & itemIdx)
{
    if (count == 0) return false;

    char * pEnd; // pointer to char after chars accepted by strtoul
    itemIdx = strtoul(getTop(), &pEnd, 0);
    if (pEnd == getTop())
    {
        // strtoul didn't get an index from the word
        return false;
    }
    pop();
    return true;
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

