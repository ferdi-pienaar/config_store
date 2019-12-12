/// config manager def
#include "config_manager_command.h"
#include <string.h> // memset, strcmp, memcpy

using namespace std;

namespace cfg_mgr
{

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
    if (m_count == 0) return false;

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
    m_string += w + " ";
}


// Add an unsigned integer to the context string
void Cmd_context::add(unsigned idx)
{
    char indexbuf[6]; // xxx big enough to avoid truncation in all cases?

    snprintf(indexbuf, sizeof(indexbuf), "%d ", idx);
    m_string += indexbuf;
}

}
