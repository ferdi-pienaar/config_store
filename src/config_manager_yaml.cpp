/// 
// YAML format config data, for storage in non-volatile media, e.g. a file.
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
#include "config_manager_yaml.h"
#include "config_manager_dbg.h"
#include <iostream>
#include <string.h> // memcpy
#include "nvram.h"
#include <assert.h>

using namespace std;

Yaml::Yaml(Nvram * pNvram): nvram(pNvram), stackIndex(0)
{
}


Yaml::~Yaml()
{
}


void Yaml::reset()
{
    stackIndex = 0;
}


void Yaml::writeSimple(const char * name, cm_item_id_t t, const uint8_t * v, cm_item_len_t length, YAML_PRT_FPTR prt)
{    
    write_indent();
    cout << name << ": ";
    prt(stdout, v, length);

    cout << endl;
}


// Don't actually write anything yet, since it may turn out that this
// composite is empty.
void Yaml::startWriteComposite(const char * name, cm_item_id_t t)
{
    write_indent();
    cout << name << ": " << endl;
    stackIndex++;
}


// Close writing of composite by writing L of TLV
// xxx return boolean to indicate if we've reached the bottom of the stack?
void Yaml::endWriteComposite()
{
    stackIndex--;

    write_indent();
    cout << "end" << endl;
}


// Load T from NVRAM and return it
t_cm_result Yaml::getType(cm_item_id_t * id)
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
t_cm_result Yaml::loadSimple(uint8_t * pRam, cm_item_len_t * pLength, unsigned * complete)
{
    t_cm_result ret = CM_SUCCESS;
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
    t_cm_result ret2 = updateContainer(length, complete);
    if (ret2 != CM_SUCCESS)
    {
        return ret2;
    }
    return ret;
}


// Client has identified T returned by getType() as composite:
// start loading the composite.
t_cm_result Yaml::loadComposite()
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
void Yaml::skipItem(unsigned * complete)
{
    cm_item_len_t length;
    nvram->read((uint8_t *)&length, sizeof(cm_item_len_t));

    // Skip over the V of TLV
    nvram->adjustOffset(length);

    updateContainer(length, complete);
}




// After loading an item, check if the composite container it is in has been completely loaded, and,
// if so, update the parameter "complete", used to indicate to client how many
// composites have been loaded.
// This function may call itself recursively.
// @param length - input, length (L in its TLV) of the item that has finished loading.
// @param complete - input/output, the number of composites which have been completely loaded.
t_cm_result Yaml::updateContainer(cm_item_len_t length, unsigned * complete)
{
    if (stackIndex == -1)
    {
        // Already at bottom of stack: nothing to pop
        return CM_SUCCESS;
    }
    
    Yaml::compositeLoadContext * context = &(loadStack[stackIndex]);

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
    updateContainer(context->length, complete); // update contribution of container to its container
    return CM_SUCCESS;
}

void Yaml::write_indent()
{
    for (int i = 0; i < stackIndex; i++)
    {
        cout << " ";
    }
}

