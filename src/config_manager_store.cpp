/// 
//

#include "config_manager_store.h"
#include "config_manager_tlv_store.h"
#include "config_manager_yaml_store.h"

using namespace std;

enum CM_PERSISTENT_STORE_TYPE
{
    TLV_STORE,
    YAML_STORE
};

cm_store * cm_store::getStore()
{
    if (M_DEFINED_STORE_TYPE == TLV_STORE)
        return new cm_tlv_store;
    else
        return new cm_yaml_store;
}

