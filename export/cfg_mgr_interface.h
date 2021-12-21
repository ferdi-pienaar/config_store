#ifndef CFG_MGR_INTERFACE_H
#define CFG_MGR_INTERFACE_H

namespace cfg_mgr
{

class Config_manager;
class Nvram;
class Descriptor;

/// Interface class for configuration manager, managing all configurable items in the system.
// This class is the application programme's sole point of access to
// the configurable items, that live in RAM allocated by this class.
// This class is a wrapper around Config_manager; its purpose is to
// avoid exporting internal classes used by Config_manager.
// It's also convenient to have a thin wrapper that allocates nvram module and passes it
// to the implementation class. It means the client doesn't have to know about nvram,
// but that when we test the implementation class, we can pass it a fake nvram object.
//
class Config_manager_interface
{
public:
    Config_manager_interface(const Descriptor * pDesc);
    ~Config_manager_interface();
    void handleCmd(int argc, char *argv[]);
    const char * getPromptString() const; ///< get context-dependent prompt string h file
    void * getConfig();

private:
    Config_manager * m_config_manager;
    Nvram * m_nvram;
};

}
#endif // CFG_MGR_INTERFACE_H
