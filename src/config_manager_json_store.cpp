///
//

#include <stdint.h> // uint8_t, etc
#include "config_manager_json_store.h"
#include "config_manager.h"
#include "config_manager_dbg.h"
#include "nvram.h"
#include <assert.h>

using namespace std;

namespace cfg_mgr
{

Json_store::Json_store(Nvram * nvram) : Store(nvram)
{
    cout << "Json_store::Json_store()" << endl;

    m_json = new Json(m_nvram);
}


Json_store::~Json_store()
{
    cout << "Json_store::~Json_store()" << endl;

    delete m_json;
}


bool Json_store::startWrite()
{
    cout << __PRETTY_FUNCTION__ << endl;

    if (!Store::startWrite())
    {
        return false;
    }
    m_json->startWrite();
    return true;
}


void Json_store::endWrite()
{
    m_json->endWrite();
    Store::endWrite();
}


bool Json_store::startLoad()
{
    cout << __PRETTY_FUNCTION__ << endl;

    return Store::startLoad();
}


void Json_store::endLoad()
{
    Store::endLoad();
}

void Json_store::writeSimple(const Simple_metadata * data, const uint8_t * v)
{
    m_json->writeSimple(data->c.name, data->c.len, v, data->pPrt);
}


void Json_store::startWriteComposite(const Composite_metadata * data)
{
    m_json->startWriteComposite(data->c.name);
}


void Json_store::endWriteComposite()
{
    m_json->endWriteComposite();
}

void Json_store::startWriteArray(const char * name)
{
    m_json->startWriteArray(name);
}

void Json_store::endWriteArray()
{
    m_json->endWriteArray();
}

result_t Json_store::startLoadSimple(const Simple_metadata * data)
{
    cout << __PRETTY_FUNCTION__ << endl;
    return m_json->startLoadSimple(data->c.name);
}

result_t Json_store::endLoadSimple(uint8_t * pRam, const Simple_metadata * data)
{
    cout << __PRETTY_FUNCTION__ << endl;
    item_len_t len = data->c.len;
    return m_json->endLoadSimple(&len, pRam, data->pSet);
}

// xxx incomplete
result_t Json_store::startLoadComposite(const Composite_metadata * data)
{
    cout << "Json_store::startLoadComposite()" << endl;

    return m_json->startLoadComposite(data->c.name);
}

result_t Json_store::endLoadComposite()
{
    cout << "Json_store::endLoadComposite()" << endl;
    return m_json->endLoadComposite();
}

}
