///
// Json format config data, for storage in non-volatile media, e.g. a file.
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
#include "config_manager_json.h"
#include "config_manager_dbg.h"
#include <iostream>
#include <string.h> // memcpy
#include "nvram.h"
#include <assert.h>

using namespace std;

Json::Json(Nvram * pNvram): nvram(pNvram), firstMember(true)
{
}


Json::~Json()
{
}


void Json::reset()
{
}


void Json::writeSimple(const char * name, cm_item_len_t length, const uint8_t * v, JSON_PRT_FPTR prt)
{
    if (!firstMember)
    {
        nvram->write((const uint8_t *)",", 1);
    }
    nvram->write((const uint8_t *)"\n", 1);
    nvram->write((const uint8_t *)indent.c_str(), indent.size());
    writeName(name);
    string val_str = prt(v, length);
    nvram->write((const uint8_t *)val_str.c_str(), val_str.size());
    firstMember = false;
}


// Don't actually write anything yet, since it may turn out that this
// composite is empty.
void Json::startWriteComposite(const char * name)
{
    nvram->write((const uint8_t *)indent.c_str(), indent.size());
    writeName(name);
    nvram->write((const uint8_t *)"{", 1);
    indent += " ";
    firstMember = true;
}


// Close writing of composite by writing L of TLV
// xxx return boolean to indicate if we've reached the bottom of the stack?
void Json::endWriteComposite()
{
    nvram->write((const uint8_t *)indent.c_str(), indent.size());
    nvram->write((const uint8_t *)"\n}", 2);
    indent.resize(indent.size() - 1);
}

t_cm_result Json::startLoadSimple(const char * name)
{
    unsigned int readLen = strlen(name) + 2; // add space for quotes.
    char readName[129];
    if (!nvram->read((uint8_t *)readName, readLen))
    {
        return CM_READ_FAIL;
    }
    if (strncmp(readName, name, readLen) != 0)
    {
        return CM_NOT_FOUND;
    }
    return CM_SUCCESS;

}

t_cm_result Json::endLoadSimple(cm_item_len_t * length, uint8_t * pRam, JSON_SET_FPTR set)
{

    return CM_SUCCESS;
}

t_cm_result Json::startLoadComposite(const char * name)
{
    return CM_SUCCESS;
}

t_cm_result Json::endLoadComposite()
{

    return CM_SUCCESS;
}

void Json::startWriteList(const char * name)
{

}

void Json::endWriteList()
{

}

void Json::writeName(const char * name)
{    
    nvram->write((const uint8_t *)"\"", 1);
    nvram->write((const uint8_t *)name, strlen(name));
    nvram->write((const uint8_t *)"\": ", 3);
}
