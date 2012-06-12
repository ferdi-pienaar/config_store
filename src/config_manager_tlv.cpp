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
// The client monitors the "complete" field returned by loadSimple to determine if
// one or more of the composite loads in progress are complete.
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


// Close writing of composite by writing L of TLV
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


// Load T from NVRAM and return it
t_cm_result Tlv::getType(cm_item_id_t * id)
{
    if (!nvram->read((uint8_t *)id, sizeof(cm_item_id_t)))
    {
        return CM_READ_FAIL;
    }
    return CM_SUCCESS;
}


// Client has identified T returned by getType() as simple:
// Load the simple item into the provided memory
// @param pRam
// @param length in/out, in: available memory, out: amount of data written to pRam
// @param containerComplete, out: number of containers complete
//
// @note: if the length is unexpected, we could skip just that item, but it's
//        simpler to just return an error, presumably forcing the client to abandon
//        the load process completely.
t_cm_result Tlv::loadSimple(uint8_t * pRam, cm_item_len_t * pLength, unsigned * complete)
{
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
    *complete = 0;
    return updateContainer(length, complete);
}


// Client has identified T returned by getType() as composite:
// start loading the composite.
t_cm_result Tlv::loadComposite()
{
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


// Skip item: should be called after getType() only
t_cm_result Tlv::skipItem(unsigned * complete)
{
    cm_item_len_t length;
    nvram->read((uint8_t *)&length, sizeof(cm_item_len_t));

    // Skip over the V of TLV
    nvram->adjustOffset(length);

    return updateContainer(length, complete);
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


// After loading an item, check if the composite container it is in has been completely loaded, and,
// if so, update the parameter "complete", used to indicate to client how many
// composites have been loaded.
// This function may call itself recursively.
// @param length - input, length (L in its TLV) of the item that has finished loading.
// @param complete - input/output, the number of composites which have been completely loaded.
t_cm_result Tlv::updateContainer(cm_item_len_t length, unsigned * complete)
{
    if (stackIndex == -1)
    {
        // Already at bottom of stack: nothing to pop, so stop recursing
        return CM_SUCCESS;
    }
    
    Tlv::compositeLoadContext * context = &(loadStack[stackIndex]);

    context->readBytes += length + sizeof(cm_item_id_t) + sizeof(cm_item_len_t);
    
    if (context->readBytes < context->length)
    {
        // Composite is incomplete: we don't check further containers
        return CM_SUCCESS;
    }

    if (context->readBytes > context->length)
    {
        // Composite is incoherent: length of component > the remaining length of composite
        return CM_INCOHERENT_DATA;
    }
                
    // readBytes == length => component completes its container
    (*complete)++;
    stackIndex--; // Maybe next-level container is also complete...
    return updateContainer(context->length, complete); // update contribution of container to its container
}

