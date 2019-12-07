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
    tlv = new Tlv(&nvram);

}


Tlv_store::~Tlv_store()
{
    delete tlv;
}


bool Tlv_store::initForRead()
{
    tlv->reset();
    return nvram.initForRead();
}


bool Tlv_store::initForWrite()
{
    tlv->reset();
    return nvram.initForWrite();
}


void Tlv_store::writeSimple(const Simple_metadata * data, const uint8_t * v)
{
    tlv->writeSimple(data->c.id, data->c.len, v);
}


void Tlv_store::startWriteComposite(const Composite_metadata * data)
{
    tlv->startWriteComposite(data->c.id);
}


void Tlv_store::endWriteComposite()
{
    tlv->endWriteComposite();
}

result_t Tlv_store::startLoadSimple(const Simple_metadata * data)
{
    return tlv->startLoadSimple(data->c.id);
}

result_t Tlv_store::endLoadSimple(uint8_t * pRam, const Simple_metadata * data)
{
    item_len_t len = data->c.len;
    result_t ret = tlv->endLoadSimple(&len, pRam);

    // Sanity check: the length loaded is the length that was requested.
    // In theory we might support truncation by the persistent storage module, but given that
    // it doesn't know the nature of the data, it can't truncate it sensibly.
    if (ret == CM_SUCCESS) assert(len == data->c.len);

    return ret;
}

result_t Tlv_store::startLoadComposite(const Composite_metadata * data)
{
    return tlv->startLoadComposite(data->c.id);
}

result_t Tlv_store::endLoadComposite()
{
    return tlv->endLoadComposite();
}
}