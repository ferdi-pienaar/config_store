///
//

#include <stdint.h> // uint8_t, etc
#include "cfg_mgr_tlv_store.h"
#include "cfg_mgr_implement.h"
#include "cfg_mgr_dbg.h"
#include "store/nvram.h"
#include <assert.h>

using namespace std;

namespace cfg_mgr
{

unique_ptr<Store> createStoreSpecific(Nvram_itf * nvram)
{
    return make_unique<Tlv_store>(nvram);
}

Tlv_store::Tlv_store(Nvram_itf * nvram) : Store(nvram)
{
    m_tlv_writer = make_unique<TlvWriter>(m_nvram);
    m_tlv_loader = make_unique<TlvLoader>(m_nvram);
}

Result Tlv_store::startLoad() const
{
    m_tlv_loader->reset();
    return Store::startLoad();
}


bool Tlv_store::startWrite() const
{
    m_tlv_writer->reset();
    return Store::startWrite();
}


void Tlv_store::writeSimple(const Simple_metadata * data, const uint8_t * v) const
{
    assert(data->c.persistent_id != NOT_PERSISTENT);
    m_tlv_writer->writeSimple(data->c.persistent_id, data->c.len, v);
}


void Tlv_store::startWriteComposite(const Composite_metadata * data) const
{
    assert(data->c.persistent_id != NOT_PERSISTENT);
    m_tlv_writer->startWriteComposite(data->c.persistent_id);
}


void Tlv_store::endWriteComposite() const
{
    m_tlv_writer->endWriteComposite();
}

Result Tlv_store::startLoadSimple(const Simple_metadata * data) const
{
    return m_tlv_loader->startLoadSimple(data->c.persistent_id);
}

Result Tlv_store::endLoadSimple(uint8_t * pRam, const Simple_metadata * data) const
{
    item_len_t len = data->c.len;
    Result ret = m_tlv_loader->endLoadSimple(&len, pRam);

    // Sanity check: the length loaded is the length that was requested.
    // In theory we might support truncation by the persistent storage module, but given that
    // it doesn't know the nature of the data, it can't truncate it sensibly.
    if (ret == Result::CM_SUCCESS) assert(len == data->c.len);

    return ret;
}

Result Tlv_store::startLoadComposite(const Composite_metadata * data) const
{
    return m_tlv_loader->startLoadComposite(data->c.persistent_id);
}

Result Tlv_store::endLoadComposite() const
{
    return m_tlv_loader->endLoadComposite();
}
}
