#ifndef CFG_MGR_JSON_STORE_H
#define CFG_MGR_JSON_STORE_H

#include <stdint.h> // uint8_t, etc
#include "cfg_mgr_store.h"
#include "cfg_mgr_metadata.h"
#include "cfg_mgr_json_writer.h"
#include "cfg_mgr_json_loader.h"
#include "nvram.h"

namespace cfg_mgr
{

// Access to JSON persistent storage via the abstract interface represented by
// class Store: it's a ConcreteStrategy of the Strategy Store.
// This class implements the Adapter pattern, adapting the interface provided
// by class Json to the needs of the client.
class Json_store : public Store
{
public:
    Json_store(Nvram * nvram);
    ~Json_store();
    bool startWrite();
    void endWrite();
    result_t startLoad();
    result_t endLoad();
    void writeSimple(const Simple_metadata * data, const uint8_t * v);
    void startWriteComposite(const Composite_metadata * data);
    void endWriteComposite();
    void startWriteArray(const char * name);
    void endWriteArray();
    result_t startLoadSimple(const Simple_metadata * data);
    result_t endLoadSimple(uint8_t * pRam, const Simple_metadata * data);
    result_t startLoadComposite(const Composite_metadata * data);
    result_t endLoadComposite();
    result_t startLoadArray(const char * name);
    result_t endLoadArray();

private:
    JsonWriter * m_json_writer;
    JsonLoader * m_json_loader;
};

}
#endif // CFG_MGR_JSON_STORE_H

