CFG_MAN_SRC_DIR = ../src/
CFG_MAN_UTIL_DIR = ../util/
CXXFLAGS = -Wall -g -I . -I $(CFG_MAN_SRC_DIR) -I $(CFG_MAN_UTIL_DIR) -I ../export
#CXXFLAGS += -save-temps 
VPATH = $(CFG_MAN_SRC_DIR) # so make can find the library

test_cfg_man:  my_cfg.o my_cfg_fn.o \
$(CFG_MAN_UTIL_DIR)config_manager_set_int.o \
$(CFG_MAN_UTIL_DIR)config_manager_prt_int.o \
$(CFG_MAN_UTIL_DIR)config_manager_set_str.o \
$(CFG_MAN_UTIL_DIR)config_manager_prt_str.o \
$(CFG_MAN_UTIL_DIR)config_manager_set_int.h \
$(CFG_MAN_UTIL_DIR)config_manager_prt_int.h \
$(CFG_MAN_UTIL_DIR)config_manager_set_str.h \
$(CFG_MAN_UTIL_DIR)config_manager_prt_str.h \
$(CFG_MAN_SRC_DIR)config_manager_setdef_null.h \
../export/config_manager.h \
my_cfg_fn.h my_cfg.h \
-lCfgMgr

.PHONY: clean
clean:
	rm -f *o
