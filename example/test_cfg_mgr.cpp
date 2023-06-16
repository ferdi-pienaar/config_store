/*
  Manual test framework that accepts inputs from stdin; CM produces output to stdout.

 */
#include <iostream>
#include "cfg_mgr.h"
#include "my_cfg.h"
#include "cfg_mgr_strtok.h" // strtok
#include <pthread.h>
#include <unistd.h> // sleep
#include <assert.h>

using namespace std;

#define WORD_DELIMITERS " \n"
#define BLOCK_DELIMITER "\""

static void * stats_thread(void * arg);
static void handle_command(cfg_mgr::Config_manager & cm);

int main()
{
    // Initialize the config manager with the base descriptor it is to manage.
    cfg_mgr::Config_manager cm(get_base_descriptor());

    pthread_t thread;
    int rc = pthread_create(&thread, NULL, stats_thread, &cm);
    assert(0 == rc);

    while (true)
    {
        handle_command(cm);
    }
}

// Get a command from stdin and pass it to Config_manager.
void handle_command(cfg_mgr::Config_manager & cm)
{
    char   cmd[120];
    char * param[20];
    int    wordCnt = 0;
    
    printf("%s> ", cm.getPromptString());
    
    if (fgets(cmd, sizeof(cmd), stdin) == NULL)
    {
        return;
    }
    
    cfg_mgr::Strtok strtok(cmd);
    while ((param[wordCnt] = strtok(WORD_DELIMITERS, BLOCK_DELIMITER)))
    {
        wordCnt++;
    }
    cm.handleCmd(wordCnt, param);
}

// Periodically, update some stats that can be displayed by cfg_mgr.
void * stats_thread(void * arg)
{
    cfg_mgr::Config_manager * cm = (cfg_mgr::Config_manager *)arg;
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
