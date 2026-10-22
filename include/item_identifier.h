// Abstract identifier strategy.
#pragma once
#include <stdint.h> // uint8_t, etc
#include <string>

namespace cfg_mgr
{

class Command_stack;
class Owned_aggregate;

class Item_identifier
{
public:
    virtual ~Item_identifier() = default;
    virtual bool get_indexEval(Command_stack *cmd, const Owned_aggregate &aggr,
                               const uint8_t *pParentItem, unsigned &itemIdx,
                               unsigned count) const = 0;
    virtual bool get_indexAdd(Command_stack *cmd, const Owned_aggregate &aggr,
                              const uint8_t *pParentItem, unsigned &itemIdx,
                              const char *&key) const = 0;
    virtual bool get_indexDel(Command_stack *cmd, const Owned_aggregate &aggr,
                              const uint8_t *pParentItem, unsigned &itemIdx) const = 0;
    virtual void set_key(const Owned_aggregate &aggr, uint8_t *pItem, const char *k) const = 0;
    virtual std::string getPrintPrefixAdd(const Owned_aggregate &aggr, const uint8_t *pItem,
                                          unsigned index) const = 0;
    virtual std::string extractId(const Owned_aggregate &aggr, Command_stack *cmd,
                                  const uint8_t *pParentItem, unsigned &itemIdx) const = 0;
};

} // namespace cfg_mgr
