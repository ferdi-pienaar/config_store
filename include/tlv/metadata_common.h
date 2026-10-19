// Metadata that is common to Simple_descriptor and Composite_descriptor.
#pragma once

#include "cfg_mgr_types.h"

namespace cfg_mgr
{

constexpr item_id_t NOT_PERSISTENT = (item_id_t) -1;

struct Common_metadata
{
    const char * const name; ///< name by which item is addressed on CLI
    const item_id_t persistent_id; ///< ID (unique in the context of the component's composite) of item in NVRAM or NOT_PERSISTENT
    const item_len_t len; ///< Number of bytes occupied by an item in RAM

    bool isPersistent() const { return persistent_id != NOT_PERSISTENT; }
};

}
