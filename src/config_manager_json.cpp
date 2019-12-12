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
// 1. Load Simple:
//  1.1 startLoadSimple
//  1.2 endLoadSimple if startLoadSimple was OK.
// OR
// 2. Load Composite:
//  2.1 startLoadComposite, then the next steps follow iff that returns OK.
//  2.2 Load components, either Simple or themselves Composite
//  2.3 endLoadComposite.
//

#include <stdint.h> // uint8_t, etc
#include "config_manager_json.h"
#include "config_manager_dbg.h"
#include <iostream>
#include <string.h> // memcpy
#include "nvram.h"
#include <assert.h>

using namespace std;

namespace cfg_mgr
{

Json::Json(Nvram * pNvram): nvram(pNvram)
{
}


Json::~Json()
{
}


void Json::reset()
{
}


void Json::writeSimple(const char * name, item_len_t length, const uint8_t * v, JSON_PRT_FPTR prt)
{
    closePredecessorLine();
    nvram->write((const uint8_t *)indent.c_str(), indent.size());
    writeName(name);
    string val_str = prt(v, length);
    nvram->write((const uint8_t *)val_str.c_str(), val_str.size());
    m_writeContext.isFirstMember = false;
}


//
void Json::startWriteComposite(const char * name)
{
    closePredecessorLine();
    nvram->write((const uint8_t *)indent.c_str(), indent.size());
    writeName(name);
    nvram->write((const uint8_t *)"{", 1);

    indent += " ";
    m_writeContext.isFirstMember = true;
}


// Close writing of composite.
void Json::endWriteComposite()
{
    indent.resize(indent.size() - 1);

    nvram->write((const uint8_t *)"\n", 1);
    nvram->write((const uint8_t *)indent.c_str(), indent.size());
    nvram->write((const uint8_t *)"}", 1);
}


//
void Json::startWriteArray(const char * name)
{
    closePredecessorLine();
    nvram->write((const uint8_t *)indent.c_str(), indent.size());
    writeName(name);
    nvram->write((const uint8_t *)"[", 1);

    indent += " ";
    m_writeContext.isFirstMember = true;
    m_writeContext.isInArray = true;
}

//
void Json::endWriteArray()
{
    indent.resize(indent.size() - 1);

    nvram->write((const uint8_t *)"\n", 1);
    nvram->write((const uint8_t *)indent.c_str(), indent.size());
    nvram->write((const uint8_t *)"]", 1);

    m_writeContext.isInArray = false;
}

result_t Json::startLoadSimple(const char * name)
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

result_t Json::endLoadSimple(item_len_t * length, uint8_t * pRam, JSON_SET_FPTR set)
{

    return CM_SUCCESS;
}

result_t Json::startLoadComposite(const char * name)
{
    return CM_SUCCESS;
}

result_t Json::endLoadComposite()
{

    return CM_SUCCESS;
}


// Called when starting an object: close the previous one with a comma if necessary.
void Json::closePredecessorLine()
{
    if (!m_writeContext.isFirstMember)
    {
        nvram->write((const uint8_t *)",", 1);
    }
    nvram->write((const uint8_t *)"\n", 1);
}

// If necessary, write name in quotes, followed by ": ".
void Json::writeName(const char * name)
{
    if (m_writeContext.isInArray)
    {
        // In an array, the name has already been written as
        // the array's name.
        return;
    }
    nvram->write((const uint8_t *)"\"", 1);
    nvram->write((const uint8_t *)name, strlen(name));
    nvram->write((const uint8_t *)"\": ", 3);
}

}
