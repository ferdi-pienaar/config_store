#pragma once

#include <stdint.h> // uint8_t, etc
#include "store/cfg_mgr_store.h"
#include "cfg_mgr_metadata.h"
#include "store/tlv/cfg_mgr_tlv_writer.h"
#include "store/tlv/cfg_mgr_tlv_loader.h"
#include "store/nvram.h"
#include <memory>

namespace cfg_mgr
{

std::unique_ptr<Store> createStoreSpecific(Nvram_itf * nvram);

// Access to TLV persistent storage via the abstract interface represented by Store:
// it's a ConcreteStrategy of the Strategy Store.
// This class implements the adapter pattern, adapting the interface provided
// by classes TlvWriter and TlvLoader to the needs of the client.
class Tlv_store : public Store
{
public:
    Tlv_store(Nvram_itf * nvram);
    ~Tlv_store() = default;
    Result startLoad() const override;
    bool startWrite() const override;
    void writeSimple(const Simple_metadata * data, const uint8_t * v) const override;
    void startWriteComposite(const Composite_metadata * data) const override;
    void endWriteComposite() const override;
    Result startLoadSimple(const Simple_metadata * data) const override;
    Result endLoadSimple(uint8_t * pRam, const Simple_metadata * data) const override;
    Result startLoadComposite(const Composite_metadata * data) const override;
    Result endLoadComposite() const override;

private:
    std::unique_ptr<TlvWriter> m_tlv_writer;
    std::unique_ptr<TlvLoader> m_tlv_loader;

};
}
