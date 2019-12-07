///
//

#include <stdint.h> // uint8_t, etc
#include "config_manager_yaml_store.h"
#include "config_manager.h"
#include "config_manager_dbg.h"
#include "nvram.h"
#include <assert.h>

using namespace std;

namespace cfg_mgr
{

Yaml_store::Yaml_store()
{
    cout << "Yaml_store::Yaml_store()" << endl;

    yaml = new Yaml(&nvram);
}


Yaml_store::~Yaml_store()
{
    cout << "Yaml_store::~Yaml_store()" << endl;

    delete yaml;
}


bool Yaml_store::initForRead()
{
    cout << "Yaml_store::resetRead()" << endl;

    yaml->reset();
    return nvram.initForRead();
}


bool Yaml_store::initForWrite()
{
    cout << "Yaml_store::resetWrite()" << endl;

    yaml->reset();
    return nvram.initForWrite();
}


void Yaml_store::writeSimple(const Simple_metadata * data, const uint8_t * v)
{
    yaml->writeSimple(data->c.name, data->c.len, v, data->pPrt);
}


void Yaml_store::startWriteComposite(const Composite_metadata * data)
{
    yaml->startWriteComposite(data->c.name);
}


void Yaml_store::endWriteComposite()
{
    yaml->endWriteComposite();
}

result_t Yaml_store::startLoadSimple(const Simple_metadata * data)
{
    cout << __PRETTY_FUNCTION__ << endl;
    return yaml->startLoadSimple(data->c.name);
}

result_t Yaml_store::endLoadSimple(uint8_t * pRam, const Simple_metadata * data)
{
    cout << __PRETTY_FUNCTION__ << endl;
    item_len_t len = data->c.len;
    return yaml->endLoadSimple(&len, pRam, data->pSet);
}

// xxx incomplete
result_t Yaml_store::startLoadComposite(const Composite_metadata * data)
{
    cout << "Yaml_store::startLoadComposite()" << endl;

    return yaml->startLoadComposite(data->c.name);
}

result_t Yaml_store::endLoadComposite()
{
    cout << "Yaml_store::endLoadComposite()" << endl;
    return yaml->endLoadComposite();
}

}
