CXXFLAGS = -Wall -g # -save-temps 

test_cfg_man: config_manager.o my_cfg.o config_manager.h my_cfg.h
