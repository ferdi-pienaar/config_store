
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
/// A simple descriptor is a leaf in the tree of descriptors, representing
/// metadata for a single configurable item.
// xxx methods (apart from constructor) are private (not for user), but Config_manager_implement is friend?
class Simple_descriptor : public Descriptor
{
public:
    Simple_descriptor(const Simple_metadata * pMeta):
        m_data(pMeta) {}
    virtual ~Simple_descriptor() {}
    void mgrInit(const Mgr_service *serv) override
    {
        m_mgr_service = serv;
    };
    bool evalCmd(Command_stack * cmd, uint8_t * pItem, Command_stack::eCmOp &op) const override;
    bool handleCmd(Command_stack * cmd, uint8_t * pItem, Cmd_context * candidate, bool & setCtxt) const override;
    const char * getName() const override
    {
        return m_data->c.name;
    }
    virtual item_id_t getId() const override
    {
        return m_data->c.id;
    }
    virtual item_len_t getLen() const override
    {
        return m_data->c.len;
    }
    virtual bool hasPersistentContent(const uint8_t *pItem) const override
    {
        (void)pItem;
        return m_data->c.persistent;
    }
    void print(const uint8_t * pItem, std::string prefix, bool include_state) const override;
    bool set(uint8_t * pItem, std::string val) const;
    void setDefault(uint8_t * pItem) const override;
    void help(const uint8_t * pItem) const override;
    virtual void save(const uint8_t * pItem) const override;
    Result startLoad() const override;
    Result endLoad(uint8_t * pItem) const override;
    bool isPersistent() const override
    {
        return m_data->c.persistent;
    }

private:
    bool evalSet(uint8_t * pItem, std::string val) const;
    const Simple_metadata * const m_data;
};

}
