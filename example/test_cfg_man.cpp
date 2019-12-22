/*
  Manual test framework that accepts inputs from stdin; CM produces output to stdout.

 */
#include <iostream>
#include "config_manager.h"
#include "my_cfg.h"
#include "config_manager_strtok.h" // strtok
#include <pthread.h>
#include <unistd.h> // sleep

using namespace std;
using namespace cfg_mgr;

#define WORD_DELIMITERS " \n"
#define BLOCK_DELIMITER "\""


// Periodically, update some stats that can be displayed by cfg_mgr.
void * stats_thread(void * arg)
{
    Config_manager * cm = (Config_manager *)arg;
    tDevice * pCfg = (tDevice *)cm->getConfig();

    for (;;)
    {
        sleep(5);
        static unsigned userIdx = 0;

        if (pCfg->userCount > 0)
        {
            userIdx = (userIdx >= pCfg->userCount - 1) ? 0 : userIdx + 1;

            pCfg->users[userIdx].elapsed++;
        }
    }
    return NULL;
}


int main()
{
    // Initialize the config manager with the base descriptor it is to manage.
    Config_manager cm(get_base_descriptor());

    pthread_t thread;
    int rc = pthread_create(&thread, NULL, stats_thread, &cm);
    assert(0 == rc);

    while (true)
    {
        char   cmd[120];
        char * param[20];
        int    wordCnt = 0;

        printf("%s> ", cm.getPromptString());

        if (fgets(cmd, sizeof(cmd), stdin) == NULL)
        {
            continue;
        }

        Strtok strtok(cmd);
        while (nullptr != (param[wordCnt] = strtok(WORD_DELIMITERS, BLOCK_DELIMITER)))
        {
            wordCnt++;
        }
        cm.handleCmd(wordCnt, param);
    }
}
