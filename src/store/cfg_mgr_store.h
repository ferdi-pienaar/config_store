#pragma once

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
    Store(Nvram_itf * nvram) : m_nvram(nvram) {}
    virtual ~Store() {}
    virtual bool startWrite() const;
    virtual void endWrite() const;
    virtual result_t startLoad() const;
    virtual result_t endLoad() const;
    virtual void writeSimple(const Simple_metadata * data, const uint8_t * v) const = 0;
    virtual void startWriteComposite(const Composite_metadata * data) const = 0;
    virtual void endWriteComposite() const = 0;
    virtual void startWriteArray(const char * name) const {}
    virtual void endWriteArray() const {}
    virtual result_t startLoadSimple(const Simple_metadata * data) const = 0;
    virtual result_t endLoadSimple(uint8_t * pRam, const Simple_metadata * data) const = 0;
    virtual result_t startLoadComposite(const Composite_metadata * data) const = 0;
    virtual result_t endLoadComposite() const  = 0;
    virtual result_t startLoadArray(const char * name) const
    {
        return CM_SUCCESS;
    }
    virtual result_t endLoadArray() const
    {
        return CM_SUCCESS;
    }
    static Store * createStore(Nvram_itf * nvram);

protected:
    Nvram_itf * m_nvram;

};
}
