#ifndef CFG_MGR_TLV_H
#define CFG_MGR_TLV_H

#include <stdint.h> // uint8_t, etc
#include "nvram.h"
#include "cfg_mgr_types.h"

namespace cfg_mgr
{

class Tlv
{
public:
    static const unsigned int STACK_DEPTH = 8;

    Tlv(Nvram * pNvram);
    ~Tlv();

    void reset();
    void writeSimple(item_id_t t, item_len_t length, const uint8_t * v);
    void startWriteComposite(item_id_t t);
    void endWriteComposite();
    result_t startLoadSimple(item_id_t t);
    result_t endLoadSimple(item_len_t * length, uint8_t * pRam);
    result_t startLoadComposite(item_id_t t);
    result_t endLoadComposite();

private:
    static const unsigned HDR_LENGTH = sizeof(item_id_t) + sizeof(item_len_t);
    // Context used in writing
    struct CompositeWriteContext
    {
        unsigned   headerOffset; // offset of location of composite's T + L, relative to base of NVRAM
        item_id_t  id;           // composite ID given by client
        item_len_t length;       // actual cumulative length in composite
    };

    // Context used in loading
    struct CompositeLoadContext
    {
        item_len_t valueOffset;  // offset of location of composite's V (i.e. first component), relative to base of NVRAM
        item_len_t length;       // length [bytes] in composite, read from NVRAM
    };

    result_t findType(item_id_t t);
    result_t findTypeInComposite(item_id_t t);
    result_t matchType(item_id_t t);
    void addLengthToComposite(unsigned length);

    Nvram * m_nvram;
    int m_stackIndex;  // write/load stack index; -1 means the current item is top-level, not part of a composite
    CompositeWriteContext m_writeStack[STACK_DEPTH];
    CompositeLoadContext  m_loadStack[STACK_DEPTH];
};
}
#endif // CFG_MGR_TLV_H

