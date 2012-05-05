CXXFLAGS = -Wall -g -I . -I ../src -I ../export 
#CXXFLAGS += -save-temps 
CFG_MAN_SRC_DIR = ../src/
VPATH = $(CFG_MAN_SRC_DIR) # so make can find the library

test_cfg_man:  my_cfg.o my_cfg_fn.o \
$(CFG_MAN_SRC_DIR)config_manager_util.h \
$(CFG_MAN_SRC_DIR)config_manager_setdef_null.h \
../export/config_manager.h \
my_cfg_fn.h my_cfg.h \
-lCfgMan

.PHONY: clean
clean:
	rm -f *o
