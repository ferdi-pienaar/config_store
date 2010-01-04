#include <iostream>
#include "config_manager.h"
#include "my_cfg.h"
#include <cstring> // strtok


using namespace std;



/* 
 TBD: create tests that take a series of commands as input.
 They verify the working of the module by checking that the
 output of a prt command is as expected.

 TBD: create tests that can verify that data is correctly saved
 to NVRAM.
*/


int main()
{
    // Initialize the config manager with the base descriptor it is to manage.
    config_manager * cm = new config_manager(pBaseDesc);
    
    while (true)
    {
        char cmd[120];
        char * param[20];
        int wordCnt = 0;

        printf(">");

        fgets(cmd, sizeof(cmd), stdin);

        param[0] = strtok(cmd, " ");

        while (NULL != (param[++wordCnt] = strtok(NULL, " ")))
        {}

        cm->do_cmd(wordCnt, param);
    }
}




