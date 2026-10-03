#include "cfg_mgr_contained_aggregate.h"
#include "cfg_mgr_descriptor.h"
#include "cfg_mgr_dbg.h"

using namespace std;

namespace cfg_mgr
{

// Give name, count
void Contained_aggregate::help(string &prefix, unsigned depth) const
{
    string info = "[" + to_string(getData()->maxCount) + "] ";
    m_data->pDesc->help(prefix, info, depth);
}

}
