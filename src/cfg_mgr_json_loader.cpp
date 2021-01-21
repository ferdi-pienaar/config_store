///
// JSON format config data, for storage in non-volatile media, e.g. a file.
// This is defined as a separate class hierarchy, since it's an optional feature:
// not all managed objects are saved in NVRAM.
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

#include "cfg_mgr_json_loader.h"
#include "cfg_mgr_dbg.h"
#include <cstring> // strlen

using namespace std;

namespace cfg_mgr
{

JsonLoader::JsonLoader(Nvram * pNvram): m_nvram(pNvram)
{
}


JsonLoader::~JsonLoader()
{
}

result_t JsonLoader::startLoad()
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

result_t JsonLoader::endLoad()
{
    skipws();
    if (!isNextRead("}", 1))
    {
        return CM_INCOHERENT_DATA;
    }
    DBG_PRT("%s OK\n", __PRETTY_FUNCTION__);
    return CM_SUCCESS;
}

// Return success if name (in quotes) followed by ':' is next.
result_t JsonLoader::startLoadSimple(const char * name)
{
    DBG_PRT("%s name=%s stackIndex=%u\n", __PRETTY_FUNCTION__, name, m_context.getIndex());
    return startLoadMember(name);
}

// Read the value from JSON in nvram, and convert to value in RAM if set function.
// @param length - output.
result_t JsonLoader::endLoadSimple(item_len_t * length, uint8_t * pRam, JSON_SET_FPTR set)
{
    DBG_PRT("%s m_stackIndex=%u\n", __PRETTY_FUNCTION__, m_context.getIndex());

    string valstr = loadValue();
    DBG_PRT("%s value='%s'\n", __PRETTY_FUNCTION__, valstr.c_str());
    set(pRam, *length, valstr);
    m_context.setIsFirstMember(false);
    return CM_SUCCESS;
}

result_t JsonLoader::startLoadObject(const char * name)
{
    return startLoadComposite(name, OBJECT);
}

result_t JsonLoader::endLoadObject()
{
    return endLoadComposite(OBJECT);
}

result_t JsonLoader::startLoadArray(const char * name)
{
    return startLoadComposite(name, ARRAY);
}

result_t JsonLoader::endLoadArray()
{
    return endLoadComposite(ARRAY);
}

// Start loading of Object or Array.
result_t JsonLoader::startLoadComposite(const char * name, ValueType t)
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
result_t JsonLoader::endLoadComposite(ValueType t)
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

// Common start for simple, object, and array.
result_t JsonLoader::startLoadMember(const char * name)
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
result_t JsonLoader::findName(const char * name)
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
        // readName returned CM_NOT_FOUND (the expected name isn't there), so go to next name.
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
result_t JsonLoader::toNextName()
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
result_t JsonLoader::toCloser(char open, char close)
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
// @param - str in/out, append found characters to this string, if provided -- includes closing quote.
// @return CM_SUCCESS, or CM_NOT_FOUND if NVRAM ends before we reach end of string.
// @pre - opening quote of string has been read.
result_t JsonLoader::toStringEnd(string *str)
{
    bool escape = false; // are we in escape state because previous char was '\'?
    char c;
    while (m_nvram->read((uint8_t *)&c, 1))
    {
        if (str != nullptr)
        {
            // An input string was provided, so append characters.
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


// @return CM_SUCCESS if name, surrounded by quotes and followed by ':' is next in nvram.
//         CM_NOT_FOUND if a name is found, but not the name that we expected.
//         CM_INCOHERENT_DATA if next in NVRAM is not a quoted string followed by ":".
// @post if there is a name, nvram advanced to after following ":".
result_t JsonLoader::readName(const char * name)
{
    if (!isNextRead("\"", 1))
    {
        DBG_PRT("%s missing open\n", __PRETTY_FUNCTION__);
        return CM_INCOHERENT_DATA;
    }

    string foundName;
    result_t res = toStringEnd(&foundName);
    if (res != CM_SUCCESS)
    {
        DBG_PRT("%s name string unclosed.\n", __PRETTY_FUNCTION__);
        return CM_INCOHERENT_DATA;
    }
    size_t name_len = strlen(name);
    size_t found_len = foundName.length() - 1; // subtract 1 because foundName includes closing quote.
    if ((name_len != found_len) || (strncmp(foundName.c_str(), name, name_len) != 0))
    {
        // Wrong name, but we don't return, continue to check the format.
        DBG_PRT("%s '%s' doesn't match '%s'.\n", __PRETTY_FUNCTION__, foundName.c_str(), name);
        res = CM_NOT_FOUND;
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
bool JsonLoader::isNextRead(const char * expect, unsigned len)
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
string JsonLoader::loadValue()
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
    return c + finishLoadNonString();
}


// Load until end of value: BEFORE whitespace or ',' or ']' or '}'.
// @return the loaded data.
string JsonLoader::finishLoadNonString()
{
    string str;
    char c;
    while (m_nvram->read((uint8_t *)&c, 1))
    {
        if (isws(c) || (c == ',') || (c == ']') || (c == '}'))
        {
            // This character is not part of the value, so move offset back.
            m_nvram->adjustOffset(-1);
            break;
        }
        str += c;
    }
    return str;
}


// Advance nvram offset to byte after whitespace.
bool JsonLoader::skipws()
{
    char candidate;
    while (m_nvram->read((uint8_t *)&candidate, 1))
    {
        if (!isws(candidate))
        {
            // This character is not whitespace, so move offset back.
            m_nvram->adjustOffset(-1);
            return true;
        }
    }
    // reached end of nvram or some other read fail.
    return false;
}

// @return true iff c is whitespace.
bool JsonLoader::isws(char c)
{
    return (c == ' ') || (c == '\t') || (c == '\n') || (c == '\r');
}

}
