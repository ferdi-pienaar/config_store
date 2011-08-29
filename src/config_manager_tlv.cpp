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


void Tlv::writeSimple(cm_item_id_t t, cm_item_len_t l, const uint8_t * v)
{
    nvram->write((uint8_t *)&t, sizeof(t));
    nvram->write((uint8_t *)&l, sizeof(l));
    nvram->write(v, l);

    addLengthToComposite(l);
}


// xxx don't really need stackItem->length: could write it to NVRAM and increment it there,
// if we don't mind the repeated read/write to NVRAM.
void Tlv::startWriteComposite(cm_item_id_t t)
{
    assert(stackIndex < (int)stackDepth);

    stackIndex++;

    Tlv::compositeContext * context = &(stack[stackIndex]);

    nvram->write((uint8_t *)&t, sizeof(t));
    context->length = 0;
    context->lengthOffset = nvram->getOffset();

    nvram->adjustOffset(sizeof(cm_item_len_t)); // reserve space for L, to be written later
}


// Close writing of composite by writing L of TLV
// xxx return boolean to indicate if we've reached the bottom of the stack?
void Tlv::endWriteComposite()
{
    assert(stackIndex >= 0); // we must be inside a composite to end one
    
    Tlv::compositeContext * context = &(stack[stackIndex]);

    nvram->setOffset(context->lengthOffset);
    nvram->write((uint8_t *)&(context->length), sizeof(context->length));

    // switch to context of owning composite (or set to -1 if we're exiting the final composite)
    stackIndex--;

    addLengthToComposite(context->length);

    if (stackIndex == -1)
    {
        // When final composite access is complete, we're done
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
t_cm_result Tlv::loadSimple(uint8_t * pRam, cm_item_len_t * length, unsigned * complete)
{
    cm_item_len_t l;
    nvram->read((uint8_t *)&l, sizeof(cm_item_len_t));

    if (*length < l)
    {
        return CM_READ_FAIL; // xxx need insufficient_mem return code
    }
    
    nvram->read(pRam, l);

    *length = l;

    *complete = popLoadStack(l);
    return CM_SUCCESS;
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
    stack[stackIndex].length = length;
    return CM_SUCCESS;
}


// Add component's contribution to the length of the composite it is contained in.
// @param L field of component, excluding length of its T + L, which is added by this method
void Tlv::addLengthToComposite(unsigned length)
{
    if (stackIndex >= 0)
    {
        // Current item is member of a composite, so add component's contribution to its length
        stack[stackIndex].length += length + sizeof(cm_item_id_t) + sizeof(cm_item_len_t);
    }
}


// @return the number of composites which have been completely loaded after loading this
//         simple component.
unsigned Tlv::popLoadStack(cm_item_len_t length)
{
    unsigned complete = 0;
    cm_item_len_t containedLength = length + sizeof(length) + sizeof(cm_item_id_t);

    while (stackIndex >= 0)
    {
        stack[stackIndex].length -= containedLength;  // xxx do this here?  we read T earlier...

        if (stack[stackIndex].length == 0)
        {
            complete++;
            stackIndex--; // This composite is complete; maybe next-level container is also
            containedLength += sizeof(length) + sizeof(cm_item_id_t);
        }
        else
        {
            // Composite is incomplete: we don't check further containers
            break;
        }
    }
    return complete;
}

