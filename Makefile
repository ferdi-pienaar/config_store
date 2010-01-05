CXXFLAGS = -Wall -g # -save-temps 

test_cfg_man: config_manager.o config_manager_util.o my_cfg.o config_manager.h my_cfg.h config_manager_util.h

