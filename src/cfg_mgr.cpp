#include "cfg_mgr.h"
#include "cfg_mgr_implement.h"
#include "cfg_mgr_printf.h"
#include "store/nvram.h"

namespace cfg_mgr
{

// This function is supplied by the client (or test).
PRINTF_FN_TYPE cm_printf = nullptr;

// Having this class inject Nvram reference to Config_manager_implement
// simplifies testing, since it allows test to inject a spy
// to Config_manager_implement.
Config_manager::Config_manager(const Descriptor * pDesc, PRINTF_FN_TYPE printf_fn, uint8_t ** ppRAM)
{
    m_nvram = new Nvram;
    cm_printf = printf_fn;
    m_config_manager = new Config_manager_implement(pDesc, ppRAM, m_nvram);
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

}
