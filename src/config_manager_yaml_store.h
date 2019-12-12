#ifndef CFG_MAN_YAML_STORE_H
#define CFG_MAN_YAML_STORE_H

#include <stdint.h> // uint8_t, etc
#include "config_manager_store.h"
#include "config_manager_metadata.h"
#include "config_manager_yaml.h"
#include "nvram.h"

namespace cfg_mgr
{

// Access to YAML persistent storage via the abstract interface represented by Store:
// it's a ConcreteStrategy of the Strategy Store.
// This class implements the adapter pattern, adapting the interface provided
// by the YAML class to the needs of the client.
class Yaml_store : public Store
{
public:
    Yaml_store();
    ~Yaml_store();

    bool initForRead();
    bool initForWrite();
    void writeSimple(const Simple_metadata * data, const uint8_t * v);
    void startWriteComposite(const Composite_metadata * data);
    void endWriteComposite();
    result_t startLoadSimple(const Simple_metadata * data);
    result_t endLoadSimple(uint8_t * pRam, const Simple_metadata * data);
    result_t startLoadComposite(const Composite_metadata * data);
    result_t endLoadComposite();

private:
    Yaml * m_yaml;

};
}
#endif // CFG_MAN_YAML_STORE_H

