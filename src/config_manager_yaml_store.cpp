///
//

#include <stdint.h> // uint8_t, etc
#include "config_manager_yaml_store.h"
#include "config_manager.h"
#include "config_manager_dbg.h"
#include "nvram.h"
#include <assert.h>

using namespace std;

cm_yaml_store::cm_yaml_store()
{
    cout << "cm_yaml_store::cm_yaml_store()" << endl;

    yaml = new Yaml(&nvram);
}


cm_yaml_store::~cm_yaml_store()
{
    cout << "cm_yaml_store::~cm_yaml_store()" << endl;

    delete yaml;
}


bool cm_yaml_store::initForRead()
{
    cout << "cm_yaml_store::resetRead()" << endl;

    yaml->reset();
    return nvram.initForRead();
}


bool cm_yaml_store::initForWrite()
{
    cout << "cm_yaml_store::resetWrite()" << endl;

    yaml->reset();
    return nvram.initForWrite();
}


void cm_yaml_store::writeSimple(const cm_simple_metadata * data, const uint8_t * v)
{
    yaml->writeSimple(data->c.name, data->c.len, v, data->pPrt);
}


void cm_yaml_store::startWriteComposite(const cm_composite_metadata * data)
{
    yaml->startWriteComposite(data->c.name);
}


void cm_yaml_store::endWriteComposite()
{
    yaml->endWriteComposite();
}

t_cm_result cm_yaml_store::startLoadSimple(const cm_simple_metadata * data)
{
    cout << __PRETTY_FUNCTION__ << endl;
    return yaml->startLoadSimple(data->c.name);
}

t_cm_result cm_yaml_store::endLoadSimple(uint8_t * pRam, const cm_simple_metadata * data)
{
    cout << __PRETTY_FUNCTION__ << endl;
    cm_item_len_t len = data->c.len;
    return yaml->endLoadSimple(&len, pRam, data->pSet);
}

// xxx incomplete
t_cm_result cm_yaml_store::startLoadComposite(const cm_composite_metadata * data)
{
    cout << "cm_yaml_store::startLoadComposite()" << endl;

    return yaml->startLoadComposite(data->c.name);
}

t_cm_result cm_yaml_store::endLoadComposite()
{
    cout << "cm_yaml_store::endLoadComposite()" << endl;
    return yaml->endLoadComposite();
}
