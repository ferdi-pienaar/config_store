/*
  Manual test framework that accepts inputs from stdin; CM produces output to stdout.

 */
#include <iostream>
#include "config_manager.h"
#include "cfg.h"
#include <cstring> // strtok
#include <pthread.h>
#include <unistd.h> // sleep

using namespace std;
using namespace cfg_mgr;

const Descriptor * get_base_descriptor();

#define WORD_DELIMITERS " \n"

// Periodically, update some stats that can be displayed by cfg_mgr.
// NB: there's a race condition here! By the time we modify config
// it may not exist anymore.
void * stats_thread(void * arg)
{
    Config_manager * cm = (Config_manager *)arg;
    t_device * pCfg = (t_device *)cm->getConfig();

    for (;;)
    {
        sleep(5);

        if (pCfg->userCnt > 0)
        {
            static unsigned short userIdx = 0;
            userIdx = (userIdx >= pCfg->userCnt - 1) ? 0 : userIdx + 1;
            pCfg->user[userIdx].elapsed++;
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

    // Read commands from stdin and give them to the config manager
    while (true)
    {
        printf("%s> ", cm.getPromptString());

        char cmd[120];
        if (fgets(cmd, sizeof(cmd), stdin) == NULL)
        {
            continue;
        }

        // Break commands into a list of words, as expected by config manager
        char * param[20];
        if ((param[0] = strtok(cmd, WORD_DELIMITERS)) != NULL)
        {
            unsigned wordCnt = 1;
            while (NULL != (param[wordCnt] = strtok(NULL, WORD_DELIMITERS)))
            {
                wordCnt++;
            }
            cm.handleCmd(wordCnt, param);
        }
    }
}


