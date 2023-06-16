///
// JSON format config data, for storage in non-volatile media, e.g. a file.
// This is defined as a separate class hierarchy, since it's an optional feature:
// not all managed objects are saved in NVRAM.
//
// This module asserts that the data passed to it by the client is valid, e.g.
// in writing:
//  - no endWriteObject without matching startWriteObject.
//  - no more than stackDepth nested calls to startWriteObject.
//
// The expected sequence of client calls in writing:
// - writeSimple, or
// - startWriteObject, then
//   - writeSimple or startWriteObject
//   - a call to endWriteObject for each call to startWriteObject
//

#include "cfg_mgr_json_writer.h"
#include "cfg_mgr_dbg.h"
#include <cstring> // strlen

using namespace std;

namespace cfg_mgr
{

JsonWriter::JsonWriter(Nvram * pNvram): m_singleIndent(" "), m_nvram(pNvram)
{
}


JsonWriter::~JsonWriter()
{
}


void JsonWriter::startWrite()
{
    m_nvram->write((const uint8_t *)"{", 1);
    m_context.init();
}

void JsonWriter::endWrite()
{
    m_nvram->write((const uint8_t *)"\n}", 2);
}

void JsonWriter::writeSimple(const char * name, item_len_t length, const uint8_t * v, JSON_PRT_FPTR prt)
{
    startWriteMember(name);
    string val_str = prt(v, length);
    m_nvram->write((const uint8_t *)val_str.c_str(), val_str.size());
    m_context.setIsFirstMember(false);
}


//
void JsonWriter::startWriteObject(const char * name)
{
    return startWriteComposite(name, OBJECT);
}


// Close writing of Object.
void JsonWriter::endWriteObject()
{
    return endWriteComposite(OBJECT);
}


//
void JsonWriter::startWriteArray(const char * name)
{
    return startWriteComposite(name, ARRAY);
}

//
void JsonWriter::endWriteArray()
{
    return endWriteComposite(ARRAY);
}

// Start writing Object or Array.
void JsonWriter::startWriteComposite(const char * name, ValueType t)
{
    startWriteMember(name);
    m_nvram->write((const uint8_t *)((t == OBJECT) ? "{" : "["), 1);
    m_context.push(t);
}

// Close writing of Object or Array.
void JsonWriter::endWriteComposite(ValueType t)
{
    m_context.pop();
    m_nvram->write((const uint8_t *)"\n", 1);
    writeIndent();
    m_nvram->write((const uint8_t *)((t == OBJECT) ? "}" : "]"), 1);
    m_context.setIsFirstMember(false);
}

// Called when starting an object: close the previous one with a comma if necessary.
// xxx line is wrong, since lines are optional whitespace.
void JsonWriter::writeEndPrecedingLine()
{
    if (!m_context.isFirstMember())
    {
        m_nvram->write((const uint8_t *)",", 1);
    }
    m_nvram->write((const uint8_t *)"\n", 1);
}

void JsonWriter::writeIndent()
{
    for (unsigned i = 0; i < m_context.getIndex() + 1; i++)
    {
        m_nvram->write((const uint8_t *)m_singleIndent.c_str(), m_singleIndent.length());
    }
}

// Common start for simple, object, array.
void JsonWriter::startWriteMember(const char * name)
{
    writeEndPrecedingLine();
    writeIndent();
    if (m_context.getType() == OBJECT)
    {
        // In an object, write the member's name (but not in an array).
        m_nvram->write((const uint8_t *)"\"", 1);
        m_nvram->write((const uint8_t *)name, strlen(name));
        m_nvram->write((const uint8_t *)"\": ", 3);
    }
}

}
