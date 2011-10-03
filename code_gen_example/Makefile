CXXFLAGS = -Wall -g -I . -I ../src -I ../export # -DDEBUG_PRINT # -save-temps 

CFG_MAN_SRC_DIR = ../src/

test_cfg1: $(CFG_MAN_SRC_DIR)config_manager.o \
$(CFG_MAN_SRC_DIR)config_manager_util.o \
$(CFG_MAN_SRC_DIR)config_manager_setdef_null.o \
$(CFG_MAN_SRC_DIR)config_manager_store.o \
$(CFG_MAN_SRC_DIR)config_manager_tlv_store.o \
$(CFG_MAN_SRC_DIR)config_manager_tlv.o \
$(CFG_MAN_SRC_DIR)nvram.o \
$(CFG_MAN_SRC_DIR)config_manager_tlv.h \
$(CFG_MAN_SRC_DIR)config_manager_util.h \
../export/config_manager.h \
my_cfg1.o my_cfg_fn1.o my_cfg_fn1.h my_cfg1.h

my_cfg1.h: cfg.h

my_cfg1.cpp: cfg.cpp

cfg.cpp cfg.h: cfg.yaml
	python ../code_gen/yaml_code_gen.py cfg.yaml

.PHONY: clean
clean:
	rm -f ../src/*o
	rm -f *o
	rm cfg.cpp
	rm cfg.h

