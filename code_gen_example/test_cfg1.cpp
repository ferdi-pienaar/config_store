/*
  Manual test framework that accepts inputs from stdin; CM produces output to stdout.

 */
#include <iostream>
#include "nvram.h"
#include "cfg_mgr.h"
#include "cfg.h"
#include "cfg_mgr_strtok.h" // strtok
#include <pthread.h>
#include <unistd.h> // sleep
#include <assert.h>

using namespace std;

const cfg_mgr::Descriptor * get_base_descriptor();

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

    // Read commands from stdin and give them to the config manager
    while (true)
    {
        handle_command(cm);
    }
}

// Get command from stdin and pass it to Config_manager.
void handle_command(cfg_mgr::Config_manager & cm)
{
    printf("%s> ", cm.getPromptString());

    char cmd[120];
    if (fgets(cmd, sizeof(cmd), stdin) == nullptr)
    {
        return;
    }

    // Break commands into a list of tokens, as expected by config manager
    char * param[20];
    cfg_mgr::Strtok strtok(cmd);
    unsigned int wordCnt = 0;
    while ((param[wordCnt] = strtok(WORD_DELIMITERS, BLOCK_DELIMITER)))
    {
        wordCnt++;
    }
    cm.handleCmd(wordCnt, param);
}

// Periodically, update some stats that can be displayed by cfg_mgr.
// NB: there's a race condition here! By the time we modify config
// it may not exist anymore.
void * stats_thread(void * arg)
{
    cfg_mgr::Config_manager * cm = (cfg_mgr::Config_manager *)arg;
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
