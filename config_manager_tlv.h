#ifndef CFG_MAN_TLV_H
#define CFG_MAN_TLV_H

#include <stdint.h> // uint8_t, etc
#include "config_manager_types.h"
#include "config_manager.h"

class cm_descriptor;

// Base class representing interaction with NVRAM where TLV is saved.
class cm_tlv
{
protected:
    const cm_descriptor * pDesc; // owner reference, passed to constructor
    
public:
    cm_tlv(const cm_descriptor * desc): pDesc(desc) {}
    virtual ~cm_tlv() {}

    virtual cm_item_len_t getLen(const uint8_t * pItem) const = 0;
    virtual void write(const uint8_t * pItem, uint8_t ** ppBuf) const = 0;
    virtual unsigned int load(FILE * fp, uint8_t * pItem) const = 0;

};


//
class cm_composite_tlv : public cm_tlv
{

public:
    cm_composite_tlv(const cm_descriptor * desc): cm_tlv(desc) {}

    virtual cm_item_len_t getLen(const uint8_t * pItem) const;
    virtual void write(const uint8_t * pItem, uint8_t ** ppBuf) const;
    virtual unsigned int load(FILE * fp, uint8_t * pItem) const;

private:
    unsigned int skipItem(FILE * fp) const;

};


//
class cm_simple_tlv : public cm_tlv
{

public:
    cm_simple_tlv(const cm_descriptor * desc): cm_tlv(desc) {}

    virtual cm_item_len_t getLen(const uint8_t * pItem) const;
    virtual void write(const uint8_t * pItem, uint8_t ** ppBuf) const;
    virtual unsigned int load(FILE * fp, uint8_t * pItem) const;

};

#endif // CFG_MAN_TLV_H

