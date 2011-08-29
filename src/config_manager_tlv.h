#ifndef CFG_MAN_TLV_H
#define CFG_MAN_TLV_H

#include <stdint.h> // uint8_t, etc
#include "nvram.h"
#include "config_manager_types.h"

class Tlv
{
public:
    static const unsigned int stackDepth = 8;

    Tlv();
    ~Tlv();
    bool resetRead()
    {
        reset();
        return nvram->initRead();
    }
    
    bool resetWrite()
    {
        reset();
        return nvram->initWrite();
    }
    
    void writeSimple(cm_item_id_t t, cm_item_len_t length, const uint8_t * v);
    void startWriteComposite(cm_item_id_t t);
    void endWriteComposite();
    t_cm_result getType(cm_item_id_t * t);
    t_cm_result loadSimple(uint8_t * pRam, cm_item_len_t * length, unsigned * complete);
    t_cm_result loadComposite();
    void skipItem(unsigned * complete);
    
private:
    // Context used in writing
    struct compositeContext
    {
        unsigned      lengthOffset; // offset of location of length of composite, relative to base of NVRAM
        cm_item_len_t length;       // actual cumulative length in composite
    };

    void reset()
    {
        stackIndex = -1;
    }

    void addLengthToComposite(unsigned length);
    t_cm_result popLoadStack(cm_item_len_t length, unsigned * complete);
    Nvram *  nvram;
    int stackIndex; // write stack index; -1 means the current item is top-level, not part of a composite
    compositeContext stack[stackDepth];

};

#endif // CFG_MAN_TLV_H

