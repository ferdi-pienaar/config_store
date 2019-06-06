///
//

#include <stdint.h> // uint8_t, etc
#include "config_manager_tlv_store.h"
#include "config_manager.h"
#include "config_manager_dbg.h"
#include "nvram.h"
#include <assert.h>

using namespace std;

cm_tlv_store::cm_tlv_store()
{
    tlv = new Tlv(&nvram);

}


cm_tlv_store::~cm_tlv_store()
{
    delete tlv;
}


bool cm_tlv_store::initForRead()
{
    tlv->reset();
    return nvram.initForRead();
}


bool cm_tlv_store::initForWrite()
{
    tlv->reset();
    return nvram.initForWrite();
}


void cm_tlv_store::writeSimple(const cm_simple_metadata * data, const uint8_t * v)
{
    tlv->writeSimple(data->c.id, data->c.len, v);
}


void cm_tlv_store::startWriteComposite(const cm_composite_metadata * data)
{
    tlv->startWriteComposite(data->c.id);
}


void cm_tlv_store::endWriteComposite()
{
    tlv->endWriteComposite();
}

t_cm_result cm_tlv_store::startLoadSimple(const cm_simple_metadata * data)
{
    return tlv->startLoadSimple(data->c.id);
}

t_cm_result cm_tlv_store::endLoadSimple(uint8_t * pRam, const cm_simple_metadata * data)
{
    cm_item_len_t len = data->c.len;
    t_cm_result ret = tlv->endLoadSimple(&len, pRam);

    // Sanity check: the length loaded is the length that was requested.
    // In theory we might support truncation by the persistent storage module, but given that
    // it doesn't know the nature of the data, it can't truncate it sensibly.
    if (ret == CM_SUCCESS) assert(len == data->c.len);

    return ret;
}

t_cm_result cm_tlv_store::startLoadComposite(const cm_composite_metadata * data)
{
    return tlv->startLoadComposite(data->c.id);
}

t_cm_result cm_tlv_store::endLoadComposite()
{
    return tlv->endLoadComposite();
}

