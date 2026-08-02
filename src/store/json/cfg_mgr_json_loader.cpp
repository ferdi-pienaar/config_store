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

JsonLoader::JsonLoader(Nvram_itf * pNvram): m_nvram(pNvram)
{
}


JsonLoader::~JsonLoader()
{
}

Result JsonLoader::startLoad()
{
    skipws();
    if (!isNextRead("{", 1))
    {
        return Result::CM_INCOHERENT_DATA;
    }
    DBG_PRT("%s OK\n", __PRETTY_FUNCTION__);
    m_context.init();
    return Result::CM_SUCCESS;
}

Result JsonLoader::endLoad()
{
    skipws();
    if (!isNextRead("}", 1))
    {
        return Result::CM_INCOHERENT_DATA;
    }
    DBG_PRT("%s OK\n", __PRETTY_FUNCTION__);
    return Result::CM_SUCCESS;
}

// Return success if name (in quotes) followed by ':' is next.
Result JsonLoader::startLoadSimple(const char * name)
{
    DBG_PRT("%s name=%s stackIndex=%u\n", __PRETTY_FUNCTION__, name, m_context.getIndex());
    return startLoadMember(name);
}

// Read the value from JSON in nvram, and convert to value in RAM if set function.
// @param length - output.
Result JsonLoader::endLoadSimple(item_len_t * length, uint8_t * pRam, JSON_SET_FPTR set)
{
    DBG_PRT("%s m_stackIndex=%u\n", __PRETTY_FUNCTION__, m_context.getIndex());

    string valstr = loadValue();
    DBG_PRT("%s value='%s'\n", __PRETTY_FUNCTION__, valstr.c_str());
    set(pRam, *length, valstr);
    m_context.setIsFirstMember(false);
    return Result::CM_SUCCESS;
}

Result JsonLoader::startLoadObject(const char * name)
{
    return startLoadComposite(name, OBJECT);
}

Result JsonLoader::endLoadObject()
{
    return endLoadComposite(OBJECT);
}

Result JsonLoader::startLoadArray(const char * name)
{
    return startLoadComposite(name, ARRAY);
}

Result JsonLoader::endLoadArray()
{
    return endLoadComposite(ARRAY);
}

// Start loading of Object or Array.
Result JsonLoader::startLoadComposite(const char * name, ValueType t)
{
    DBG_PRT("%s name=%s type=%d stackIndex=%u\n", __PRETTY_FUNCTION__, name, t, m_context.getIndex());

    Result ret = startLoadMember(name);
    if (ret != Result::CM_SUCCESS)
    {
        return ret;
    }
    if (!isNextRead((t == OBJECT) ? "{" : "[", 1))
    {
        DBG_PRT("%s: %s '%s' missing opening '%s'\n", __PRETTY_FUNCTION__,
                (t == OBJECT) ? "object" : "array",
                name,
                (t == OBJECT) ? "{" : "[");
        return Result::CM_INCOHERENT_DATA;
    }
    m_context.push(t);
    return Result::CM_SUCCESS;
}

// End loading of Object or Array.
Result JsonLoader::endLoadComposite(ValueType t)
{
    DBG_PRT("%s type=%d, m_stackIndex=%u\n", __PRETTY_FUNCTION__, t, m_context.getIndex());

    skipws();
    if (!isNextRead((t == OBJECT) ? "}" : "]", 1))
    {
        DBG_PRT("%s missing '%s'\n", __PRETTY_FUNCTION__, (t == OBJECT) ? "}" : "]");
        return Result::CM_INCOHERENT_DATA;
    }
    m_context.pop();
    m_context.setIsFirstMember(false);
    return Result::CM_SUCCESS;
}

// Common start for simple, object, and array.
Result JsonLoader::startLoadMember(const char * name)
{
    skipws();
    if (!m_context.isFirstMember())
    {
        // This is subsequent member, so expect ',' before its name.
        if (!isNextRead(",", 1))
        {
            DBG_PRT("%s missing ','\n", __PRETTY_FUNCTION__);
            return Result::CM_NOT_FOUND;
        }
        skipws();
    }
    if (m_context.getType() == OBJECT)
    {
        // In an object, expect the member's name (but not in an array).
        Result ret = findName(name);
        if (ret != Result::CM_SUCCESS)
        {
            return ret;
        }
        skipws();
    }
    return Result::CM_SUCCESS;
}

// Find name within the current object.
// If not found, leave nvram offset unchanged so next find starts at same offset.
Result JsonLoader::findName(const char * name)
{
    unsigned int start_offset = m_nvram->getOffset();
    while (true)
    {
        Result res = readName(name);
        if (res == Result::CM_SUCCESS)
        {
            return res;
        }
        if (res != Result::CM_NOT_FOUND)
        {
            // Error: it doesn't even look like a name, e.g. no opening or closing quotes.
            return res;
        }
        // readName returned CM_NOT_FOUND (the expected name isn't there), so go to next name.
        res = toNextName();
        if (res != Result::CM_SUCCESS)
        {
            // There are no more names in the current context, or some other error.
            m_nvram->setOffset(start_offset);
            return res;
        }
    }
    return Result::CM_SUCCESS;
}

// Advance nvram to name within the current context.
// @pre nvram is after the ':' that follows a name within the current context.
// @post
Result JsonLoader::toNextName()
{
    Result ret = Result::CM_SUCCESS;
    skipws();
    char c;
    if (!m_nvram->read((uint8_t *)&c, 1))
    {
        return Result::CM_INCOHERENT_DATA;
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
    if (ret !=Result::CM_SUCCESS)
    {
        DBG_PRT("%s unterminated '%c'\n", __PRETTY_FUNCTION__, c);
        return ret;
    }
    if (toCloser(0, ',') != Result::CM_SUCCESS)
    {
        DBG_PRT("%s no ','\n", __PRETTY_FUNCTION__);
        return Result::CM_NOT_FOUND;
    }
    skipws();
    return ret;
}


// Move NVRAM offset to matching closing marker.
// If we meet 'open's, they must be closed by 'close's,
// then the next 'close' represents the match we're looking for.
// Ignore opening and closing markers that appear inside strings.
// It seems this basic parsing is sufficient.
Result JsonLoader::toCloser(char open, char close)
{
    unsigned depth = 0; // number of opens that are not closed.
    char c;
    while (m_nvram->read((uint8_t *)&c, 1))
    {
        if (c == close)
        {
            if (depth == 0)
            {
                return Result::CM_SUCCESS;
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
    return Result::CM_NOT_FOUND;
}

// Advance nvram offset to character after closing quote of string.
// @param - str in/out, append found characters to this string, if provided -- includes closing quote.
// @return CM_SUCCESS, or CM_NOT_FOUND if NVRAM ends before we reach end of string.
// @pre - opening quote of string has been read.
Result JsonLoader::toStringEnd(string *str)
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
            return Result::CM_SUCCESS;
        }
        escape = (c == '\\');
    }
    return Result::CM_NOT_FOUND;
}


// @return CM_SUCCESS if name, surrounded by quotes and followed by ':' is next in nvram.
//         CM_NOT_FOUND if a name is found, but not the name that we expected.
//         CM_INCOHERENT_DATA if next in NVRAM is not a quoted string followed by ":".
// @post if there is a name, nvram advanced to after following ":".
Result JsonLoader::readName(const char * name)
{
    if (!isNextRead("\"", 1))
    {
        DBG_PRT("%s missing open\n", __PRETTY_FUNCTION__);
        return Result::CM_INCOHERENT_DATA;
    }

    string foundName;
    Result res = toStringEnd(&foundName);
    if (res != Result::CM_SUCCESS)
    {
        DBG_PRT("%s name string unclosed.\n", __PRETTY_FUNCTION__);
        return Result::CM_INCOHERENT_DATA;
    }
    size_t name_len = strlen(name);
    size_t found_len = foundName.length() - 1; // subtract 1 because foundName includes closing quote.
    if ((name_len != found_len) || (strncmp(foundName.c_str(), name, name_len) != 0))
    {
        // Wrong name, but we don't return, continue to check the format.
        DBG_PRT("%s '%s' doesn't match '%s'.\n", __PRETTY_FUNCTION__, foundName.c_str(), name);
        res = Result::CM_NOT_FOUND;
    }
    skipws();
    if (!isNextRead(":", 1))
    {
        DBG_PRT("%s missing ':'\n", __PRETTY_FUNCTION__);
        return Result::CM_INCOHERENT_DATA;
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
