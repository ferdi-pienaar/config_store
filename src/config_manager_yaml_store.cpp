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


bool cm_yaml_store::resetRead()
{    
    cout << "cm_yaml_store::resetRead()" << endl;

    yaml->reset();
    return nvram.initRead();
}


bool cm_yaml_store::resetWrite()
{    
    cout << "cm_yaml_store::resetWrite()" << endl;

    yaml->reset();
    return nvram.initWrite();
}


void cm_yaml_store::writeSimple(const cm_simple_metadata * data, const uint8_t * v)
{    
    yaml->writeSimple(data->c.name, data->c.id, v, data->c.len, data->pPrt);
}


void cm_yaml_store::startWriteComposite(const cm_composite_metadata * data)
{    
    yaml->startWriteComposite(data->c.name, data->c.id);
}


void cm_yaml_store::endWriteComposite()
{    
    yaml->endWriteComposite();
}


t_cm_result cm_yaml_store::getType(cm_item_id_t * t)
{    
    cout << "cm_yaml_store::getType()" << endl;
    return yaml->getType(t);
}


t_cm_result cm_yaml_store::loadSimple(uint8_t * pRam, const cm_simple_metadata * data, unsigned * complete)
{    
    cout << "cm_yaml_store::loadSimple()" << endl;
    cm_item_len_t len = data->c.len;
    return yaml->loadSimple(pRam, &len, complete);

}


t_cm_result cm_yaml_store::loadComposite()
{    
    cout << "cm_yaml_store::loadComposite()" << endl;

    return yaml->loadComposite();
}


void cm_yaml_store::skipItem(unsigned * complete)
{
    cout << "cm_yaml_store::skipItem()" << endl;

    yaml->skipItem(complete);
}

