///
//

#include "config_manager_store.h"
#include "config_manager_tlv_store.h"
#include "config_manager_json_store.h"

using namespace std;

namespace cfg_mgr
{

enum CM_PERSISTENT_STORE_TYPE
{
    TLV_STORE,
    YAML_STORE
};

Store * Store::createStore(Nvram * nvram)
{
    if (M_DEFINED_STORE_TYPE == TLV_STORE)
        return new Tlv_store(nvram);
    else
        return new Json_store(nvram);
}

}
