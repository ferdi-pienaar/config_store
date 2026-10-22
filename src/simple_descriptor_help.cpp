#include "cfg_mgr_simple_descriptor.h"
#include "item_identifier.h"
#include <string>

using namespace std;

namespace cfg_mgr
{

void Simple_descriptor::handleHelp(Command_stack * cmd) const
{
    string empty;
    help(empty, empty, 0);
}

void Simple_descriptor::help(string prefix, const string &aggr_info, unsigned depth) const
{
    m_mgr_service->m_print("%s%s %slen=%u %spersistent\n",
        prefix.c_str(), getName(), aggr_info.c_str(), getLen(), m_data->c.isPersistent() ? "" : "not ");
}

}
