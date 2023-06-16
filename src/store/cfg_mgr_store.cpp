///
//

#include "store/cfg_mgr_store.h"
#include "store/tlv/cfg_mgr_tlv_store.h"
#include "store/json/cfg_mgr_json_store.h"

using namespace std;

namespace cfg_mgr
{

enum CM_PERSISTENT_STORE_TYPE
{
    TLV_STORE,
    JSON_STORE
};

Store * Store::createStore(Nvram * nvram)
{
    if (M_DEFINED_STORE_TYPE == TLV_STORE)
        return new Tlv_store(nvram);
    else
        return new Json_store(nvram);
}

result_t Store::startLoad()
{
    return m_nvram->initForRead() ? CM_SUCCESS : CM_FAIL;
}

result_t Store::endLoad()
{
    m_nvram->accessComplete();
    return CM_SUCCESS;
}

bool Store::startWrite()
{
    return m_nvram->initForWrite();
}

void Store::endWrite()
{
    m_nvram->accessComplete();
}

}
