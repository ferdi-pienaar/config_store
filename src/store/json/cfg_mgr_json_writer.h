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

class JsonWriter
{
public:
    JsonWriter(Nvram * pNvram);
    ~JsonWriter();

    void startWrite();
    void endWrite();
    void writeSimple(const char * name, item_len_t length, const uint8_t * v, JSON_PRT_FPTR prt);
    void startWriteObject(const char * name);
    void endWriteObject();
    void startWriteArray(const char * name);
    void endWriteArray();

private:
    void startWriteComposite(const char * name, ValueType t);
    void endWriteComposite(ValueType t);
    void startWriteMember(const char * name);
    void writeEndPrecedingLine();
    void writeIndent();

    ContextStack m_context;
    std::string m_singleIndent;
    Nvram * m_nvram;
};
}
