CXXFLAGS = -Wall -g -I . -I ../src -I ../export 
#CXXFLAGS += -DDEBUG_PRINT 
#CXXFLAGS += -save-temps 
CXXFLAGS += -D M_DEFINED_STORE_TYPE=TLV_STORE

CFG_MAN_SRC_DIR = ../src/

test_cfg_man: $(CFG_MAN_SRC_DIR)config_manager.o \
$(CFG_MAN_SRC_DIR)config_manager_util.o \
$(CFG_MAN_SRC_DIR)config_manager_setdef_null.o \
$(CFG_MAN_SRC_DIR)config_manager_store.o \
$(CFG_MAN_SRC_DIR)config_manager_tlv_store.o \
$(CFG_MAN_SRC_DIR)config_manager_tlv.o \
$(CFG_MAN_SRC_DIR)nvram.o \
$(CFG_MAN_SRC_DIR)config_manager_tlv.h \
$(CFG_MAN_SRC_DIR)config_manager_util.h \
../export/config_manager.h \
my_cfg.o my_cfg_fn.o my_cfg_fn.h my_cfg.h

