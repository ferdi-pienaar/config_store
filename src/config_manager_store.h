#ifndef CFG_MAN_STORE_H
#define CFG_MAN_STORE_H

#include <stdint.h> // uint8_t, etc
#include "config_manager_metadata.h"
#include "nvram.h"


// Abstract interface to classes that give access to persistent storage
class cm_store
{
public:
    virtual ~cm_store() {}
    virtual bool resetRead() = 0;
    virtual bool resetWrite() = 0;
    virtual void writeSimple(const cm_simple_metadata * data, const uint8_t * v) = 0;
    virtual void startWriteComposite(const cm_composite_metadata * data) = 0;
    virtual void endWriteComposite() = 0;
    virtual t_cm_result getType(cm_item_id_t * t) = 0;
    virtual t_cm_result loadSimple(uint8_t * pRam, const cm_simple_metadata * data, unsigned * complete) = 0;
    virtual t_cm_result loadComposite() = 0;
    virtual void skipItem(unsigned * complete) = 0;
    static cm_store * getStore();

protected:
    Nvram  nvram;

};

#endif // CFG_MAN_STORE_H

