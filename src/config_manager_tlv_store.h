#ifndef CFG_MAN_TLV_STORE_H
#define CFG_MAN_TLV_STORE_H

#include <stdint.h> // uint8_t, etc
#include "config_manager_store.h"
#include "config_manager_metadata.h"
#include "config_manager_tlv.h"
#include "nvram.h"


// Access to TLV persistent storage via the abstract interface represented by cm_store:
// it's a ConcreteStrategy of the Strategy cm_store.
// This class implements the adapter pattern, adapting the interface provided
// by the TLV class to the needs of the client.
class cm_tlv_store : public cm_store
{
public:
    cm_tlv_store();
    ~cm_tlv_store();

    bool initForRead();
    bool initForWrite();
    void writeSimple(const cm_simple_metadata * data, const uint8_t * v);
    void startWriteComposite(const cm_composite_metadata * data);
    void endWriteComposite();
    t_cm_result loadSimple(uint8_t * pRam, const cm_common_metadata * data);
    t_cm_result startLoadComposite(const cm_common_metadata * data);
    t_cm_result endLoadComposite();

private:
    Tlv * tlv;

};


#endif // CFG_MAN_TLV_STORE_H

