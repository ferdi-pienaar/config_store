#ifndef CFG_MAN_JSON_H
#define CFG_MAN_JSON_H

#include <stdint.h> // uint8_t, etc
#include <iostream>
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
    static const unsigned int stackDepth = 8;

    Json(Nvram * pNvram);
    ~Json();

    void reset();
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
    void closePredecessorLine();
    Nvram * m_nvram;
    std::string indent;
    struct WriteContext
    {
        WriteContext(): isFirstMember(true), isInArray(false) {}
        bool isFirstMember; // Are we writing the first member of a list or composite?
        bool isInArray; // Are we writing a list? xxx I think we'll need multiple copies -- we could be in an array inside an array.
    };
    WriteContext m_writeContext;
};
}
#endif // CFG_MAN_JSON_H

