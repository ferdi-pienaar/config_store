#ifndef CFG_MGR_CMD_CTXT_H
#define CFG_MGR_CMD_CTXT_H

#include <stdint.h> // uint8_t, etc
#include <string>

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

}
#endif // CFG_MGR_CMD_CTXT_H
