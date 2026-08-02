#pragma once

#include <stdint.h> // uint8_t, etc
#include "store/nvram.h"
#include "cfg_mgr_types.h"

namespace cfg_mgr
{

class TlvLoader
{
public:
    static const unsigned int STACK_DEPTH = 8;

    TlvLoader(Nvram_itf * pNvram);
    ~TlvLoader();

    void reset();
    Result startLoadSimple(item_id_t t);
    Result endLoadSimple(item_len_t * length, uint8_t * pRam);
    Result startLoadComposite(item_id_t t);
    Result endLoadComposite();

private:
    static const unsigned HDR_LENGTH = sizeof(item_id_t) + sizeof(item_len_t);
    // Context used in loading
    struct CompositeLoadContext
    {
        item_len_t valueOffset;  // offset of location of composite's V (i.e. first component), relative to base of NVRAM
        item_len_t length;       // length [bytes] in composite, read from NVRAM
    };

    Result findType(item_id_t t);
    Result findTypeInComposite(item_id_t t);
    Result matchType(item_id_t t);

    Nvram_itf * m_nvram;
    int m_stackIndex;  // write/load stack index; -1 means the current item is top-level, not part of a composite
    CompositeLoadContext  m_loadStack[STACK_DEPTH];
};
}
