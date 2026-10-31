#include "item_identifier.h"
#include "item_identifier_index.h"
#include <cassert>

using namespace std;

namespace cfg_mgr
{

unique_ptr<Item_identifier> Item_identifier::make_Identifier(bool use_key)
{
    if (use_key)
    {
        // The client uses a key identifier, but this build doesn't support that.
        assert(false && "This build does not support key identifiers: build with KEY_ENABLE.");
    }
    return make_unique<Item_identifier_index>();
}

}
