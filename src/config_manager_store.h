#ifndef CFG_MAN_STORE_H
#define CFG_MAN_STORE_H

#include <stdint.h> // uint8_t, etc
#include "config_manager_tlv.h"
#include "config_manager_metadata.h"
#include "nvram.h"


// xxx move this to separate file
class cm_store
{
public:
    cm_store();
    ~cm_store();

    bool resetRead();
    bool resetWrite();
    void writeSimple(const cm_simple_metadata * data, const uint8_t * v);
    void startWriteComposite(const cm_composite_metadata * data);
    void endWriteComposite();
    t_cm_result getType(cm_item_id_t * t);
    t_cm_result loadSimple(uint8_t * pRam, const cm_simple_metadata * data, unsigned * complete);
    t_cm_result loadComposite();
    void skipItem(unsigned * complete);

private:
    Tlv *  tlv;
    Nvram  nvram;

};

#endif // CFG_MAN_STORE_H

