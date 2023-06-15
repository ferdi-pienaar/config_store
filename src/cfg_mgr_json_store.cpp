///
//

#include <stdint.h> // uint8_t, etc
#include "cfg_mgr_json_store.h"
#include "cfg_mgr_implement.h"
#include "cfg_mgr_dbg.h"
#include "nvram.h"
#include <assert.h>

using namespace std;

namespace cfg_mgr
{

Json_store::Json_store(Nvram * nvram) : Store(nvram)
{
    m_json_writer = new JsonWriter(m_nvram);
    m_json_loader = new JsonLoader(m_nvram);
}


Json_store::~Json_store()
{
    delete m_json_writer;
    delete m_json_loader;
}


bool Json_store::startWrite()
{
    if (!Store::startWrite())
    {
        return false;
    }
    m_json_writer->startWrite();
    return true;
}


void Json_store::endWrite()
{
    m_json_writer->endWrite();
    Store::endWrite();
}


result_t Json_store::startLoad()
{
    result_t ret = Store::startLoad();
    if (ret != CM_SUCCESS)
    {
        return ret;
    }
    return m_json_loader->startLoad();
}


result_t Json_store::endLoad()
{
    result_t ret = m_json_loader->endLoad();
    if (ret != CM_SUCCESS)
    {
        return ret;
    }
    return Store::endLoad();
}

void Json_store::writeSimple(const Simple_metadata * data, const uint8_t * v)
{
    m_json_writer->writeSimple(data->c.name, data->c.len, v, data->pPrt);
}


void Json_store::startWriteComposite(const Composite_metadata * data)
{
    m_json_writer->startWriteObject(data->c.name);
}


void Json_store::endWriteComposite()
{
    m_json_writer->endWriteObject();
}

void Json_store::startWriteArray(const char * name)
{
    m_json_writer->startWriteArray(name);
}

void Json_store::endWriteArray()
{
    m_json_writer->endWriteArray();
}

result_t Json_store::startLoadSimple(const Simple_metadata * data)
{
    return m_json_loader->startLoadSimple(data->c.name);
}

result_t Json_store::endLoadSimple(uint8_t * pRam, const Simple_metadata * data)
{
    item_len_t len = data->c.len;
    return m_json_loader->endLoadSimple(&len, pRam, data->pSet);
}

result_t Json_store::startLoadComposite(const Composite_metadata * data)
{
    return m_json_loader->startLoadObject(data->c.name);
}

result_t Json_store::endLoadComposite()
{
    return m_json_loader->endLoadObject();
}

result_t Json_store::startLoadArray(const char * name)
{
    return m_json_loader->startLoadArray(name);
}

result_t Json_store::endLoadArray()
{
    return m_json_loader->endLoadArray();
}

}
