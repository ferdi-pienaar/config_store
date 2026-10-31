#include "item_identifier.h"
#include "item_identifier_index.h"
#include "item_identifier_key.h"

using namespace std;

namespace cfg_mgr
{

unique_ptr<Item_identifier> Item_identifier::make_Identifier(bool use_key)
{
    if (use_key)
    {
        // The client uses a key identifier, so return the corresponding strategy object.
        return make_unique<Item_identifier_key>();
    }
    return make_unique<Item_identifier_index>();
}

}
