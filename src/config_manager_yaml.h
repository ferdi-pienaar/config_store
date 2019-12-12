#ifndef CFG_MAN_YAML_H
#define CFG_MAN_YAML_H

#include <stdint.h> // uint8_t, etc
#include <iostream>
#include "nvram.h"
#include "config_manager_types.h"

namespace cfg_mgr
{

// Function pointer to convert a string into data in RAM
typedef bool (*YAML_SET_FPTR)(uint8_t *pItem, item_len_t len, std::string val);
// Function pointer to convert data in RAM into a string
typedef void (*YAML_PRT_FPTR)(FILE * f, const uint8_t *pItem, item_len_t len);

class Yaml
{
public:
    static const unsigned int stackDepth = 8;

    Yaml(Nvram * pNvram);
    ~Yaml();

    void reset();
    void writeSimple(const char * name, item_len_t length, const uint8_t * v, YAML_PRT_FPTR prt);
    void startWriteComposite(const char * name);
    void endWriteComposite();
    void startWriteList(const char * name);
    void endWriteList();
    result_t startLoadSimple(const char * name);
    result_t endLoadSimple(item_len_t * length, uint8_t * pRam, YAML_SET_FPTR set);
    result_t startLoadComposite(const char * name);
    result_t endLoadComposite();

private:
    // Context used in writing
    struct CompositeWriteContext
    {
        unsigned      headerOffset; // offset of location of composite's T + L, relative to base of NVRAM
        item_id_t  id;           // composite ID given by client
        item_len_t length;       // actual cumulative length in composite
    };

    struct CompositeLoadContext
    {
        item_len_t length;       // length [bytes] in composite, read from NVRAM
        item_len_t readBytes;    // number of composite bytes read from MVRAM
    };

    void write_indent();

    result_t updateContainer(item_len_t length, unsigned * complete);
    Nvram *  m_nvram;
    int m_stackIndex;  // write stack index; -1 means the current item is top-level, not part of a composite
    CompositeWriteContext m_writeStack[stackDepth];
    CompositeLoadContext  m_loadStack[stackDepth];
};
}
#endif // CFG_MAN_YAML_H

