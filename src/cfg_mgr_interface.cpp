#include "cfg_mgr_interface.h"
#include "cfg_mgr.h"

namespace cfg_mgr
{

Config_manager_interface::Config_manager_interface(const Descriptor * pDesc, Nvram * nvram)
{
    m_config_manager = new Config_manager(pDesc, nvram);
}

Config_manager_interface::~Config_manager_interface()
{
    delete m_config_manager;
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
