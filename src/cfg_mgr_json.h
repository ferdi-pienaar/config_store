#ifndef CFG_MGR_JSON_H
#define CFG_MGR_JSON_H

#include <stdint.h> // uint8_t, etc
#include <string>
#include "nvram.h"
#include "cfg_mgr_types.h"

namespace cfg_mgr
{

// Function pointer to convert a string into data in RAM.
typedef bool (*JSON_SET_FPTR)(uint8_t *pItem, item_len_t len, std::string val);
// Function pointer to convert data in RAM into a string
typedef std::string (*JSON_PRT_FPTR)(const uint8_t *pItem, item_len_t len);

class Json
{
public:
    Json(Nvram * pNvram);
    ~Json();

    void startWrite();
    void endWrite();
    result_t startLoad();
    result_t endLoad();
    void writeSimple(const char * name, item_len_t length, const uint8_t * v, JSON_PRT_FPTR prt);
    void startWriteComposite(const char * name);
    void endWriteComposite();
    void startWriteArray(const char * name);
    void endWriteArray();
    result_t startLoadSimple(const char * name);
    result_t endLoadSimple(item_len_t * length, uint8_t * pRam, JSON_SET_FPTR set);
    result_t startLoadComposite(const char * name);
    result_t endLoadComposite();
    result_t startLoadArray(const char * name);
    result_t endLoadArray();

private:
    void startWriteMember(const char * name);
    void writeEndPrecedingLine();
    void writeIndent();
    result_t startLoadMember(const char * name);
    result_t findName(const char * name);
    result_t toNextName();
    result_t toCloser(char open, char close);
    result_t toStringEnd(std::string *str = nullptr);
    result_t readName(const char * name);
    std::string loadValue();
    std::string finishLoadNonString();
    bool isNextRead(const char * b, unsigned len);
    bool skipws();
    bool isws(char c);

    // Stack used in writing and loading.
    class ContextStack
    {
    public:
        enum ValueType
        {
            OBJECT,
            ARRAY
        };

        ContextStack()
        {
            init();
        }

        void init();
        void push(ValueType t);
        void pop();

        ValueType getType()
        {
            return m_stack[m_index].m_type;
        }

        void setIsFirstMember(bool first)
        {
            m_stack[m_index].m_isFirstMember = first;
        }

        bool isFirstMember()
        {
            return m_stack[m_index].m_isFirstMember;
        }

        unsigned getIndex()
        {
            return m_index;
        }

    private:
        static const unsigned int STACK_DEPTH = 16;

        // Context maintained for reading/writing a Value that is an Object or Array.
        struct Context
        {
            Context(): m_type(OBJECT), m_isFirstMember(true) {}

            ValueType m_type;
            bool m_isFirstMember; // Are we writing/loading the first member of a list or composite?
        };

        Context m_stack[STACK_DEPTH];
        unsigned m_index;
    };

    ContextStack m_context;
    std::string m_singleIndent;
    Nvram * m_nvram;
};
}
#endif // CFG_MGR_JSON_H

