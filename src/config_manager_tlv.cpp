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
    nvram->adjustOffset(sizeof(cm_item_id_t) + sizeof(cm_item_len_t));
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
        assert(endOffset >= sizeof(cm_item_id_t) + sizeof(cm_item_len_t));
        endOffset -= sizeof(cm_item_id_t) + sizeof(cm_item_len_t);
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
// @param t: type to load.
// @param pLength in/out, in: available memory, out: amount of data written to pRam
// @param pRam
//
// @note: if the length is unexpected, we could skip just that item, but it's
//        simpler to just return an error, presumably forcing the client to abandon
//        the load process completely.
t_cm_result Tlv::loadSimple(cm_item_id_t t, cm_item_len_t * pLength, uint8_t * pRam)
{
    t_cm_result ret = findType(t);
    if (ret != CM_SUCCESS)
    {
        return ret;
    }

    cm_item_len_t length;
    nvram->read((uint8_t *)&length, sizeof(cm_item_len_t));

    if (*pLength != length)
    {
        *pLength = 0; // No data loaded into client RAM
        return CM_INCOHERENT_DATA;
    }

    DBG_PRT("loadSimple: %d at %p\n", length, pRam);

    if (!nvram->read(pRam, length))
    {
        // This error aborts the loading process, and there's no need to updateContainer
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

    if (stackIndex >= 0)
    {
        // xxx is container complete?
        // No, let's assume or assert that a container must contain something.
    }

    stackIndex++;
    loadStack[stackIndex].length = length;
    loadStack[stackIndex].readBytes = 0;
    return CM_SUCCESS;
}


//
t_cm_result Tlv::endLoadComposite()
{
    if (stackIndex < 0)
    {
        // We're not in a composite.
        return CM_INCOHERENT_DATA;
    }
    stackIndex--;
    return CM_SUCCESS;
}


// Look in current context for the requested type.
// This function does not return a location, because, in case of success,
// it sets NVRAM position to the location after T.
// The search strategy is:
// Read the next byte for T: do we need something more advanced to
//  support out-of-order load? xxx
// If T is not found, return NVRAM and context readoffset to start,
// so next attempt starts from same position.
t_cm_result Tlv::findType(cm_item_id_t t)
{
    t_cm_result ret = CM_NOT_FOUND;
    // Save start location of search so we can restore it if search fails.
    int start_offset = nvram->getOffset();

    if (stackIndex == -1)
    {
        // We're not within a composite.
        ret = matchType(t);
        if (ret == CM_NOT_FOUND)
        {
            nvram->setOffset(start_offset);
        }
        return ret;
    }

    // We're in a compound within whose context we search for a matching T.
    compositeLoadContext * context = &(loadStack[stackIndex]);
    cm_item_len_t start_readBytes = context->readBytes;
    while (context->readBytes + sizeof(cm_item_id_t) + sizeof(cm_item_len_t) < context->length)
    {
        ret = matchType(t);
        if (ret == CM_READ_FAIL)
        {
            return CM_READ_FAIL;
        }
        if (ret == CM_SUCCESS)
        {
            goto done;
        }
        // T did not match, so keep searching.
        cm_item_len_t len;
        if (!nvram->read((uint8_t *)&len, sizeof(len)))
        {
            return CM_READ_FAIL;
        }
        // Move to next element.
        nvram->adjustOffset(len);
        context->readBytes += sizeof(cm_item_id_t) + sizeof(len) + len;
    }
done:
    // Either search failed without read errors, or success.
    if (ret != CM_SUCCESS)
    {
        nvram->setOffset(start_offset);
        context->readBytes = start_readBytes;
    }
    return ret;
}

// See if value form NVRAM matches t.
// This advances NVRAM read loc to start of L.
t_cm_result Tlv::matchType(cm_item_id_t t)
{
    cm_item_id_t found_t;

    // Read T from next location in NVRAM.
    if (!nvram->read((uint8_t *)&found_t, sizeof(found_t)))
    {
        return CM_READ_FAIL;
    }

    if (found_t == t)
    {
        return CM_SUCCESS;
    }
    return CM_NOT_FOUND;
}

// Add component's contribution to the length of the composite it is contained in.
// @param L field of component, excluding length of its T + L, which is added by this method
void Tlv::addLengthToComposite(unsigned length)
{
    if (stackIndex >= 0)
    {
        // Current item is member of a composite, so add component's contribution to its length
        writeStack[stackIndex].length += length + sizeof(cm_item_id_t) + sizeof(cm_item_len_t);
    }
}
