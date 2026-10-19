// Metadata that is common to Simple_descriptor and Composite_descriptor.
#pragma once

#include "cfg_mgr_types.h"

namespace cfg_mgr
{

constexpr bool PERSISTENT = true;
constexpr bool NOT_PERSISTENT = false;

struct Common_metadata
{
    const char * const name; ///< name by which item is addressed on CLI
    bool is_persistent;
    const item_len_t len; ///< Number of bytes occupied by an item in RAM

    bool isPersistent() const { return is_persistent; }
};

}
