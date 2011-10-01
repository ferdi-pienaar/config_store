#ifndef CFG_MAN_STORE_H
#define CFG_MAN_STORE_H

#include <stdint.h> // uint8_t, etc
#include "config_manager_tlv.h"
#include "config_manager_yaml.h"
#include "config_manager_metadata.h"
#include "nvram.h"


// xxx move this to separate file
class cm_store
{
public:
    virtual ~cm_store() {}
    virtual bool resetRead() = 0;
    virtual bool resetWrite() = 0;
    virtual void writeSimple(const cm_simple_metadata * data, const uint8_t * v) = 0;
    virtual void startWriteComposite(const cm_composite_metadata * data) = 0;
    virtual void endWriteComposite() = 0;
    virtual t_cm_result getType(cm_item_id_t * t) = 0;
    virtual t_cm_result loadSimple(uint8_t * pRam, const cm_simple_metadata * data, unsigned * complete) = 0;
    virtual t_cm_result loadComposite() = 0;
    virtual void skipItem(unsigned * complete) = 0;

protected:
    Nvram  nvram;

};


// xxx move this to separate file
class cm_tlv_store : public cm_store
{
public:
    cm_tlv_store();
    ~cm_tlv_store();

    bool resetRead();
    bool resetWrite();
    void writeSimple(const cm_simple_metadata * data, const uint8_t * v);
    void startWriteComposite(const cm_composite_metadata * data);
    void endWriteComposite();
    t_cm_result getType(cm_item_id_t * t);
    t_cm_result loadSimple(uint8_t * pRam, const cm_simple_metadata * data, unsigned * complete);
    t_cm_result loadComposite();
    void skipItem(unsigned * complete);

private:
    Tlv * tlv;

};


// xxx move this to separate file
class cm_yaml_store : public cm_store
{
public:
    cm_yaml_store();
    ~cm_yaml_store();

    bool resetRead();
    bool resetWrite();
    void writeSimple(const cm_simple_metadata * data, const uint8_t * v);
    void startWriteComposite(const cm_composite_metadata * data);
    void endWriteComposite();
    t_cm_result getType(cm_item_id_t * t);
    t_cm_result loadSimple(uint8_t * pRam, const cm_simple_metadata * data, unsigned * complete);
    t_cm_result loadComposite();
    void skipItem(unsigned * complete);

private:
    Yaml * yaml;

};

#endif // CFG_MAN_STORE_H

