#include "cfg_mgr.h"
#include "cfg_mgr_implement.h"
#include "nvram.h"

namespace cfg_mgr
{

// Having this class inject Nvram reference to Config_manager_implement
// simplifies testing, since it allows test to inject a spy
// to Config_manager_implement.
Config_manager::Config_manager(const Descriptor * pDesc)
{
    m_nvram = new Nvram;
    m_config_manager = new Config_manager_implement(pDesc, m_nvram);
}

Config_manager::~Config_manager()
{
    delete m_config_manager;
    delete m_nvram;
}

void Config_manager::handleCmd(int argc, char *argv[])
{
    m_config_manager->handleCmd(argc, argv);
}

const char * Config_manager::getPromptString() const
{
    return m_config_manager->getPromptString();
}

void * Config_manager::getConfig()
{
    return m_config_manager->getConfig();
}

}
