CXXFLAGS = -Wall -g -I . -I ../src -I ../export 
#CXXFLAGS += -DDEBUG_PRINT 
#CXXFLAGS += -save-temps 
CXXFLAGS += -D M_DEFINED_STORE_TYPE=TLV_STORE

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

# .o depends on both cpp files, but only one of them is in the command
my_cfg1.o: cfg.cpp my_cfg1.cpp
	$(CXX) $(CXXFLAGS) -c -o $@ my_cfg1.cpp

cfg.cpp: cfg.yaml
	python ../code_gen/yaml_code_gen.py cfg.yaml

.PHONY: clean
clean:
	rm -f ../src/*o
	rm -f *o
	rm cfg.cpp
	rm cfg.h

