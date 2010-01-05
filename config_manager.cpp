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

// Execute command words from client.
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

    unsigned char      * pRam;
    cm_item_descriptor * pDesc;

    if (1)
    {
        pDesc = base_desc;
        pRam  = ramBase;
        
        // xxx for certain ops, find the item and descriptor to which to apply the op
        // xxx in some cases, we might not want to start
        base_desc->getItem(argc, argv, &pDesc, &pRam);
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


/// Write TLV to memory, and advance the ptr to the end of memory written to.
//  This is useful for writing to a RAM buffer first, for subsequent write
//  to NVRAM.
//  xxx if we want to write directly to NVRAM, we need to implement a method
//  that does that...
void cm_item_descriptor::writeTlv(unsigned char ** ppMem)
{
    **ppMem = id;            // write to memory
    *ppMem += sizeof(id);    // advance the memory pointer
    **ppMem = getLen();           // write to memory
    *ppMem += sizeof(cm_item_len);   // advance the memory pointer
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

cm_item_len cm_composite_item_descriptor::getLen()
{
    // xxx recursive calls to components' getLen.
    // xxx verify we don't overflow cm_item_len
    return 0;
}


void cm_simple_item_descriptor::print(unsigned char * pRam, cm_item_len len)
{
    if (pPrt == NULL)
    {
        // xxx error handling
        cout << "err" << endl;
        return;
    }
    pPrt(pRam, len);
}

void cm_simple_item_descriptor::setdef(unsigned char * pRam, cm_item_len len)
{
    if (pSetDef == NULL)
    {
        // No function installed, so use default default value: 0
        memset(pRam, 0, len);
        return;
    }
    pSetDef(pRam, len);
}




