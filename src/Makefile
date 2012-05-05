CXXFLAGS = -Wall -g -I . -I ../export
CXXFLAGS += -D M_DEFINED_STORE_TYPE=TLV_STORE
#CXXFLAGS += -D M_DEFINED_STORE_TYPE=YAML_STORE
#CXXFLAGS += -save-temps
#CXXFLAGS += -D DEBUG_PRINT
OBJS = config_manager.o config_manager_util.o config_manager_setdef_null.o config_manager_store.o nvram.o

OBJS += config_manager_tlv_store.o config_manager_tlv.o

#OBJS += config_manager_yaml_store.o config_manager_yaml.o

# -cr means
# c: create the archive (without this option, if archive doesn't exist, there's a warning)
# r: insert the files into archive (with replacement)
libCfgMan.a: $(OBJS)
	ar -cr $@ $^

.PHONY: clean
clean:
	rm -f *o

# Auto-generate dependencies using gcc's MM option -- see 'Managing Projects with GNU Make'
include config_manager_setdef_null.d config_manager.d nvram.d config_manager_yaml.d \
 config_manager_tlv.d config_manager_yaml_store.d config_manager_tlv_store.d \
 config_manager_store.d config_manager_util.d

%.d: %.cpp
	$(CXX) -MM $(CXXFLAGS) $< > $@.$$$$; \
	sed 's,\($*\)\.o[ :]*,\1.o $@ : ,g' < $@.$$$$ > $@; \
	rm -f $@.$$$$

