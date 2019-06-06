///
// YAML format config data, for storage in non-volatile media, e.g. a file.
// This is defined as a separate class hierarchy, since it's an optional feature:
// not all managed objects are saved in NVRAM.
//
// This module asserts that the data passed to it by the client is valid, e.g.
// in writing:
//  - no endWriteComposite without matching startWriteComposite.
//  - no more than stackDepth nested calls to startWriteComposite.
//
// The expected sequence of client calls in writing:
// - writeSimple, or
// - startWriteComposite, then
//   - writeSimple or startWriteComposite
//   - a call to endWriteComposite for each call to startWriteComposite
//
// The expected sequence of client calls in loading:
// - getType, then
// - loadComposite or loadSimple, depending on client's interpretation of the type
//   returned by getType
// The client monitors the "complete" field returned by loadSimple to determine if
// one or more of the composite loads in progress are complete.
//

#include <stdint.h> // uint8_t, etc
#include "config_manager_yaml.h"
#include "config_manager_dbg.h"
#include <iostream>
#include <string.h> // memcpy
#include "nvram.h"
#include <assert.h>

using namespace std;

Yaml::Yaml(Nvram * pNvram): nvram(pNvram), stackIndex(0)
{
}


Yaml::~Yaml()
{
}


void Yaml::reset()
{
    stackIndex = 0;
}


void Yaml::writeSimple(const char * name, cm_item_len_t length, const uint8_t * v, YAML_PRT_FPTR prt)
{
    write_indent();
    cout << name << ": ";
    prt(stdout, v, length);

    cout << endl;
}


// Don't actually write anything yet, since it may turn out that this
// composite is empty.
void Yaml::startWriteComposite(const char * name)
{
    write_indent();
    cout << name << ": " << endl;
    stackIndex++;
}


// Close writing of composite by writing L of TLV
// xxx return boolean to indicate if we've reached the bottom of the stack?
void Yaml::endWriteComposite()
{
    stackIndex--;

    write_indent();
    cout << "end" << endl;
}

t_cm_result Yaml::startLoadSimple(const char * name)
{
    return CM_SUCCESS;
}

t_cm_result Yaml::endLoadSimple(cm_item_len_t * length, uint8_t * pRam)
{

    return CM_SUCCESS;
}

t_cm_result Yaml::startLoadComposite(const char * name)
{
    return CM_SUCCESS;
}

t_cm_result Yaml::endLoadComposite()
{

    return CM_SUCCESS;
}

void Yaml::startWriteList(const char * name)
{

}

void Yaml::endWriteList()
{

}

void Yaml::write_indent()
{
    for (int i = 0; i < stackIndex; i++)
    {
        cout << " ";
    }
}

