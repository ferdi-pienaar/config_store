
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
    Cmd_context (std::string istr = "", const Descriptor * desc = nullptr, uint8_t * item = nullptr):
        m_string(istr), m_desc(desc), m_item(item) {}
    void add(std::string w);
    void add(unsigned idx);
    void setDesc(const Descriptor * desc)
    {
        m_desc = desc;
    }

    void setItem(uint8_t * item)
    {
        m_item = item;
    }

    std::string getString() const
    {
        return m_string;
    }
    const Descriptor * getDesc() const
    {
        return m_desc;
    }
    uint8_t * getItem() const
    {
        return m_item;
    }

private:
    std::string           m_string;
    const Descriptor *    m_desc;
    uint8_t *             m_item;
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

    Command_stack(int argc, char ** argv) : m_count(argc), m_wordPtr(argv) {}

    // Pop top word.
    // Return ref to self so the value returned by the command can be passed to a fn
    Command_stack & pop()
    {
        m_count--;
        m_wordPtr++;
        return *this;
    }

    char * getTop() const
    {
        return m_wordPtr[0];
    }

    int getCount() const
    {
        return m_count;
    }

    eCmOp getTopOp() const;
    bool getIndex(unsigned int & itemIdx);

private:
    int     m_count;
    char ** m_wordPtr;
};
}
#endif // CFG_MAN_COMMAND_H
