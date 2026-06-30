#pragma once

#include <stdint.h> // uint8_t, etc
#include "store/cfg_mgr_store.h"
#include "cfg_mgr_metadata.h"
#include "store/tlv/cfg_mgr_tlv_writer.h"
#include "store/tlv/cfg_mgr_tlv_loader.h"
#include "store/nvram.h"

namespace cfg_mgr
{

Store * createStorex(Nvram_itf * nvram);

// Access to TLV persistent storage via the abstract interface represented by Store:
// it's a ConcreteStrategy of the Strategy Store.
// This class implements the adapter pattern, adapting the interface provided
// by classes TlvWriter and TlvLoader to the needs of the client.
class Tlv_store : public Store
{
public:
    Tlv_store(Nvram_itf * nvram);
    ~Tlv_store();
    const char * getFileName() const
    {
        return "cfg.bin";
    }
    result_t startLoad() const override;
    bool startWrite() const override;
    void writeSimple(const Simple_metadata * data, const uint8_t * v) const override;
    void startWriteComposite(const Composite_metadata * data) const override;
    void endWriteComposite() const override;
    result_t startLoadSimple(const Simple_metadata * data) const override;
    result_t endLoadSimple(uint8_t * pRam, const Simple_metadata * data) const override;
    result_t startLoadComposite(const Composite_metadata * data) const override;
    result_t endLoadComposite() const override;

private:
    TlvWriter * m_tlv_writer;
    TlvLoader * m_tlv_loader;

};
}
