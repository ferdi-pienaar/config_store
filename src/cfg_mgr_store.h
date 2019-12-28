#ifndef CFG_MGR_STORE_H
#define CFG_MGR_STORE_H

#include <stdint.h> // uint8_t, etc
#include "cfg_mgr_metadata.h"
#include "nvram.h"

namespace cfg_mgr
{
// Abstract interface to classes that give access to persistent storage.
// It's a Strategy pattern's (abstract) strategy class, with two ConcreteStrategies.
// The static method getStore returns an instance of a concrete sub-class.
// (_Head First Design Patterns_ calls this a "static factory" -- it's the equivalent
// of putting a static createPizza() method in that book's abstract Pizza class.)
// See also my notes for "Refactoring: Improving the Design of Existing Code".
class Store
{
public:
    Store(Nvram * nvram) : m_nvram(nvram) {}
    virtual ~Store() {}
    virtual bool startWrite();
    virtual void endWrite();
    virtual bool startLoad();
    virtual void endLoad();
    virtual void writeSimple(const Simple_metadata * data, const uint8_t * v) = 0;
    virtual void startWriteComposite(const Composite_metadata * data) = 0;
    virtual void endWriteComposite() = 0;
    virtual void startWriteArray(const char * name) {}
    virtual void endWriteArray() {}
    virtual result_t startLoadSimple(const Simple_metadata * data) = 0;
    virtual result_t endLoadSimple(uint8_t * pRam, const Simple_metadata * data) = 0;
    virtual result_t startLoadComposite(const Composite_metadata * data) = 0;
    virtual result_t endLoadComposite() = 0;
    static Store * createStore(Nvram * nvram);

protected:
    Nvram * m_nvram;

};
}
#endif // CFG_MGR_STORE_H

