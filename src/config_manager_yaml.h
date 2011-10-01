#ifndef CFG_MAN_YAML_H
#define CFG_MAN_YAML_H

#include <stdint.h> // uint8_t, etc
#include "nvram.h"
#include "config_manager_types.h"

// Function pointer to convert a string into data in RAM
typedef bool (*YAML_SET_FPTR)(FILE * f, uint8_t *pItem, cm_item_len_t len, std::string val);
// Function pointer to convert data in RAM into a string
typedef void (*YAML_PRT_FPTR)(FILE * f, const uint8_t *pItem, cm_item_len_t len);

class Yaml
{
public:
    static const unsigned int stackDepth = 8;

    Yaml(Nvram * pNvram);
    ~Yaml();

    void reset();
    void writeSimple(const char * name, cm_item_id_t t, const uint8_t * v, cm_item_len_t length, YAML_PRT_FPTR prt);
    void startWriteComposite(const char * name, cm_item_id_t t);
    void endWriteComposite();
    t_cm_result getType(cm_item_id_t * t);
    t_cm_result loadSimple(uint8_t * pRam, cm_item_len_t * length, unsigned * complete);
    t_cm_result loadComposite();
    void skipItem(unsigned * complete);

private:
    // Context used in writing
    struct compositeWriteContext
    {
        unsigned      headerOffset; // offset of location of composite's T + L, relative to base of NVRAM
        cm_item_id_t  id;           // composite ID given by client
        cm_item_len_t length;       // actual cumulative length in composite
    };

    struct compositeLoadContext
    {
        cm_item_len_t length;       // length [bytes] in composite, read from NVRAM
        cm_item_len_t readBytes;    // number of composite bytes read from MVRAM
    };


    t_cm_result updateContainer(cm_item_len_t length, unsigned * complete);
    Nvram *  nvram;
    int stackIndex;  // write stack index; -1 means the current item is top-level, not part of a composite
    compositeWriteContext writeStack[stackDepth];
    compositeLoadContext  loadStack[stackDepth];

};

#endif // CFG_MAN_YAML_H

