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


bool cm_tlv_store::resetRead()
{
    tlv->reset();
    return nvram.initRead();
}


bool cm_tlv_store::resetWrite()
{
    tlv->reset();
    return nvram.initWrite();
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


t_cm_result cm_tlv_store::getType(cm_item_id_t * t)
{
    return tlv->getType(t);
}


t_cm_result cm_tlv_store::loadSimple(uint8_t * pRam, const cm_simple_metadata * data, unsigned * complete)
{
    cm_item_len_t len = data->c.len;
    t_cm_result ret = tlv->loadSimple(pRam, &len, complete);

    // Sanity check: the length loaded is the length that was requested.
    // In theory we might support truncation by the persistent storage module, but given that
    // it doesn't know the nature of the data, it can't truncate it sensibly.
    if (ret == CM_SUCCESS) assert(len == data->c.len);

    return ret;
}


t_cm_result cm_tlv_store::loadComposite()
{
    return tlv->loadComposite();
}


t_cm_result cm_tlv_store::skipItem(unsigned * complete)
{
    return tlv->skipItem(complete);
}

