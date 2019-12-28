#ifndef CFG_MAN_JSON_H
#define CFG_MAN_JSON_H

#include <stdint.h> // uint8_t, etc
#include <string>
#include "nvram.h"
#include "config_manager_types.h"

namespace cfg_mgr
{

// Function pointer to convert a string into data in RAM.
typedef bool (*JSON_SET_FPTR)(uint8_t *pItem, item_len_t len, std::string val);
// Function pointer to convert data in RAM into a string
typedef std::string (*JSON_PRT_FPTR)(const uint8_t *pItem, item_len_t len);

class Json
{
public:
    static const unsigned int STACK_DEPTH = 16;

    Json(Nvram * pNvram);
    ~Json();

    void startWrite();
    void endWrite();
    void writeSimple(const char * name, item_len_t length, const uint8_t * v, JSON_PRT_FPTR prt);
    void startWriteComposite(const char * name);
    void endWriteComposite();
    void startWriteArray(const char * name);
    void endWriteArray();
    result_t startLoadSimple(const char * name);
    result_t endLoadSimple(item_len_t * length, uint8_t * pRam, JSON_SET_FPTR set);
    result_t startLoadComposite(const char * name);
    result_t endLoadComposite();

private:
    void writeName(const char * name);
    void writeEndPrecedingLine();
    void writeIndent();
    bool loadName(const char * name);
    std::string loadValue();
    std::string finishLoadString();
    std::string finishLoadNonString();
    bool isNextLoad(const char * b, unsigned len);
    bool skipws();
    bool isws(char c);

    enum ValueType
    {
        OBJECT,
        ARRAY
    };

    struct WriteContext
    {
        WriteContext(): m_isFirstMember(true) {}

        ValueType m_type;
        bool m_isFirstMember; // Are we writing the first member of a list or composite?
    };

    struct LoadContext
    {
        LoadContext(): m_isFirstMember(true) {}

        ValueType m_type;
        bool m_isFirstMember; // Are we writing the first member of a list or composite?
    };
    std::string m_singleIndent;
    WriteContext m_writeContext[STACK_DEPTH];
    LoadContext m_loadContext[STACK_DEPTH];
    unsigned m_stackIndex;
    Nvram * m_nvram;
};
}
#endif // CFG_MAN_JSON_H

