///
//

#include "config_manager_store.h"
#include "config_manager_tlv_store.h"
#include "config_manager_yaml_store.h"

using namespace std;

namespace cfg_mgr
{

enum CM_PERSISTENT_STORE_TYPE
{
    TLV_STORE,
    YAML_STORE
};

Store * Store::getStore()
{
    if (M_DEFINED_STORE_TYPE == TLV_STORE)
        return new Tlv_store;
    else
        return new Yaml_store;
}

}