CXXFLAGS = -Wall -g -I . -I ../src -I ../export 
#CXXFLAGS += -DDEBUG_PRINT 
#CXXFLAGS += -save-temps 
CXXFLAGS += -D M_DEFINED_STORE_TYPE=TLV_STORE
CFG_MAN_SRC_DIR = ../src/
VPATH = $(CFG_MAN_SRC_DIR) # so make can find the library

# xxx Why do we need config_manager_setdef_null.o here?  It should be in libCfgMan.a!!!
test_cfg_man: -lCfgMan \
$(CFG_MAN_SRC_DIR)config_manager_setdef_null.o \
$(CFG_MAN_SRC_DIR)config_manager_util.h \
$(CFG_MAN_SRC_DIR)config_manager_setdef_null.h \
../export/config_manager.h \
my_cfg.o my_cfg_fn.o my_cfg_fn.h my_cfg.h

