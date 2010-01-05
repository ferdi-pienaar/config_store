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
    CM_OP_NONE,

} eCmOp;

static eCmOp getOp(char * word);


config_manager::config_manager(cm_item_descriptor * desc)
{
    base_desc = desc;
}

// Execute command words from client.
void config_manager::do_cmd(int argc, char *argv[])
{
    int i;
    eCmOp op = CM_OP_NONE;
    
    for (i = 0; i < argc; i++)
    {
        cout << i << ": " << argv[i] << "." << endl;

        if (getOp(argv[i]) != CM_OP_NONE)
        {
            break;
        }
    }


    if (op != CM_OP_NONE)
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
        base_desc->do_cmd(argc, argv, pRam);
    }

    switch (op)
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
// pRam - pointer to RAM at which item is located
//
void cm_composite_item_descriptor::do_cmd(int argc,
                                          char *argv[],
                                          unsigned char * pRam)
{
    // Sanity check
    #if 0
    if (strcmp(argv[0], name.c_str()) != 0)
    {
        return;
    }

    #endif

    eCmOp op = getOp(argv[0]);

    if (op == CM_PRT)
    {
        print(pRam);
        return;
    }

}

// Delegate print command to components
void cm_composite_item_descriptor::print(unsigned char * pRam)
{
    unsigned short last;
    
    for (int i = 0; i < compCount; i++)
    {
        cm_component * pComp = &(compList[i]);

        if (pComp->type == cm_component::CONTAINED)
        {
            for (int j = 0; j < pComp->count; j++)
            {
                pComp->pDesc->print(pRam);
                pRam += pComp->pDesc->getLen();
            }
        }
    }
}


cm_item_len cm_composite_item_descriptor::getLen()
{
    // xxx recursive calls to components' getLen.
    // xxx verify we don't overflow cm_item_len
    return 0;
}

//
// argc number of items in argv
// argv array of strings containing name elements
// pRam - pointer to RAM at which item is located
//
void cm_simple_item_descriptor::do_cmd(int argc,
                                       char *argv[],
                                       unsigned char * pRam)
{
    
    
}


void cm_simple_item_descriptor::print(unsigned char * pRam)
{
    if (pPrt == NULL)
    {
        // No function installed so default print function: hex chars
        for (int i = 0; i < len; i++)
        {
            printf("%02x", pRam[i]);
        }
    }
    else
    {
        pPrt(pRam, len);
    }
}

void cm_simple_item_descriptor::setdef(unsigned char * pRam)
{
    if (pSetDef == NULL)
    {
        // No function installed, so use default default value: 0
        memset(pRam, 0, len);
    }
    else
    {
        pSetDef(pRam, len);
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

// Helper function that returns what kind of operation (if any) a word is
eCmOp getOp(char * word)
{
    if (strcmp(word, "add") == 0) return CM_ADD;
    if (strcmp(word, "del") == 0) return CM_DEL;
    if (strcmp(word, "prt") == 0) return CM_PRT;
    if (strcmp(word, "set") == 0) return CM_SET;
    if (strcmp(word, "setdef") == 0) return CM_SETDEF;
    if (strcmp(word, "load") == 0) return  CM_LOAD;
    if (strcmp(word, "save") == 0) return CM_SAVE;

    // If no match, it's not an operation
    return CM_OP_NONE;
}



