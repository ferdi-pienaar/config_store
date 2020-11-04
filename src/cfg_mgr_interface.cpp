#include "cfg_mgr_interface.h"
#include "cfg_mgr.h"
#include "nvram.h"

namespace cfg_mgr
{

// Having this class inject Nvram reference to Config_manager
// simplifies testing, since it allows test to inject a spy
// to Config_manager.
Config_manager_interface::Config_manager_interface(const Descriptor * pDesc)
{
    m_nvram = new Nvram;
    m_config_manager = new Config_manager(pDesc, m_nvram);
}

Config_manager_interface::~Config_manager_interface()
{
    delete m_config_manager;
    delete m_nvram;
}

void Config_manager_interface::handleCmd(int argc, char *argv[])
{
    m_config_manager->handleCmd(argc, argv);
}

const char * Config_manager_interface::getPromptString() const
{
    return m_config_manager->getPromptString();
}

void * Config_manager_interface::getConfig()
{
    return m_config_manager->getConfig();
}

}
