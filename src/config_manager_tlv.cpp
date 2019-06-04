///
// Type-Length-Value format config data, for storage in non-volatile media, e.g. a file.
// This is defined as a separate class hierarchy, since it's an optional feature:
// not all managed objects are saved in NVRAM.
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
// The expected sequence of client calls in loading:
// - getType, then
// - loadComposite or loadSimple, depending on client's interpretation of the type
//   returned by getType
// The client monitors the "complete" field returned by loadSimple and skipItem
// to determine if one or more of the composite loads in progress are complete.
//

#include <stdint.h> // uint8_t, etc
#include "config_manager_tlv.h"
#include "config_manager_dbg.h"
#include <iostream>
#include <string.h> // memcpy
#include "nvram.h"
#include <assert.h>

using namespace std;

Tlv::Tlv(Nvram * pNvram): nvram(pNvram), stackIndex(-1)
{
}


Tlv::~Tlv()
{
}


void Tlv::reset()
{
    stackIndex = -1;
}


void Tlv::writeSimple(cm_item_id_t t, cm_item_len_t len, const uint8_t * v)
{
    nvram->write((uint8_t *)&t, sizeof(t));
    nvram->write((uint8_t *)&len, sizeof(len));
    nvram->write(v, len);

    addLengthToComposite(len);
}


// Don't actually write anything yet, since it may turn out that this
// composite is empty.
void Tlv::startWriteComposite(cm_item_id_t t)
{
    assert(stackIndex < (int)stackDepth);

    stackIndex++;

    Tlv::compositeWriteContext * context = &(writeStack[stackIndex]);

    context->id = t;
    context->length = 0;
    context->headerOffset = nvram->getOffset();

    // reserve space for T + L, to be written by endWriteComposite()
    nvram->adjustOffset(HDR_LENGTH);
}


// Close writing of composite by writing header (T and L) of TLV.
// xxx return boolean to indicate if we've reached the bottom of the stack?
void Tlv::endWriteComposite()
{
    assert(stackIndex >= 0); // we must be inside a composite to end one

    unsigned int endOffset = nvram->getOffset(); // current offset, at end of composite
    Tlv::compositeWriteContext * context = &(writeStack[stackIndex]);

    // switch to context of owning composite (or set to -1 if we're exiting the final composite)
    stackIndex--;

    if (context->length == 0)
    {
        // Empty composite: write nothing, and set offset to beginning of composite
        assert(endOffset >= HDR_LENGTH);
        endOffset -= HDR_LENGTH;
    }
    else
    {
        // Non-empty composite: write its header
        nvram->setOffset(context->headerOffset);
        nvram->write((uint8_t *)&(context->id), sizeof(context->id));
        nvram->write((uint8_t *)&(context->length), sizeof(context->length));

        addLengthToComposite(context->length);
    }

    if (stackIndex >= 0)
    {
        // We're still inside a composite, so set offset for writing next component
        nvram->setOffset(endOffset);
    }
    else
    {
        // Final composite is complete: we're done reading from NVRAM
        nvram->accessComplete();
    }
}

// Load a simple item into the provided memory.
//
// @param t: type to load.
t_cm_result Tlv::startLoadSimple(cm_item_id_t t)
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
t_cm_result Tlv::endLoadSimple(cm_item_len_t * pLength, uint8_t * pRam)
{
    cm_item_len_t length;
    nvram->read((uint8_t *)&length, sizeof(cm_item_len_t));

    if (*pLength != length)
    {
        *pLength = 0; // No data loaded into client RAM
        return CM_INCOHERENT_DATA;
    }

    // DBG_PRT("%s: %d at %p\n", __PRETTY_FUNCTION__, length, pRam);

    if (!nvram->read(pRam, length))
    {
        // This error aborts the loading process, and there's no need to updateContainer xxx obsolete comment.
        return CM_READ_FAIL;
    }
    
    // Data has been loaded into client RAM
    *pLength = length;
    return CM_SUCCESS;
}

// Start loading the composite identified by t.
t_cm_result Tlv::startLoadComposite(cm_item_id_t t)
{
    t_cm_result ret = findType(t);
    if (ret != CM_SUCCESS)
    {
        return ret;
    }

    cm_item_len_t length;
    nvram->read((uint8_t *)&length, sizeof(cm_item_len_t));

    stackIndex++;
    loadStack[stackIndex].length = length;
    loadStack[stackIndex].valueOffset = nvram->getOffset();
    return CM_SUCCESS;
}


//
// @note we don't check for coherence, i.e. that the sum of the lengths
//  of the components add up to the length in the composite header.
t_cm_result Tlv::endLoadComposite()
{
    if (stackIndex < 0)
    {
        // We're not in a composite. xxx error should be INVALID_CALL, the client called End without a corresponding Start.
        return CM_INCOHERENT_DATA;
    }
    // Move to next item in NVRAM (in case we didn't just read the last component of this composite).
    nvram->setOffset(loadStack[stackIndex].valueOffset + loadStack[stackIndex].length);
    stackIndex--;
    return CM_SUCCESS;
}


// Look in current context for the requested type.
// This function does not return a location, because, in case of success,
// it sets NVRAM position to the location after T.
t_cm_result Tlv::findType(cm_item_id_t t)
{
    t_cm_result ret;
    // Save start location of search so we can restore it if search fails.
    unsigned int start_offset = nvram->getOffset();
    
    DBG_PRT("%s: type=%hx start_offset=%d stackIndex=%d\n",
            __PRETTY_FUNCTION__, t, start_offset, stackIndex);

    if (stackIndex == -1)
    {
        // We're not within a composite.
        ret = matchType(t);
    }
    else
    {
        ret = findTypeInComposite(t);
    }
    DBG_PRT("%s DONE: type=%hx start_offset=%d stackIndex=%d\n",
            __PRETTY_FUNCTION__, t, start_offset, stackIndex);
    if (ret == CM_NOT_FOUND)
    {
        DBG_PRT("%s: type=%hx not found so restore NVRAM offset=%u\n",
                __PRETTY_FUNCTION__, t, start_offset);
        nvram->setOffset(start_offset);
    }
    return ret;
}

// Search for type t in the context of a composite.
// Search forward from current NVRAM offset.
//
// @pre we're in a compound
//
// The search strategy is: search forward, to end of the composite.
// If T is not found, return NVRAM offset to its value at start of the call,
// so next attempt starts from same position as this one did.
// xxx Do we need something more advanced to support out-of-order load?
// Yes, we could search from current position as above, but if that
// failed, return to start of the composite.
t_cm_result Tlv::findTypeInComposite(cm_item_id_t t)
{
    assert(stackIndex >= 0);
   
    compositeLoadContext * context = &(loadStack[stackIndex]);
    while (nvram->getOffset() - context->valueOffset + HDR_LENGTH < context->length)
    {
        t_cm_result ret = matchType(t);
        if (ret != CM_NOT_FOUND)
        {
            // Found, or a NVRAM error, i.e. trying to read beyond end of NVRAM file.
            return ret;
        }
        // T did not match, so keep searching.
        cm_item_len_t len;
        if (!nvram->read((uint8_t *)&len, sizeof(len)))
        {
            return CM_READ_FAIL;
        }
        // Move to next element.
        nvram->adjustOffset(len);
    }
    return CM_NOT_FOUND;    
}


// See if value form NVRAM matches t.
// This advances NVRAM read loc to start of L.
// @pre xxx enough bytes should remain in NVRAM that we can read T.
t_cm_result Tlv::matchType(cm_item_id_t t)
{
    cm_item_id_t found_t;

    // Read T from next location in NVRAM.
    if (!nvram->read((uint8_t *)&found_t, sizeof(found_t)))
    {
        DBG_PRT("%s: type=%hx read fail NVRAM offset=%lu\n",
            __PRETTY_FUNCTION__, t, nvram->getOffset() - sizeof(found_t));
        return CM_READ_FAIL;
    }

    if (found_t == t)
    {
        return CM_SUCCESS;
    }
    DBG_PRT("%s: type=%hx does not match %hx found at NVRAM offset=%lu\n",
            __PRETTY_FUNCTION__, t, found_t, nvram->getOffset() - sizeof(found_t));
    return CM_NOT_FOUND;
}

// Add component's contribution to the length of the composite it is contained in.
// @param L field of component, excluding length of its T + L, which is added by this method
void Tlv::addLengthToComposite(unsigned length)
{
    if (stackIndex >= 0)
    {
        // Current item is member of a composite, so add component's contribution to its length
        writeStack[stackIndex].length += length + HDR_LENGTH;
    }
}
