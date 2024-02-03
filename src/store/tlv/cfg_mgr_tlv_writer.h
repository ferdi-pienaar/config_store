#pragma once

#include <stdint.h> // uint8_t, etc
#include "store/nvram.h"
#include "cfg_mgr_types.h"

namespace cfg_mgr
{

class TlvWriter
{
public:
    static const unsigned int STACK_DEPTH = 8;

    TlvWriter(Nvram * pNvram);
    ~TlvWriter();

    void reset();
    void writeSimple(item_id_t t, item_len_t length, const uint8_t * v);
    void startWriteComposite(item_id_t t);
    void endWriteComposite();

private:
    static const unsigned HDR_LENGTH = sizeof(item_id_t) + sizeof(item_len_t);
    // Context used in writing
    struct CompositeWriteContext
    {
        unsigned   headerOffset; // offset of location of composite's T + L, relative to base of NVRAM
        item_id_t  id;           // composite ID given by client
        item_len_t length;       // actual cumulative length in composite
    };

    void addLengthToComposite(unsigned length);

    Nvram * m_nvram;
    int m_stackIndex;  // write/load stack index; -1 means the current item is top-level, not part of a composite
    CompositeWriteContext m_writeStack[STACK_DEPTH];
};
}
