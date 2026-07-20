///
//

#include "store/cfg_mgr_store.h"
#include "store/tlv/cfg_mgr_tlv_store.h"
#include "store/json/cfg_mgr_json_store.h"

using namespace std;

namespace cfg_mgr
{

Store * Store::createStore(Nvram_itf * nvram)
{
    return createStorex(nvram);
}

result_t Store::startLoad() const
{
    return m_nvram->initForRead() ? CM_SUCCESS : CM_FAIL;
}

result_t Store::endLoad() const
{
    m_nvram->accessComplete();
    return CM_SUCCESS;
}

bool Store::startWrite() const
{
    return m_nvram->initForWrite();
}

void Store::endWrite() const
{
    m_nvram->accessComplete();
}

}
