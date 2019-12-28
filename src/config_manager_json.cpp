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

#include "config_manager_json.h"
#include "config_manager_dbg.h"
#include <string.h> // memcpy
#include "nvram.h"
#include <assert.h>

using namespace std;

namespace cfg_mgr
{

Json::Json(Nvram * pNvram): m_singleIndent(" "), m_stackIndex(0), m_nvram(pNvram)
{
}


Json::~Json()
{
}


void Json::startWrite()
{
    m_nvram->write((const uint8_t *)"{", 1);

    m_stackIndex = 0;
    m_writeContext[m_stackIndex].m_isFirstMember = true;
    m_writeContext[m_stackIndex].m_type = OBJECT;
}

void Json::endWrite()
{
    m_nvram->write((const uint8_t *)"\n}", 2);
}


void Json::writeSimple(const char * name, item_len_t length, const uint8_t * v, JSON_PRT_FPTR prt)
{
    writeEndPrecedingLine();
    writeIndent();
    if (m_writeContext[m_stackIndex].m_type == OBJECT)
    {
        writeName(name);
    }
    string val_str = prt(v, length);
    m_nvram->write((const uint8_t *)val_str.c_str(), val_str.size());
    m_writeContext[m_stackIndex].m_isFirstMember = false;
}


//
void Json::startWriteComposite(const char * name)
{
    writeEndPrecedingLine();
    writeIndent();
    if (m_writeContext[m_stackIndex].m_type == OBJECT)
    {
        writeName(name);
    }
    m_nvram->write((const uint8_t *)"{", 1);
    m_stackIndex++;
    m_writeContext[m_stackIndex].m_isFirstMember = true;
    m_writeContext[m_stackIndex].m_type = OBJECT;
}


// Close writing of composite.
void Json::endWriteComposite()
{
    m_stackIndex--;
    m_nvram->write((const uint8_t *)"\n", 1);
    writeIndent();
    m_nvram->write((const uint8_t *)"}", 1);
    m_writeContext[m_stackIndex].m_isFirstMember = false;
}


//
void Json::startWriteArray(const char * name)
{
    writeEndPrecedingLine();
    writeIndent();
    if (m_writeContext[m_stackIndex].m_type == OBJECT)
    {
        writeName(name);
    }
    m_nvram->write((const uint8_t *)"[", 1);
    m_stackIndex++;
    m_writeContext[m_stackIndex].m_isFirstMember = true;
    m_writeContext[m_stackIndex].m_type = ARRAY;
}

//
void Json::endWriteArray()
{
    m_stackIndex--;

    m_nvram->write((const uint8_t *)"\n", 1);
    writeIndent();
    m_nvram->write((const uint8_t *)"]", 1);
}

// Return success if name (in quotes) followed by ':' is next.
result_t Json::startLoadSimple(const char * name)
{
    skipws();
    if (!m_loadContext[m_stackIndex].m_isFirstMember)
    {
        if (!isNextLoad(",", 1))
        {
            return CM_NOT_FOUND;
        }
        skipws();
    }
    return loadName(name) ? CM_SUCCESS : CM_NOT_FOUND;
}

// Read the value from JSON in nvram, and convert to value in RAM if set function.
// @param length - output.
result_t Json::endLoadSimple(item_len_t * length, uint8_t * pRam, JSON_SET_FPTR set)
{
    string valstr = loadValue();

    set(pRam, *length, valstr);
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
void Json::writeEndPrecedingLine()
{
    if (!m_writeContext[m_stackIndex].m_isFirstMember)
    {
        m_nvram->write((const uint8_t *)",", 1);
    }
    m_nvram->write((const uint8_t *)"\n", 1);
}

void Json::writeIndent()
{
    for (unsigned i = 0; i < m_stackIndex + 1; i++)
    {
        m_nvram->write((const uint8_t *)m_singleIndent.c_str(), m_singleIndent.length());
    }
}


// If necessary, write name in quotes, followed by ": ".
void Json::writeName(const char * name)
{
    m_nvram->write((const uint8_t *)"\"", 1);
    m_nvram->write((const uint8_t *)name, strlen(name));
    m_nvram->write((const uint8_t *)"\": ", 3);
}

// Return true if name, surrounded by quotes and followed by ':' is next in nvram,
// else return false and set nvram xxx.
bool Json::loadName(const char * name)
{
    if (!isNextLoad("\"", 1))
    {
        // No opening quote.
        return false;
    }

    // A name should be next, xxx but is it necessarily the name we are looking for?
    if (!isNextLoad(name, strlen(name)))
    {
        return false;
    }

    // A name should be next, xxx but is it necessarily the name we are looking for?
    if (!isNextLoad("\"", 1))
    {
        // No closing quote.
        return false;
    }

    skipws();
    if (!isNextLoad(":", 1))
    {
        // No ':' after the name.
        return false;
    }
    skipws();
    return true;
}


// If next len chars in nvram match b, return true, and set nvram offset to end of match.
// if no match, return false, and set nvram offset to xxx.
bool Json::isNextLoad(const char * b, unsigned len)
{
    for (unsigned i = 0; i < len; i++)
    {
        char candidate;
        if (!m_nvram->read((uint8_t *)&candidate, 1))
        {
            // end of nvram or other read failure.
            return false;
        }
        if (candidate != b[i])
        {
            // mismatch
            return false;
        }
    }
    return true;
}

// If the value starts with '"' (value is a string), go to the next '"'.
// Else (value is not a string, i.e. number of true or false or null)
//  value ends at next whitespace or ',' or ']' or '}'.
//
string Json::loadValue()
{
    char c;
    if (!m_nvram->read((uint8_t *)&c, 1))
    {
        // end of nvram or other read failure.
        return "";
    }
    if (c == '"')
    {
        return c + finishLoadString();
    }
    else
    {
        return c + finishLoadNonString();
    }
}

// Load up to and including closing '"' of string.
string Json::finishLoadString()
{
    string str;
    char c;
    while (m_nvram->read((uint8_t *)&c, 1))
    {
        str += c;
        if (c == '"')
        {
            break;
        }
    }
    return str;
}

// Load until end of value: BEFORE whitespace or ',' or ']' or '}'.
string Json::finishLoadNonString()
{
    string str;
    char c;
    while (m_nvram->read((uint8_t *)&c, 1))
    {
        if (isws(c) || (c == ',') || (c == ']') || (c == '}'))
        {
            // Next character read will be same again.
            m_nvram->adjustOffset(-1);
            break;
        }
        str += c;
    }
    return str;
}


// advance nvram offset to byte after whitespace.
bool Json::skipws()
{
    char candidate;
    while (m_nvram->read((uint8_t *)&candidate, 1))
    {
        if (!isws(candidate))
        {
            // Next character read will be this non-ws again.
            m_nvram->adjustOffset(-1);
            return true;
        }
    }
    // reached end of nvram or some other read fail.
    return false;
}

bool Json::isws(char c)
{
    if (c == ' ') return true;
    if (c == '\t') return true;
    if (c == '\n') return true;
    if (c == '\r') return true;
    return false;
}
}
