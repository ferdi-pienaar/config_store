#pragma once

#include <stdint.h> // uint8_t, etc
#include <string>
#include "store/nvram.h"
#include "cfg_mgr_types.h"
#include "store/json/cfg_mgr_json_ctxt.h"
#include "store/json/cfg_mgr_json_common.h"

namespace cfg_mgr
{

// Function pointer to convert a string into data in RAM.
typedef bool (*JSON_SET_FPTR)(uint8_t *pItem, item_len_t len, std::string val);
// Function pointer to convert data in RAM into a string
typedef std::string (*JSON_PRT_FPTR)(const uint8_t *pItem, item_len_t len);

class JsonLoader
{
public:
    JsonLoader(Nvram * pNvram);
    ~JsonLoader();

    result_t startLoad();
    result_t endLoad();
    result_t startLoadSimple(const char * name);
    result_t endLoadSimple(item_len_t * length, uint8_t * pRam, JSON_SET_FPTR set);
    result_t startLoadObject(const char * name);
    result_t endLoadObject();
    result_t startLoadArray(const char * name);
    result_t endLoadArray();

private:
    result_t startLoadComposite(const char * name, ValueType t);
    result_t endLoadComposite(ValueType t);
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

    ContextStack m_context;
    Nvram * m_nvram;
};
}
