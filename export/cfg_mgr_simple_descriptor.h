
#ifndef CFG_MGR_SIMPLE_DESCRIPTOR_H
#define CFG_MGR_SIMPLE_DESCRIPTOR_H

#include <stdint.h> // uint8_t, etc
#include "cfg_mgr_descriptor.h" // parent class
#include "cfg_mgr_types.h"
#include "cfg_mgr_metadata.h"

namespace cfg_mgr
{

class Command_stack;
class Cmd_context;
class Store;

////////////////////////////////////////////////////////////////////////////////
/// A simple descriptor is a leaf in the tree of descriptors, representing
/// metadata for a single configurable item.
// xxx methods (apart from constructor) are private (not for user), but Config_manager is friend?
class Simple_descriptor : public Descriptor
{
public:
    Simple_descriptor(const Simple_metadata * pMeta):
        m_data(pMeta) {}
    virtual ~Simple_descriptor() {}
    bool handleCmd(Command_stack * cmd, uint8_t * pItem, Cmd_context * candidate, bool & setCtxt) const;
    const char * getName() const
    {
        return m_data->c.name;
    }
    virtual item_id_t getId() const
    {
        return m_data->c.id;
    }
    virtual item_len_t getLen() const
    {
        return m_data->c.len;
    }
    void print(const uint8_t * pItem, std::string prefix, bool include_state) const;
    bool set(uint8_t * pItem, std::string val) const;
    void setDefault(uint8_t * pItem) const;
    void help(const uint8_t * pItem) const;
    virtual void save(const uint8_t * pItem, Store * store) const;
    result_t startLoad(Store * store) const;
    result_t endLoad(uint8_t * pItem, Store * store) const;
    bool isPersistent() const
    {
        return m_data->c.persistent;
    }

private:
    const Simple_metadata * const m_data;
};

}
#endif // CFG_MGR_SIMPLE_DESCRIPTOR_H
