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
// The expected sequence of client calls in loading:
// 1. Load Simple:
//  1.1 startLoadSimple
//  1.2 endLoadSimple if startLoadSimple was OK.
// OR
// 2. Load Object:
//  2.1 startLoadObject, then the next steps follow iff that returns OK.
//  2.2 Load components, either Simple or themselves Objects
//  2.3 endLoadObject.
//

#include "cfg_mgr_json.h"
#include "cfg_mgr_dbg.h"
#include <cstring> // strlen
#include "nvram.h"
#include <assert.h>

using namespace std;

namespace cfg_mgr
{

Json::Json(Nvram * pNvram): m_singleIndent(" "), m_nvram(pNvram)
{
}


Json::~Json()
{
}


void Json::startWrite()
{
    m_nvram->write((const uint8_t *)"{", 1);
    m_context.init();
}

void Json::endWrite()
{
    m_nvram->write((const uint8_t *)"\n}", 2);
}

result_t Json::startLoad()
{
    skipws();
    if (!isNextRead("{", 1))
    {
        return CM_INCOHERENT_DATA;
    }
    DBG_PRT("%s OK\n", __PRETTY_FUNCTION__);
    m_context.init();
    return CM_SUCCESS;
}

result_t Json::endLoad()
{
    skipws();
    if (!isNextRead("}", 1))
    {
        return CM_INCOHERENT_DATA;
    }
    DBG_PRT("%s OK\n", __PRETTY_FUNCTION__);
    return CM_SUCCESS;
}

void Json::writeSimple(const char * name, item_len_t length, const uint8_t * v, JSON_PRT_FPTR prt)
{
    startWriteMember(name);
    string val_str = prt(v, length);
    m_nvram->write((const uint8_t *)val_str.c_str(), val_str.size());
    m_context.setIsFirstMember(false);
}


//
void Json::startWriteObject(const char * name)
{
    return startWriteComposite(name, OBJECT);
}


// Close writing of Object.
void Json::endWriteObject()
{
    return endWriteComposite(OBJECT);
}


//
void Json::startWriteArray(const char * name)
{
    return startWriteComposite(name, ARRAY);
}

//
void Json::endWriteArray()
{
    return endWriteComposite(ARRAY);
}

// Return success if name (in quotes) followed by ':' is next.
result_t Json::startLoadSimple(const char * name)
{
    DBG_PRT("%s name=%s stackIndex=%u\n", __PRETTY_FUNCTION__, name, m_context.getIndex());
    return startLoadMember(name);
}

// Read the value from JSON in nvram, and convert to value in RAM if set function.
// @param length - output.
result_t Json::endLoadSimple(item_len_t * length, uint8_t * pRam, JSON_SET_FPTR set)
{
    DBG_PRT("%s m_stackIndex=%u\n", __PRETTY_FUNCTION__, m_context.getIndex());

    string valstr = loadValue();
    DBG_PRT("%s value='%s'\n", __PRETTY_FUNCTION__, valstr.c_str());
    set(pRam, *length, valstr);
    m_context.setIsFirstMember(false);
    return CM_SUCCESS;
}

result_t Json::startLoadObject(const char * name)
{
    return startLoadComposite(name, OBJECT);
}

result_t Json::endLoadObject()
{
    return endLoadComposite(OBJECT);
}

result_t Json::startLoadArray(const char * name)
{
    return startLoadComposite(name, ARRAY);
}

result_t Json::endLoadArray()
{
    return endLoadComposite(ARRAY);
}

// Start writing Object or Array.
void Json::startWriteComposite(const char * name, ValueType t)
{
    startWriteMember(name);
    m_nvram->write((const uint8_t *)((t == OBJECT) ? "{" : "["), 1);
    m_context.push(t);
}

// Close writing of Object or Array.
void Json::endWriteComposite(ValueType t)
{
    m_context.pop();
    m_nvram->write((const uint8_t *)"\n", 1);
    writeIndent();
    m_nvram->write((const uint8_t *)((t == OBJECT) ? "}" : "]"), 1);
    m_context.setIsFirstMember(false);
}

// Start loading of Object or Array.
result_t Json::startLoadComposite(const char * name, ValueType t)
{
    DBG_PRT("%s name=%s type=%d stackIndex=%u\n", __PRETTY_FUNCTION__, name, t, m_context.getIndex());

    result_t ret = startLoadMember(name);
    if (ret != CM_SUCCESS)
    {
        return ret;
    }
    if (!isNextRead((t == OBJECT) ? "{" : "[", 1))
    {
        DBG_PRT("%s missing '%s'\n", __PRETTY_FUNCTION__, (t == OBJECT) ? "{" : "[");
        return CM_INCOHERENT_DATA;
    }
    m_context.push(t);
    return CM_SUCCESS;
}

// End loading of Object or Array.
result_t Json::endLoadComposite(ValueType t)
{
    DBG_PRT("%s type=%d, m_stackIndex=%u\n", __PRETTY_FUNCTION__, t, m_context.getIndex());

    skipws();
    if (!isNextRead((t == OBJECT) ? "}" : "]", 1))
    {
        DBG_PRT("%s missing '%s'\n", __PRETTY_FUNCTION__, (t == OBJECT) ? "}" : "]");
        return CM_INCOHERENT_DATA;
    }
    m_context.pop();
    m_context.setIsFirstMember(false);
    return CM_SUCCESS;
}

// Called when starting an object: close the previous one with a comma if necessary.
// xxx line is wrong, since lines are optional whitespace.
void Json::writeEndPrecedingLine()
{
    if (!m_context.isFirstMember())
    {
        m_nvram->write((const uint8_t *)",", 1);
    }
    m_nvram->write((const uint8_t *)"\n", 1);
}

void Json::writeIndent()
{
    for (unsigned i = 0; i < m_context.getIndex() + 1; i++)
    {
        m_nvram->write((const uint8_t *)m_singleIndent.c_str(), m_singleIndent.length());
    }
}

// Common start for simple, object, array.
void Json::startWriteMember(const char * name)
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


// Common start for simple, object, and array.
result_t Json::startLoadMember(const char * name)
{
    skipws();
    if (!m_context.isFirstMember())
    {
        // This is subsequent member, so expect ',' before its name.
        if (!isNextRead(",", 1))
        {
            DBG_PRT("%s missing ','\n", __PRETTY_FUNCTION__);
            return CM_NOT_FOUND;
        }
        skipws();
    }
    if (m_context.getType() == OBJECT)
    {
        // In an object, expect the member's name (but not in an array).
        result_t ret = findName(name);
        if (ret != CM_SUCCESS)
        {
            return ret;
        }
        skipws();
    }
    return CM_SUCCESS;
}

// Find name within the current object.
// If not found, leave nvram offset unchanged so next find starts at same offset.
result_t Json::findName(const char * name)
{
    unsigned int start_offset = m_nvram->getOffset();
    while (true)
    {
        result_t res = readName(name);
        if (res == CM_SUCCESS)
        {
            return res;
        }
        if (res != CM_NOT_FOUND)
        {
            // Error: it doesn't even look like a name, e.g. no opening or closing quotes.
            return res;
        }
        // readName returned CM_NOT_FOUND, so go to next name.
        res = toNextName();
        if (res != CM_SUCCESS)
        {
            // There are no more names in the current context, or some other error.
            m_nvram->setOffset(start_offset);
            return res;
        }
    }
    return CM_SUCCESS;
}

// Advance nvram to name within the current context.
// @pre nvram is after the ':' that follows a name within the current context.
// @post
result_t Json::toNextName()
{
    result_t ret = CM_SUCCESS;
    skipws();
    char c;
    if (!m_nvram->read((uint8_t *)&c, 1))
    {
        return CM_INCOHERENT_DATA;
    }
    switch (c)
    {
    case '{':
        ret = toCloser('{', '}');
        break;
    case '[':
        ret = toCloser('[', ']');
        break;
    case '\"':
        ret = toStringEnd();
        break;
    }
    if (ret != CM_SUCCESS)
    {
        DBG_PRT("%s unterminated '%c'\n", __PRETTY_FUNCTION__, c);
        return ret;
    }
    if (toCloser(0, ',') != CM_SUCCESS)
    {
        DBG_PRT("%s no ','\n", __PRETTY_FUNCTION__);
        return CM_NOT_FOUND;
    }
    skipws();
    return ret;
}


// Move NVRAM offset to matching closing marker.
// If we meet 'open's, they must be closed by 'close's,
// then the next 'close' represents the match we're looking for.
// Ignore opening and closing markers that appear inside strings.
// It seems this basic parsing is sufficient.
result_t Json::toCloser(char open, char close)
{
    unsigned depth = 0; // number of opens that are not closed.
    char c;
    while (m_nvram->read((uint8_t *)&c, 1))
    {
        if (c == close)
        {
            if (depth == 0)
            {
                return CM_SUCCESS;
            }
            depth--;
        }
        else if (c == open)
        {
            depth++;
        }
        else if (c == '\"')
        {
            toStringEnd();
        }
    }
    return CM_NOT_FOUND;
}

// Advance nvram offset to character after closing quote of string.
// @param - str in/out, append found characters to this string, if provided.
result_t Json::toStringEnd(string *str)
{
    bool escape = false; // are we in escape state because previous char was '\'?
    char c;
    while (m_nvram->read((uint8_t *)&c, 1))
    {
        if (str != nullptr)
        {
            *str += c;
        }
        if (!escape && (c == '"'))
        {
            return CM_SUCCESS;
        }
        escape = (c == '\\');
    }
    return CM_NOT_FOUND;
}


// Return SUCCESS if name, surrounded by quotes and followed by ':' is next in nvram,
// else return error.
// If name is not found, xxx.
result_t Json::readName(const char * name)
{
    result_t res = CM_SUCCESS;
    if (!isNextRead("\"", 1))
    {
        DBG_PRT("%s missing open\n", __PRETTY_FUNCTION__);
        return CM_INCOHERENT_DATA;
    }
    if (isNextRead(name, strlen(name)))
    {
        if (!isNextRead("\"", 1))
        {
            DBG_PRT("%s missing close for '%s'\n", __PRETTY_FUNCTION__, name);
            return CM_INCOHERENT_DATA;
        }
    }
    else
    {
        res = CM_NOT_FOUND;
        DBG_PRT("%s name '%s' not found.\n", __PRETTY_FUNCTION__, name);
        // Name doesn't match, but move to end anyway.
        if (toStringEnd() != CM_SUCCESS)
        {
            DBG_PRT("%s unclosed string.\n", __PRETTY_FUNCTION__);
            return CM_INCOHERENT_DATA;
        }
    }
    skipws();
    if (!isNextRead(":", 1))
    {
        DBG_PRT("%s missing ':'\n", __PRETTY_FUNCTION__);
        return CM_INCOHERENT_DATA;
    }
    return res;
}


// If the next 'len' chars in nvram match input 'expect', return true and set
// nvram offset to end of match.
// If mismatch, return false, with nvram offset unchanged.
bool Json::isNextRead(const char * expect, unsigned len)
{
    for (unsigned i = 0; i < len; i++)
    {
        char candidate;
        if (!m_nvram->read((uint8_t *)&candidate, 1))
        {
            // end of nvram or other read failure.
            return false;
        }
        if (candidate != expect[i])
        {
            m_nvram->adjustOffset(-(i + 1));
            return false;
        }
    }
    return true;
}

// If the value starts with '"' (value is a string), go to the next '"'.
// Else (value is not a string, i.e. number or true or false or null)
//  value ends at next whitespace or ',' or ']' or '}'.
// xxx this could fail, should return error code?
// @return the characters read from NVRAM.
string Json::loadValue()
{
    skipws();
    char c;
    if (!m_nvram->read((uint8_t *)&c, 1))
    {
        // end of nvram or other read failure.
        return "";
    }
    if (c == '"')
    {
        string str;
        toStringEnd(&str);
        return c + str;
    }
    else
    {
        return c + finishLoadNonString();
    }
}


// Load until end of value: BEFORE whitespace or ',' or ']' or '}'.
// @return the loaded data.
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


// Advance nvram offset to byte after whitespace.
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

// @return true iff c is whitespace.
bool Json::isws(char c)
{
    return (c == ' ') || (c == '\t') || (c == '\n') || (c == '\r');
}


void Json::ContextStack::init()
{
    m_index = 0;
    m_stack[m_index].m_type = OBJECT;
    m_stack[m_index].m_isFirstMember = true;
}

void Json::ContextStack::push(ValueType t)
{
    m_index++;
    assert(m_index < STACK_DEPTH);
    m_stack[m_index].m_type = t;
    m_stack[m_index].m_isFirstMember = true;
}

void Json::ContextStack::pop()
{
    assert(m_index > 0);
    m_index--;
}

}
