#pragma once

#include <stdint.h> // uint8_t, etc
#include "store/cfg_mgr_store.h"
#include "cfg_mgr_metadata.h"
#include "store/json/cfg_mgr_json_writer.h"
#include "store/json/cfg_mgr_json_loader.h"
#include "store/nvram.h"

namespace cfg_mgr
{

Store * createStorex(Nvram * nvram);

// Access to JSON persistent storage via the abstract interface represented by
// class Store: it's a ConcreteStrategy of the Strategy Store.
// This class implements the Adapter pattern, adapting the interface provided
// by classes JsonWriter and JsonLoader to the needs of the client.
class Json_store : public Store
{
public:
    Json_store(Nvram * nvram);
    ~Json_store();
    const char * getFileName() const
    {
        return "cfg.json";
    }
    bool startWrite() const override;
    void endWrite() const override;
    result_t startLoad() const override;
    result_t endLoad() const override;
    void writeSimple(const Simple_metadata * data, const uint8_t * v) const override;
    void startWriteComposite(const Composite_metadata * data) const override;
    void endWriteComposite() const override;
    void startWriteArray(const char * name) const override;
    void endWriteArray() const override;
    result_t startLoadSimple(const Simple_metadata * data) const override;
    result_t endLoadSimple(uint8_t * pRam, const Simple_metadata * data) const override;
    result_t startLoadComposite(const Composite_metadata * data) const override;
    result_t endLoadComposite() const override;
    result_t startLoadArray(const char * name) const override;
    result_t endLoadArray() const override;

private:
    JsonWriter * m_json_writer;
    JsonLoader * m_json_loader;
};

}
