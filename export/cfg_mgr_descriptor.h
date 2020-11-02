
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

}
#endif // CFG_MGR_DESCRIPTOR_H
