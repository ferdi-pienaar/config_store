#pragma once
#include "cfg_mgr_types.h"

namespace cfg_mgr
{

class Config_manager_implement;
class Nvram_itf;
class Descriptor;

/// Interface class for configuration manager, managing all configurable items in the system.
// This class is the application programme's sole point of access to
// the configurable items, that live in RAM allocated by this class.
// This class is a wrapper around Config_manager_implement; its purpose is to
// avoid exporting internal classes used by Config_manager_implement.
// It's also convenient to have a thin wrapper that allocates nvram module and passes it
// to the implementation class. It means the client doesn't have to know about nvram,
// but that when we test the implementation class, we can pass it a fake nvram object.
//
class Config_manager
{
public:
    Config_manager(const Descriptor * pDesc, const char * fname, PRINTF_FN_TYPE printf_fn, uint8_t ** ppRAM);
    ~Config_manager();
    void handleCmd(int argc, char *argv[]);
    const char * getPromptString() const; ///< get context-dependent prompt string h file

private:
    Config_manager_implement * m_config_manager;
    Nvram_itf * m_nvram;
};

}
