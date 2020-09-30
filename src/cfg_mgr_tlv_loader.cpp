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
// The expected sequence of client calls in loading:
// 1. Load Simple:
//  1.1 startLoadSimple
//  1.2 endLoadSimple if startLoadSimple was OK.
// OR
// 2. Load Composite:
//  2.1 startLoadComposite, then the next steps follow iff that returns OK.
//  2.2 Load components, either Simple or themselves Composite
//  2.3 endLoadComposite.
//
//

#include <stdint.h> // uint8_t, etc
#include "cfg_mgr_tlv_loader.h"
#include "cfg_mgr_dbg.h"
#include <iostream>
#include "nvram.h"
#include <assert.h>

using namespace std;

namespace cfg_mgr
{

TlvLoader::TlvLoader(Nvram * pNvram): m_nvram(pNvram), m_stackIndex(-1)
{
}


TlvLoader::~TlvLoader()
{
}


void TlvLoader::reset()
{
    m_stackIndex = -1;
}


// Load a simple item into the provided memory.
//
// @param t: type to load.
result_t TlvLoader::startLoadSimple(item_id_t t)
{
    return findType(t);
}

// Load a simple item into the provided memory.
//
// @param pLength in/out, in: available memory, out: amount of data written to pRam
// @param pRam
//
// @note: if the length is unexpected, we could skip just that item, but it's
//        simpler to just return an error, presumably forcing the client to abandon
//        the load process completely.
result_t TlvLoader::endLoadSimple(item_len_t * pLength, uint8_t * pRam)
{
    item_len_t length;
    m_nvram->read((uint8_t *)&length, sizeof(item_len_t));

    if (*pLength != length)
    {
        *pLength = 0; // No data loaded into client RAM
        return CM_INCOHERENT_DATA;
    }

    // DBG_PRT("%s: %d at %p\n", __PRETTY_FUNCTION__, length, pRam);

    if (!m_nvram->read(pRam, length))
    {
        // This error aborts the loading process, and there's no need to updateContainer xxx obsolete comment.
        return CM_READ_FAIL;
    }

    // Data has been loaded into client RAM
    *pLength = length;
    return CM_SUCCESS;
}

// Start loading the composite identified by t.
result_t TlvLoader::startLoadComposite(item_id_t t)
{
    result_t ret = findType(t);
    if (ret != CM_SUCCESS)
    {
        return ret;
    }

    item_len_t length;
    m_nvram->read((uint8_t *)&length, sizeof(item_len_t));

    m_stackIndex++;
    m_loadStack[m_stackIndex].length = length;
    m_loadStack[m_stackIndex].valueOffset = m_nvram->getOffset();
    return CM_SUCCESS;
}


//
// @note we don't check for coherence, i.e. that the sum of the lengths
//  of the components add up to the length in the composite header.
result_t TlvLoader::endLoadComposite()
{
    if (m_stackIndex < 0)
    {
        // We're not in a composite. xxx error should be INVALID_CALL, the client called End without a corresponding Start.
        return CM_INCOHERENT_DATA;
    }
    // Move to next item in m_nvram (in case we didn't just read the last component of this composite).
    m_nvram->setOffset(m_loadStack[m_stackIndex].valueOffset + m_loadStack[m_stackIndex].length);
    m_stackIndex--;
    return CM_SUCCESS;
}


// Look in current context for the requested type.
//
// In case of success, it sets m_nvram offset to the location after T.
// If T is not found, it returns m_nvram offset to its value at start of the call,
// so next attempt starts from same position as this one did.
// xxx Do we need something more advanced to support out-of-order load?
// Yes, we could search from current position as above, but if that
// failed, return to start of the composite and search again.
result_t TlvLoader::findType(item_id_t t)
{
    result_t ret;
    // Save start location of search so we can restore it if search fails.
    unsigned int start_offset = m_nvram->getOffset();

    DBG_PRT("%s: type=%hx start_offset=%d m_stackIndex=%d\n",
            __PRETTY_FUNCTION__, t, start_offset, m_stackIndex);

    if (m_stackIndex == -1)
    {
        // We're not within a composite, so no search is needed.
        ret = matchType(t);
    }
    else
    {
        ret = findTypeInComposite(t);
    }

    if (ret == CM_NOT_FOUND)
    {
        DBG_PRT("%s: type=%hx not found so restore m_nvram offset=%u\n",
                __PRETTY_FUNCTION__, t, start_offset);
        m_nvram->setOffset(start_offset);
    }
    return ret;
}

// Search for type t in the context of a composite.
// Search forward from current m_nvram offset.
//
// @pre we're in a composite
//
// The search strategy is: search forward, to end of the composite.
result_t TlvLoader::findTypeInComposite(item_id_t t)
{
    assert(m_stackIndex >= 0);

    const CompositeLoadContext & context = m_loadStack[m_stackIndex];
    while (m_nvram->getOffset() - context.valueOffset + HDR_LENGTH < context.length)
    {
        // Enough space for a HDR (T + L) remains in current composite.
        result_t ret = matchType(t);
        if (ret != CM_NOT_FOUND)
        {
            // Found, or m_nvram error (trying to read beyond end of m_nvram file).
            return ret;
        }
        // T did not match, so keep searching.
        item_len_t len;
        if (!m_nvram->read((uint8_t *)&len, sizeof(len)))
        {
            return CM_READ_FAIL;
        }
        // Move to next element.
        m_nvram->adjustOffset(len);
    }
    // We got to end of current composite without finding t.
    return CM_NOT_FOUND;
}


// See if value form m_nvram matches t.
// This advances m_nvram read loc to start of L.
// @pre xxx enough bytes should remain in m_nvram that we can read T.
result_t TlvLoader::matchType(item_id_t t)
{
    // Read T from next location in m_nvram.
    item_id_t found_t;
    if (!m_nvram->read((uint8_t *)&found_t, sizeof(found_t)))
    {
        DBG_PRT("%s: type=%hx read fail m_nvram offset=%lu\n",
                __PRETTY_FUNCTION__, t, m_nvram->getOffset() - sizeof(found_t));
        return CM_READ_FAIL;
    }

    if (found_t == t)
    {
        return CM_SUCCESS;
    }
    DBG_PRT("%s: type=%hx does not match %hx found at m_nvram offset=%lu\n",
            __PRETTY_FUNCTION__, t, found_t, m_nvram->getOffset() - sizeof(found_t));
    return CM_NOT_FOUND;
}

}
