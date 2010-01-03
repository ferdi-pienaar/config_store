#include "config_manager.h"
using namespace std;

config_manager::config_manager(cm_item_descriptor * desc)
{
    base_desc = desc;
}

void config_manager::do_cmd(int argc, char *argv[])
{

    if (argv[0] == "add")
    {
        cout << "ADD" << endl;
    }
    else
    {
        cout << "Unknown op " << argv[0] << "." << endl;
    }
}



