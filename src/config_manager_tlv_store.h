#ifndef CFG_MAN_TLV_STORE_H
#define CFG_MAN_TLV_STORE_H

#include <stdint.h> // uint8_t, etc
#include "config_manager_store.h"
#include "config_manager_metadata.h"
#include "config_manager_tlv.h"
#include "nvram.h"

namespace cfg_mgr
{
// Access to TLV persistent storage via the abstract interface represented by Store:
// it's a ConcreteStrategy of the Strategy Store.
// This class implements the adapter pattern, adapting the interface provided
// by the TLV class to the needs of the client.
class Tlv_store : public Store
{
public:
    Tlv_store(Nvram * nvram);
    ~Tlv_store();

    bool startLoad();
    bool startWrite();
    void writeSimple(const Simple_metadata * data, const uint8_t * v);
    void startWriteComposite(const Composite_metadata * data);
    void endWriteComposite();
    result_t startLoadSimple(const Simple_metadata * data);
    result_t endLoadSimple(uint8_t * pRam, const Simple_metadata * data);
    result_t startLoadComposite(const Composite_metadata * data);
    result_t endLoadComposite();

private:
    Tlv * m_tlv;

};
}

#endif // CFG_MAN_TLV_STORE_H

