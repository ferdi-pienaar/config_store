#ifndef CFG_MGR_CMD_STACK_H
#define CFG_MGR_CMD_STACK_H

namespace cfg_mgr
{

// Container for the command passed to cfg_mgr.  It's a stack of tokens, i.e.
// a stack of C-strings, each string consisting of 1 token (such as a word or a quoted block).
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
        CM_OP_NONE, // not a keyword
        CM_EMPTY // client passed an empty command stack.
    };

    Command_stack(int argc, char ** argv) : m_count(argc), m_tokenPtr(argv) {}
    Command_stack & pop();
    char * getTop() const;
    int getCount() const
    {
        return m_count;
    }
    eCmOp getTopOp() const;
    bool popIndex(unsigned int & itemIdx);

private:
    int     m_count;
    char ** m_tokenPtr;
};
}
#endif // CFG_MGR_CMD_STACK_H
