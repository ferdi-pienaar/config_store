#ifndef CFG_MAN_YAML_STORE_H
#define CFG_MAN_YAML_STORE_H

#include <stdint.h> // uint8_t, etc
#include "config_manager_store.h"
#include "config_manager_metadata.h"
#include "config_manager_yaml.h"
#include "nvram.h"


// Access to YAML persistent storage via the abstract interface represented by cm_store:
// it's a ConcreteStrategy of the Strategy cm_store.
// This class implements the adapter pattern, adapting the interface provided
// by the YAML class to the needs of the client.
class cm_yaml_store : public cm_store
{
public:
    cm_yaml_store();
    ~cm_yaml_store();

    bool initForRead();
    bool initForWrite();
    void writeSimple(const cm_simple_metadata * data, const uint8_t * v);
    void startWriteComposite(const cm_composite_metadata * data);
    void endWriteComposite();
    t_cm_result getType(cm_item_id_t * t);
    t_cm_result loadSimple(uint8_t * pRam, const cm_simple_metadata * data, unsigned * complete);
    t_cm_result loadComposite();
    t_cm_result skipItem(unsigned * complete);

private:
    Yaml * yaml;

};

#endif // CFG_MAN_YAML_STORE_H

