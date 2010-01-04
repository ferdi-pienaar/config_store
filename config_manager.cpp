#include "config_manager.h"
using namespace std;

typedef enum
{
    CM_ADD,
    CM_DEL,
    CM_PRT,
    CM_SET,
    CM_SETDEF,
    CM_LOAD,
    CM_SAVE,    
    CM_CMD_NONE,

} eCmCmd;

config_manager::config_manager(cm_item_descriptor * desc)
{
    base_desc = desc;
}

void config_manager::do_cmd(int argc, char *argv[])
{
    int i;
    eCmCmd cmd = CM_CMD_NONE;
    
    for (i = 0; i < argc; i++)
    {
        cout << i << ": " << argv[i] << "." << endl;

        if (strcmp(argv[i], "add") == 0)
        {
            cmd = CM_ADD;
            break;
        }
        else if (strcmp(argv[i], "del") == 0)
        {            
            cmd = CM_DEL;
            break;
        }
        else if (strcmp(argv[i], "prt") == 0)
        {
            cmd = CM_PRT;
            break;
        }
        else if (strcmp(argv[i], "set") == 0)
        {
            cmd = CM_SET;
            break;
        }
        else if (strcmp(argv[i], "setdef") == 0)
        {
            cmd = CM_SETDEF;
            break;
        }
        else if (strcmp(argv[i], "load") == 0)
        {
            cmd = CM_LOAD;
            break;
        }
        else if (strcmp(argv[i], "save") == 0)
        {
            cmd = CM_SAVE;
            break;
        }
    }


    if (cmd != CM_CMD_NONE)
    {
        cout << "command at " << i << endl;
    }

    switch (cmd)
    {
        case CM_LOAD:
            cout << "load at " << i;
            break;

        case CM_SAVE:
            cout << "save at " << i;
            break;

        default:
            break;
    }
}


//
// argc number of items in argv
// argv array of strings containing name elements
// ppItem - output, updated when item is found
// ppRam - input/output, pointer to RAM at which item is located
//
// When we enter this method, we already know the composite
// determined this object has a matching name.
void cm_composite_item_descriptor::getItem(int argc,
                                           char *argv[],
                                           cm_item_descriptor ** ppItem,
                                           unsigned char **ppRam)
{
    // Sanity check
    if (strcmp(argv[0], name.c_str()) != 0)
    {
        return;
    }

    cout << "found " << name << endl;

    // Search for matching component
    for (int i = 0; i < compCount; i++)
    {
        cm_component * pComp = &(compList[i]);

        // xxx create a public getName function
        cout << pComp->pDesc->getName();
    }
}




