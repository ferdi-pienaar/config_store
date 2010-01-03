#include "config_manager.h"
using namespace std;

config_manager::config_manager(cm_item_descriptor * desc)
{
    base_desc = desc;
}

void config_manager::do_op(string op)
{

    if (op == "add")
    {
        cout << "ADD" << endl;
    }
    else
    {
        cout << "Unknown op " << op << "." << endl;
    }
}



