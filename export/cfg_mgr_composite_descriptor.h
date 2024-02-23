
#pragma once

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
/// A composite descriptor consists of components, linked to the composite via
/// aggregates.
// xxx methods (apart from constructor) are private (not for user), but Config_manager_implement is friend?
class Composite_descriptor : public Descriptor
{
public:
    Composite_descriptor(const Composite_metadata * pMeta):
        m_data(pMeta) {}
    ~Composite_descriptor() {};
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
    virtual bool hasContent(const uint8_t *pItem) const;
    bool handleCmd(Command_stack * cmd, uint8_t * pItem, Cmd_context * candidate, bool & setCtxt) const;
    void print(const uint8_t * pItem, std::string prefix, bool include_state) const;
    void setDefault(uint8_t * pItem) const;
    virtual void help(const uint8_t * pItem) const;
    void save(const uint8_t * pItem, Store * store) const;
    result_t startLoad(Store * store) const;
    result_t endLoad(uint8_t * pItem, Store * store) const;
    bool isPersistent() const
    {
        return m_data->c.persistent;
    }

private:
    bool handleAdd(Command_stack * cmd, uint8_t * pItem) const;
    bool handleDel(Command_stack * cmd, uint8_t * pItem) const;
    bool handleIdWord(Command_stack * cmd, uint8_t * pItem, Cmd_context * candidate, bool & setCtxt) const;
    virtual unsigned short getAggrCount() const
    {
        return m_data->aggrCount;
    }
    virtual const Aggregate * getAggrAtIndex(unsigned int i) const
    {
        return m_data->aggrList[i];
    }
    const Aggregate * getAggr(const char * name) const;
    const Aggregate * getAggr(item_id_t id) const;

    const Composite_metadata * const m_data;
};

}
