///
// Type-Length-Value format config data, for storage in non-volatile media, e.g. a file.
// This is defined as a separate class hierarchy, since it's an optional feature:
// not all managed objects are saved in m_nvram.
//
// This module asserts that the data passed to it by the client is valid, e.g.
// in writing:
//  - no endWriteComposite without matching startWriteComposite.
//  - no more than stackDepth nested calls to startWriteComposite.
//
// The expected sequence of client calls in writing:
// - writeSimple, or
// - startWriteComposite, then
//   - writeSimple or startWriteComposite
//   - a call to endWriteComposite for each call to startWriteComposite
//

#include <stdint.h> // uint8_t, etc
#include "cfg_mgr_tlv_writer.h"
#include "cfg_mgr_dbg.h"
#include <iostream>
#include "store/nvram.h"
#include <assert.h>

using namespace std;

namespace cfg_mgr
{

TlvWriter::TlvWriter(Nvram * pNvram): m_nvram(pNvram), m_stackIndex(-1)
{
}


TlvWriter::~TlvWriter()
{
}


void TlvWriter::reset()
{
    m_stackIndex = -1;
}


void TlvWriter::writeSimple(item_id_t t, item_len_t len, const uint8_t * v)
{
    m_nvram->write((uint8_t *)&t, sizeof(t));
    m_nvram->write((uint8_t *)&len, sizeof(len));
    m_nvram->write(v, len);

    addLengthToComposite(len);
}


// Don't actually write anything yet, since it may turn out that this
// composite is empty.
void TlvWriter::startWriteComposite(item_id_t t)
{
    assert(m_stackIndex < (int)STACK_DEPTH);

    m_stackIndex++;

    CompositeWriteContext & context = m_writeStack[m_stackIndex];

    context.id = t;
    context.length = 0;
    context.headerOffset = m_nvram->getOffset();

    // reserve space for T + L, to be written by endWriteComposite()
    m_nvram->adjustOffset(HDR_LENGTH);
}


// Close writing of composite by writing header (T and L) of TLV.
// xxx return boolean to indicate if we've reached the bottom of the stack?
void TlvWriter::endWriteComposite()
{
    assert(m_stackIndex >= 0); // we must be inside a composite to end one

    unsigned int endOffset = m_nvram->getOffset(); // current offset, at end of composite
    const CompositeWriteContext & context = m_writeStack[m_stackIndex];

    // switch to context of owning composite (or set to -1 if we're exiting the final composite)
    m_stackIndex--;

#if 1 // Do I really need this? Start composite shouldn't be called for an empty composite.
    if (context.length == 0)
    {
        // Empty composite: write nothing, and set offset to beginning of composite
        assert(endOffset >= HDR_LENGTH);
        endOffset -= HDR_LENGTH;
    }
    else
    {
#endif
        // Non-empty composite: write its header
        m_nvram->setOffset(context.headerOffset);
        m_nvram->write((uint8_t *)&(context.id), sizeof(context.id));
        m_nvram->write((uint8_t *)&(context.length), sizeof(context.length));

        addLengthToComposite(context.length);
#if 1
    }
#endif

    if (m_stackIndex >= 0)
    {
        // We're still inside a composite, so set offset for writing next component
        m_nvram->setOffset(endOffset);
    }
}


// Add component's contribution to the length of the composite it is contained in.
// @param L field of component, excluding length of its T + L, which is added by this method
void TlvWriter::addLengthToComposite(unsigned length)
{
    if (m_stackIndex >= 0)
    {
        // Current item is member of a composite, so add component's contribution to its length
        m_writeStack[m_stackIndex].length += length + HDR_LENGTH;
    }
}
}
