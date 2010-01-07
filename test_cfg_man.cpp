/*
 Test framework accepts inputs from stdin; CM produces output to stdout.
 @todo wrap around this a script framework for automated testing, that
 feeds inputs to stdin and receives results for comparision from stdout.
 TBD: create tests that can verify that data is correctly saved
 to NVRAM.
 */
#include <iostream>
#include "config_manager.h"
#include "my_cfg.h"
#include <cstring> // strtok

using namespace std;

#define WORD_DELIMITERS " \n"

int main()
{
    // Initialize the config manager with the base descriptor it is to manage.
    config_manager * cm = new config_manager(pBaseDesc);

    cm->init();
    
    while (true)
    {
        char cmd[120];
        char * param[20];
        int wordCnt = 0;

        printf(">");

        fgets(cmd, sizeof(cmd), stdin);

        param[0] = strtok(cmd, WORD_DELIMITERS);

        while (NULL != (param[++wordCnt] = strtok(NULL, WORD_DELIMITERS)))
        {}

        cm->do_cmd(wordCnt, param);
    }
}




