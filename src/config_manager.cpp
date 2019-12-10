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

namespace cfg_mgr
{

////////////////////////////////////////////////////////////////////////////////
//
// Config_manager
//
////////////////////////////////////////////////////////////////////////////////

Config_manager::Config_manager(const Descriptor * desc): base_desc(desc)
{
    ramBase = (uint8_t *)malloc(base_desc->getLen());
    DBG_PRT("init: ramBase, %d at %p\n", base_desc->getLen(), ramBase);
    assert(ramBase != NULL);
    memset(ramBase, 0, base_desc->getLen());
    base_desc->setDefault(ramBase);
    resetCtxt();
    store = Store::getStore();
}

Config_manager::~Config_manager()
{
    free(ramBase);
    delete store; // xxx is this clean, is delete the obvious pair to getStore (in Config_manager constructor)?
}

/// Execute command words entered by client on CLI cpp file
/// @param argc number of command words
/// @param argv command word array
void Config_manager::handleCmd(int argc, char *argv[])
{
    Command_stack cmd(argc, argv);
    bool updateCtxt = false;

    // First treat the commands that are only applicable at the top level
    switch (cmd.getTopOp())
    {
    case Command_stack::CM_LOAD:
        return load();

    case Command_stack::CM_SAVE:
        return save();

    case Command_stack::CM_RESET_CTXT:
        return resetCtxt();

    default: // Other commands are passed to current context
        break;
    }

    // The candidate context starts as a copy of the current context
    candidateCtxt = currCtxt;

    // Pass command that doesn't apply to CM as a whole, to current context for handling
    currCtxt.getDesc()->handleCmd(&cmd, currCtxt.getItem(), &candidateCtxt, updateCtxt);
    if (updateCtxt)
    {
        currCtxt = candidateCtxt;
    }
}


// Set context back to base
void Config_manager::resetCtxt()
{
    DBG_PRT("resetCtxt\n");
    currCtxt = Cmd_context("", base_desc, ramBase); // temp context with base properties
}


// Get a prompt string to display to user, representing the current context
const char * Config_manager::getPromptString() const
{
    return currCtxt.getString().c_str();
}


// Save data in RAM to persistent storage
void Config_manager::save()
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
void Config_manager::load()
{
    store->initForRead();

    // Before loading, thus allocating new memory, call setDefault to free owned memory
    base_desc->setDefault(ramBase);
    result_t res = base_desc->startLoad(store);
    if (res == CM_SUCCESS)
    {
        res = base_desc->endLoad(ramBase, store);
    }
    if (res != CM_SUCCESS)
    {
        cm_printf("Load failed: defaults restored.\n");
        base_desc->setDefault(ramBase);
        return;
    }
    resetCtxt();
}


////////////////////////////////////////////////////////////////////////////////
//
// Command_stack
//
////////////////////////////////////////////////////////////////////////////////

// Return the operation represented by the word at the top of the command stack
Command_stack::eCmOp Command_stack::getTopOp() const
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


// Extract index from top word in command stack and pop it.
bool Command_stack::getIndex(unsigned int & itemIdx)
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
// Cmd_context
//
////////////////////////////////////////////////////////////////////////////////

// Add a word to the context string
void Cmd_context::add(string w)
{
    str += w + " ";
}


// Add an unsigned integer to the context string
void Cmd_context::add(unsigned idx)
{
    char indexbuf[6]; // xxx big enough to avoid truncation in all cases?

    snprintf(indexbuf, sizeof(indexbuf), "%d ", idx);
    str += indexbuf;
}

}
