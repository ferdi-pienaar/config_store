
#ifndef CFG_MAN_COMMAND_H
#define CFG_MAN_COMMAND_H

#include <stdint.h> // uint8_t, etc
#include <iostream>
#include "config_manager_types.h"

namespace cfg_mgr
{

class Descriptor;

// xxx should not be exported
// The context in which a command string is interpreted:
// the current item, its descriptor (i.e. its metadata), and the
// string that's displayed on the command-line to represent the context, i.e. the location
// of the item within the hierarchy of items.
class Cmd_context
{
public:
   

   Cmd_context (std::string istr = "", const Descriptor * desc = NULL, uint8_t * item = NULL):
        str(istr), pDesc(desc), pItem(item) {}
    void add(std::string w);
    void add(unsigned idx);
    void setDesc(const Descriptor * desc)
    {
        pDesc = desc;
    }

    void setItem(uint8_t * item)
    {
        pItem = item;
    }

    std::string getString() const
    {
        return str;
    }
    const Descriptor * getDesc() const
    {
        return pDesc;
    }
    uint8_t * getItem() const
    {
        return pItem;
    }

private:
    std::string           str;
    const Descriptor *    pDesc;
    uint8_t *             pItem;
};

// Container for the command passed to cfg_mgr.  It's a stack of words, i.e.
// a stack of C-strings, each string consisting of 1 word only (i.e. no spaces).
class Command_stack
{
public:
    // Operations - each represents a reserved 'word' in commands passed to Config_manager
    enum eCmOp
    {
        CM_ADD,
        CM_DEL,
        CM_PRT,
        CM_PRT_CFG,
        CM_SET,
        CM_SETDEF,
        CM_LOAD,
        CM_SAVE,
        CM_HELP,       //
        CM_RESET_CTXT, // return context to top level
        CM_OP_NONE
    };

    Command_stack(int argc, char ** argv) : count(argc), wordPtr(argv) {}

    // Pop top word.
    // Return ref to self so the value returned by the command can be passed to a fn
    Command_stack & pop()
    {
        count--;
        wordPtr++;
        return *this;
    }

    char * getTop() const
    {
        return wordPtr[0];
    }

    int getCount() const
    {
        return count;
    }

    eCmOp getTopOp() const;
    bool getIndex(unsigned int & itemIdx);

private:
    int     count;
    char ** wordPtr;
};
}
#endif // CFG_MAN_COMMAND_H
