/// 
// Type-Length-Value format of data, for storage in non-volatile media, e.g. a file.
// This is defined as a separate class hierarchy, since it's an optional feature:
// not all managed objects are saved in NVRAM.
//
// This module asserts that the data passed to it by the client is valid, e.g.
//  - no endWriteComposite without matching startWriteComposite.
//  - no more than stackDepth nested calls to startWriteComposite.
//

#include <stdint.h> // uint8_t, etc
#include "config_manager_tlv.h"
#include "../../cfg_man/src/config_manager_dbg.h"
#include <iostream>
#include <string.h> // memcpy
#include "nvram.h"
#include <assert.h>

using namespace std;

Tlv::Tlv(): stackIndex(-1)
{
    nvram = new Nvram;
}

Tlv::~Tlv()
{
    delete nvram;
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
        // Final composite is complete: we're done
        nvram->accessComplete();
    }
}


// Load T from NVRAM and return it
t_cm_result Tlv::getType(cm_item_id_t * id)
{
    nvram->read((uint8_t *)id, sizeof(cm_item_id_t));
    return CM_SUCCESS;
}


// T returned has been identified as composite:
// start loading the composite by returning the next T
// (and start keeping track of the composite's L)
// @param pRam
// @param length in/out, in: available memory, out: used memory
// @param containerComplete, out: number of containers complete
t_cm_result Tlv::loadSimple(uint8_t * pRam, cm_item_len_t * pLength, unsigned * complete)
{
    t_cm_result ret = CM_SUCCESS;
    cm_item_len_t length;
    nvram->read((uint8_t *)&length, sizeof(cm_item_len_t));

    if (*pLength != length)
    {
        // skip unreadable item
        nvram->adjustOffset(length);
        // Inform application what's in NVRAM can't find in pRam
        ret = CM_READ_FAIL; // xxx need insufficient_mem return code
    }
    else
    {
        nvram->read(pRam, length);
    }

    *pLength = length;
    popLoadStack(length, complete);
    return ret;
}


// T returned has been identified as composite:
// start loading the composite by returning the next T
// (and start keeping track of the composite's L).
// @param T, out, type of 1st component
// @param containerComplete, out
t_cm_result Tlv::loadComposite()
{
    cm_item_len_t length;
    nvram->read((uint8_t *)&length, sizeof(cm_item_len_t));

    if (stackIndex >= 0)
    {
        // xxx is container complete?
    }

    stackIndex++;
    loadStack[stackIndex].length = length;
    loadStack[stackIndex].readBytes = 0;
    return CM_SUCCESS;
}


// Skip item: should be called after getType() only
void Tlv::skipItem(unsigned * complete)
{
    cm_item_len_t length;
    nvram->read((uint8_t *)&length, sizeof(cm_item_len_t));

    // Skip over the V of TLV
    nvram->adjustOffset(length);

    popLoadStack(length, complete);
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


// @complete - output, the number of composites which have been completely loaded after loading this
//         simple component.
t_cm_result Tlv::popLoadStack(cm_item_len_t length, unsigned * complete)
{
    if (stackIndex == -1)
    {
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
                
    // readBytes == length => component completes its composite
    (*complete)++;
    stackIndex--; // Maybe next-level container is also complete...
    popLoadStack(context->length, complete); // remove contribution of container from its container
    return CM_SUCCESS;
}

