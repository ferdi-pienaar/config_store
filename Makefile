CXXFLAGS = -Wall -g # -DDEBUG_PRINT # -save-temps 

test_cfg_man: config_manager.o config_manager_util.o my_cfg.o config_manager.h my_cfg.h config_manager_util.h \
 my_cfg_fn.o my_cfg_fn.h

