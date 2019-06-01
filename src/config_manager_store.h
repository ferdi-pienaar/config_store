#ifndef CFG_MAN_STORE_H
#define CFG_MAN_STORE_H

#include <stdint.h> // uint8_t, etc
#include "config_manager_metadata.h"
#include "nvram.h"


// Abstract interface to classes that give access to persistent storage.
// It's a Strategy pattern's (abstract) strategy class, with two ConcreteStrategies.
// The static method getStore returns an instance of a concrete sub-class.
// (_Head First Design Patterns_ calls this a "static factory" -- it's the equivalent
// of putting a static createPizza() method in that book's abstract Pizza class.)
// See also my notes for "Refactoring: Improving the Design of Existing Code".
class cm_store
{
public:
    virtual ~cm_store() {}
    virtual bool initForRead() = 0;
    virtual bool initForWrite() = 0;
    virtual void writeSimple(const cm_simple_metadata * data, const uint8_t * v) = 0;
    virtual void startWriteComposite(const cm_composite_metadata * data) = 0;
    virtual void endWriteComposite() = 0;
    virtual t_cm_result loadSimple(uint8_t * pRam, const cm_common_metadata * data) = 0;
    virtual t_cm_result startLoadComposite(const cm_common_metadata * data) = 0;
    virtual t_cm_result endLoadComposite() = 0;
    static cm_store * getStore();

protected:
    Nvram  nvram;

};

#endif // CFG_MAN_STORE_H

