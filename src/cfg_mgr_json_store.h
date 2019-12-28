#ifndef CFG_MGR_JSON_STORE_H
#define CFG_MGR_JSON_STORE_H

#include <stdint.h> // uint8_t, etc
#include "cfg_mgr_store.h"
#include "cfg_mgr_metadata.h"
#include "cfg_mgr_json.h"
#include "nvram.h"

namespace cfg_mgr
{

// Access to YAML persistent storage via the abstract interface represented by cm_store:
// it's a ConcreteStrategy of the Strategy cm_store.
// This class implements the adapter pattern, adapting the interface provided
// by the YAML class to the needs of the client.
class Json_store : public Store
{
public:
    Json_store(Nvram * nvram);
    ~Json_store();
    bool startWrite();
    void endWrite();
    bool startLoad();
    void endLoad();
    void writeSimple(const Simple_metadata * data, const uint8_t * v);
    void startWriteComposite(const Composite_metadata * data);
    void endWriteComposite();
    void startWriteArray(const char * name);
    void endWriteArray();
    result_t startLoadSimple(const Simple_metadata * data);
    result_t endLoadSimple(uint8_t * pRam, const Simple_metadata * data);
    result_t startLoadComposite(const Composite_metadata * data);
    result_t endLoadComposite();

private:
    Json * m_json;

};

}
#endif // CFG_MGR_JSON_STORE_H

