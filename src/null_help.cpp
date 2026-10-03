#include "cfg_mgr_composite_descriptor.h"
#include "cfg_mgr_simple_descriptor.h"
#include "cfg_mgr_contained_aggregate.h"
#include "cfg_mgr_owned_aggregate.h"
#include <string>

using namespace std;

namespace cfg_mgr
{

void Composite_descriptor::handleHelp(Command_stack * cmd) const
{
}

void Composite_descriptor::help(string &prefix, const string &aggr_info, unsigned depth) const
{
}

void Simple_descriptor::handleHelp(Command_stack * cmd) const
{
}

void Simple_descriptor::help(string &prefix, const string &aggr_info, unsigned depth) const
{
}

void Contained_aggregate::help(string &prefix, unsigned depth) const
{
}

void Owned_aggregate::help(string &prefix, unsigned depth) const
{
}

}
