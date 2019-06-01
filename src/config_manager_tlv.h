#ifndef CFG_MAN_TLV_H
#define CFG_MAN_TLV_H

#include <stdint.h> // uint8_t, etc
#include "nvram.h"
#include "config_manager_types.h"

class Tlv
{
public:
    static const unsigned int stackDepth = 8;

    Tlv(Nvram * pNvram);
    ~Tlv();

    void reset();
    void writeSimple(cm_item_id_t t, cm_item_len_t length, const uint8_t * v);
    void startWriteComposite(cm_item_id_t t);
    void endWriteComposite();
    t_cm_result loadSimple(cm_item_id_t t, cm_item_len_t * length, uint8_t * pRam);
    t_cm_result startLoadComposite(cm_item_id_t t);
    t_cm_result endLoadComposite();

private:
    // Context used in writing
    struct compositeWriteContext
    {
        unsigned      headerOffset; // offset of location of composite's T + L, relative to base of NVRAM
        cm_item_id_t  id;           // composite ID given by client
        cm_item_len_t length;       // actual cumulative length in composite
    };

    struct compositeLoadContext
    {
        cm_item_len_t length;       // length [bytes] in composite, read from NVRAM
        cm_item_len_t readBytes;    // number of composite bytes read from MVRAM
    };

    t_cm_result findType(cm_item_id_t t);
    t_cm_result matchType(cm_item_id_t t);
    void addLengthToComposite(unsigned length);
    Nvram *  nvram;
    int stackIndex;  // write stack index; -1 means the current item is top-level, not part of a composite
    compositeWriteContext writeStack[stackDepth];
    compositeLoadContext  loadStack[stackDepth];
};

#endif // CFG_MAN_TLV_H

