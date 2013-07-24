CXXFLAGS = -Wall -g -I . -I ../export
# Include some C99 macros that are not part of the C++ standard
CPPFLAGS += -D __STDC_LIMIT_MACROS
CPPFLAGS += -D M_DEFINED_STORE_TYPE=TLV_STORE
#CPPFLAGS += -D M_DEFINED_STORE_TYPE=YAML_STORE
#CXXFLAGS += -save-temps
#CPPFLAGS += -D DEBUG_PRINT
OBJS = config_manager.o \
config_manager_util.o \
config_manager_setdef_null.o \
config_manager_store.o \
nvram.o \
config_manager_printf.o

OBJS += config_manager_tlv_store.o config_manager_tlv.o

#OBJS += config_manager_yaml_store.o config_manager_yaml.o

# -cr means
# c: create the archive (without this option, if archive doesn't exist, there's a warning)
# r: insert the files into archive (with replacement)
# We only pass newer prerequisites to the command ($? instead of $^)
libCfgMan.a: $(OBJS)
	ar cr $@ $?

.PHONY: clean
clean:
	rm -f *o

# Auto-generate dependencies using gcc's MM option -- see 'Managing Projects with GNU Make'
# But don't include if the target is 'clean'
ifneq "$(MAKECMDGOALS)" "clean"
include $(subst .o,.d,$(OBJS))
endif

%.d: %.cpp
	$(CXX) -MM $(CXXFLAGS) $< > $@.$$$$; \
	sed 's,\($*\)\.o[ :]*,\1.o $@ : ,g' < $@.$$$$ > $@; \
	rm -f $@.$$$$

