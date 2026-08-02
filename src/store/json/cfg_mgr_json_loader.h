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
    JsonLoader(Nvram_itf * pNvram);
    ~JsonLoader();

    Result startLoad();
    Result endLoad();
    Result startLoadSimple(const char * name);
    Result endLoadSimple(item_len_t * length, uint8_t * pRam, JSON_SET_FPTR set);
    Result startLoadObject(const char * name);
    Result endLoadObject();
    Result startLoadArray(const char * name);
    Result endLoadArray();

private:
    Result startLoadComposite(const char * name, ValueType t);
    Result endLoadComposite(ValueType t);
    Result startLoadMember(const char * name);
    Result findName(const char * name);
    Result toNextName();
    Result toCloser(char open, char close);
    Result toStringEnd(std::string *str = nullptr);
    Result readName(const char * name);
    std::string loadValue();
    std::string finishLoadNonString();
    bool isNextRead(const char * b, unsigned len);
    bool skipws();
    bool isws(char c);

    ContextStack m_context;
    Nvram_itf * m_nvram;
};
}
