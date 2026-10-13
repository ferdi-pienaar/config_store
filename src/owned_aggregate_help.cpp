#include "cfg_mgr_owned_aggregate.h"
#include "cfg_mgr_contained_aggregate.h"
#include "cfg_mgr_descriptor.h"
#include "cfg_mgr_dbg.h"

using namespace std;

namespace cfg_mgr
{

// Give name, current count, and maxcount.
// xxx todo if there's no counter, give a warning if one is necessary, i.e. if maxCount > 1.
void Owned_aggregate::help(string prefix, unsigned depth) const
{
    string info = "=> [" + to_string(getData()->maxCount) + "] ";
    if (m_data->m_counterAggr != nullptr)
    {
        info += string("(counter ") + string(m_data->m_counterAggr->getData()->pDesc->getName()) + ") ";
    }
    getData()->pDesc->help(prefix, info, depth);
}

}
