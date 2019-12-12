///
//

#include <stdint.h> // uint8_t, etc
#include "config_manager_tlv_store.h"
#include "config_manager.h"
#include "config_manager_dbg.h"
#include "nvram.h"
#include <assert.h>

using namespace std;

namespace cfg_mgr
{

Tlv_store::Tlv_store()
{
    m_tlv = new Tlv(&m_nvram);

}


Tlv_store::~Tlv_store()
{
    delete m_tlv;
}


bool Tlv_store::initForRead()
{
    m_tlv->reset();
    return m_nvram.initForRead();
}


bool Tlv_store::initForWrite()
{
    m_tlv->reset();
    return m_nvram.initForWrite();
}


void Tlv_store::writeSimple(const Simple_metadata * data, const uint8_t * v)
{
    m_tlv->writeSimple(data->c.id, data->c.len, v);
}


void Tlv_store::startWriteComposite(const Composite_metadata * data)
{
    m_tlv->startWriteComposite(data->c.id);
}


void Tlv_store::endWriteComposite()
{
    m_tlv->endWriteComposite();
}

result_t Tlv_store::startLoadSimple(const Simple_metadata * data)
{
    return m_tlv->startLoadSimple(data->c.id);
}

result_t Tlv_store::endLoadSimple(uint8_t * pRam, const Simple_metadata * data)
{
    item_len_t len = data->c.len;
    result_t ret = m_tlv->endLoadSimple(&len, pRam);

    // Sanity check: the length loaded is the length that was requested.
    // In theory we might support truncation by the persistent storage module, but given that
    // it doesn't know the nature of the data, it can't truncate it sensibly.
    if (ret == CM_SUCCESS) assert(len == data->c.len);

    return ret;
}

result_t Tlv_store::startLoadComposite(const Composite_metadata * data)
{
    return m_tlv->startLoadComposite(data->c.id);
}

result_t Tlv_store::endLoadComposite()
{
    return m_tlv->endLoadComposite();
}
}