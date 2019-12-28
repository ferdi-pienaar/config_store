
#ifndef CFG_MGR_DESCRIPTOR_H
#define CFG_MGR_DESCRIPTOR_H

#include <stdint.h> // uint8_t, etc
#include "cfg_mgr_types.h"
#include "cfg_mgr_metadata.h"

/*
 *
 *              +----------------+
 *              |  descriptor    |
 *              |----------------|
 *              | name           | 1
 *              | id             |<-------------------------------------+
 *              | len            |                                      |
 *              | persistent     |                                      |
 *              +----------------+                                      |
 *                       ^                                              |
 *                      / \                                             |
 *                     -----                                            |
 *                       |                                              |
 *          +------------+------------+                                 |
 *          |                         |                                 |
 * +-------------------+  +----------------------+ 1  * +-----------+ 1 |
 * | simple descriptor |  | composite descriptor +<>--->| aggregate |<>-+
 * |-------------------|  |----------------------|      |-----------|
 *                                                      | maxCount  |
 *                                                      | offset    |
 *                                                      +-----------+
 *                                                            ^
 *                                                           / \
 *                                                          -----
 *                                                            |
 *                                              +-------------+-------+
 *                                              |                     |
 *                                  +---------------------+  +-----------------+
 *                                  | contained aggregate |  | owned aggregate |
 *                                  |---------------------|  |-----------------|
 *
 */

namespace cfg_mgr
{

class Command_stack;
class Cmd_context;
class Store;

////////////////////////////////////////////////////////////////////////////////
/// Descriptor of configurable item (either simple or composite).
// xxx methods are private (not for user), but Config_manager is friend?
class Descriptor
{
public:
    Descriptor() {}
    virtual ~Descriptor() {}

    virtual bool handleCmd(Command_stack * cmd, uint8_t * pItem, Cmd_context * candidate, bool & setCtxt) const = 0;
    virtual const char * getName() const = 0;
    virtual item_id_t getId() const = 0;
    virtual void save(const uint8_t * pItem, Store * store) const = 0;
    virtual result_t startLoad(Store * store) const = 0;
    virtual result_t endLoad(uint8_t * pItem, Store * store) const = 0;
    virtual item_len_t getLen() const = 0;
    virtual void print(const uint8_t * pItem, std::string prefix, bool include_state) const = 0;
    virtual void setDefault(uint8_t * pItem) const = 0;
    virtual void help(const uint8_t * pItem) const = 0;
    virtual bool isPersistent() const = 0;

};


////////////////////////////////////////////////////////////////////////////////
/// A composite descriptor consists of components, linked to the composite via
/// aggregates.
// xxx methods (apart from constructor) are private (not for user), but Config_manager is friend?
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
#endif // CFG_MGR_DESCRIPTOR_H
