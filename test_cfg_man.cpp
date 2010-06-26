/*
  Manual test framework that accepts inputs from stdin; CM produces output to stdout.
 
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
    config_manager * cm = config_manager::getInstance();

    cm->init(pBaseDesc);
    
    while (true)
    {
        char   cmd[120];
        char * param[20];
        int    wordCnt = 0;

        printf("%s> ", cm->getPromptString());

        if (fgets(cmd, sizeof(cmd), stdin) == NULL)
        {
            continue;
        }

        param[0] = strtok(cmd, WORD_DELIMITERS);

        while (NULL != (param[++wordCnt] = strtok(NULL, WORD_DELIMITERS)))
        {}

        cm->handleCmd(wordCnt, param);
    }
}



