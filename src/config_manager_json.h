#ifndef CFG_MAN_JSON_H
#define CFG_MAN_JSON_H

#include <stdint.h> // uint8_t, etc
#include <iostream>
#include "nvram.h"
#include "config_manager_types.h"

// Function pointer to convert a string into data in RAM.
typedef bool (*JSON_SET_FPTR)(uint8_t *pItem, cm_item_len_t len, std::string val);
// Function pointer to convert data in RAM into a string
typedef std::string (*JSON_PRT_FPTR)(const uint8_t *pItem, cm_item_len_t len);

class Json
{
public:
    static const unsigned int stackDepth = 8;

    Json(Nvram * pNvram);
    ~Json();

    void reset();
    void writeSimple(const char * name, cm_item_len_t length, const uint8_t * v, JSON_PRT_FPTR prt);
    void startWriteComposite(const char * name);
    void endWriteComposite();
    void startWriteList(const char * name);
    void endWriteList();
    t_cm_result startLoadSimple(const char * name);
    t_cm_result endLoadSimple(cm_item_len_t * length, uint8_t * pRam, JSON_SET_FPTR set);
    t_cm_result startLoadComposite(const char * name);
    t_cm_result endLoadComposite();

private:
    void writeName(const char * name);
    void closePreviousLine();
    Nvram *  nvram;
    std::string indent;
    bool firstMember;
};

#endif // CFG_MAN_JSON_H

