# To find files that are included by the exported config_manager.h...
CFG_MAN_SRC_DIR = ../src/
CXXFLAGS = -Wall -g -I . -I $(CFG_MAN_SRC_DIR) -I ../export 
#CXXFLAGS += -save-temps 
CXXFLAGS += -Wl,-Map=output.map
VPATH = $(CFG_MAN_SRC_DIR) # so make can find the library

# xxx Why do we need config_manager_setdef_null.o here?  It should be in libCfgMan.a!!!
test_cfg1: -lCfgMan \
$(CFG_MAN_SRC_DIR)config_manager_setdef_null.o \
$(CFG_MAN_SRC_DIR)config_manager_util.h \
$(CFG_MAN_SRC_DIR)config_manager_setdef_null.h \
../export/config_manager.h \
my_cfg1.o my_cfg_fn1.o my_cfg_fn1.h my_cfg1.h

# .o depends on both cpp files, but only one of them is in the command
my_cfg1.o: cfg.cpp my_cfg1.cpp my_cfg1.h
	$(CXX) $(CXXFLAGS) -c -o $@ my_cfg1.cpp

cfg.cpp: cfg.yaml
	python ../code_gen/yaml_code_gen.py cfg.yaml

.PHONY: clean
clean:
	rm -f *o
	rm cfg.cpp
	rm cfg.h

