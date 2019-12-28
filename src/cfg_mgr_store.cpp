///
//

#include "cfg_mgr_store.h"
#include "cfg_mgr_tlv_store.h"
#include "cfg_mgr_json_store.h"

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

bool Store::startLoad()
{
    return m_nvram->initForRead();
}

void Store::endLoad()
{
    m_nvram->accessComplete();
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
