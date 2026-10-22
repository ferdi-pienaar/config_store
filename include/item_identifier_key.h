// Concrete identifier strategy.
#pragma once
#include "item_identifier.h"

namespace cfg_mgr
{

// On the command line interface, a key that is a string identifies an owned item.
class Item_identifier_key : public Item_identifier
{
public:
    bool get_indexEval(Command_stack *cmd, const Owned_aggregate &aggr, const uint8_t *pParentItem,
                       unsigned &itemIdx, unsigned count) const override;
    bool get_indexAdd(Command_stack *cmd, const Owned_aggregate &aggr, const uint8_t *pParentItem,
                      unsigned &itemIdx, const char *&key) const override;
    bool get_indexDel(Command_stack *cmd, const Owned_aggregate &aggr, const uint8_t *pParentItem,
                      unsigned &itemIdx) const override;
    void set_key(const Owned_aggregate &aggr, uint8_t *pItem, const char *k) const override;
    std::string getPrintPrefixAdd(const Owned_aggregate &aggr, const uint8_t *pItem,
                                  unsigned index) const override;
    std::string extractId(const Owned_aggregate &aggr, Command_stack *cmd,
                          const uint8_t *pParentItem, unsigned &itemIdx) const override;

private:
    bool find_key(const char *key, const Owned_aggregate &aggr, const uint8_t *pParentItem,
                  unsigned &itemIdx, unsigned count) const;
};

} // namespace cfg_mgr
