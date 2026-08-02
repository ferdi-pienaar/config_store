///
//

#include <stdint.h> // uint8_t, etc
#include "cfg_mgr_json_store.h"
#include "cfg_mgr_implement.h"
#include "cfg_mgr_dbg.h"
#include "store/nvram.h"
#include <assert.h>

using namespace std;

namespace cfg_mgr
{

Store * createStorex(Nvram_itf * nvram)
{
    return new Json_store(nvram);
}

Json_store::Json_store(Nvram_itf * nvram) : Store(nvram)
{
    m_json_writer = new JsonWriter(m_nvram);
    m_json_loader = new JsonLoader(m_nvram);
}


Json_store::~Json_store()
{
    delete m_json_writer;
    delete m_json_loader;
}


bool Json_store::startWrite() const
{
    if (!Store::startWrite())
    {
        return false;
    }
    m_json_writer->startWrite();
    return true;
}


void Json_store::endWrite() const
{
    m_json_writer->endWrite();
    Store::endWrite();
}


Result Json_store::startLoad() const
{
    Result ret = Store::startLoad();
    if (ret != Result::CM_SUCCESS)
    {
        return ret;
    }
    return m_json_loader->startLoad();
}


Result Json_store::endLoad() const
{
    Result ret = m_json_loader->endLoad();
    if (ret != Result::CM_SUCCESS)
    {
        return ret;
    }
    return Store::endLoad();
}

void Json_store::writeSimple(const Simple_metadata * data, const uint8_t * v) const
{
    assert(data->pPrt != nullptr);
    m_json_writer->writeSimple(data->c.name, data->c.len, v, data->pPrt);
}


void Json_store::startWriteComposite(const Composite_metadata * data) const
{
    m_json_writer->startWriteObject(data->c.name);
}


void Json_store::endWriteComposite() const
{
    m_json_writer->endWriteObject();
}

void Json_store::startWriteArray(const char * name) const
{
    m_json_writer->startWriteArray(name);
}

void Json_store::endWriteArray() const
{
    m_json_writer->endWriteArray();
}

Result Json_store::startLoadSimple(const Simple_metadata * data) const
{
    return m_json_loader->startLoadSimple(data->c.name);
}

Result Json_store::endLoadSimple(uint8_t * pRam, const Simple_metadata * data) const
{
    assert(data->pSet != nullptr);
    item_len_t len = data->c.len;
    return m_json_loader->endLoadSimple(&len, pRam, data->pSet);
}

Result Json_store::startLoadComposite(const Composite_metadata * data) const
{
    return m_json_loader->startLoadObject(data->c.name);
}

Result Json_store::endLoadComposite() const
{
    return m_json_loader->endLoadObject();
}

Result Json_store::startLoadArray(const char * name) const
{
    return m_json_loader->startLoadArray(name);
}

Result Json_store::endLoadArray() const
{
    return m_json_loader->endLoadArray();
}

}
