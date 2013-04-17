CPPUTEST_HOME = ../../../CppUTest3.3
CXXFLAGS = -Wall -g -I $(CPPUTEST_HOME)/include -I .. -I ../../export -I ../../stubs
# Include some C99 macros that are not part of the C++ standard
CXXFLAGS += -D __STDC_LIMIT_MACROS
CXXFLAGS += -D M_DEFINED_STORE_TYPE=TLV_STORE
#CXXFLAGS += -D M_DEFINED_STORE_TYPE=YAML_STORE
#CXXFLAGS += -save-temps
#CXXFLAGS += -D DEBUG_PRINT
VPATH = .. $(CPPUTEST_HOME)/lib # so make can find the sources and library
MOCK_OBJS = ../../stubs/nvram_spy.o ../../stubs/config_manager_printf_spy.o
TEST_OBJS = AllTests.o \
 cm_tlvTest.o \
 cm_simple_item_descriptorTest.o \
 config_managerTest.o \
 cm_composite_item_descriptorTest.o

# Execute tests, only if executable is new -- the empty tag file timestamps the last time the test ran
testTag: cm_test
	./cm_test
	touch testTag

# Make unit tests, using CppUTest framework
# Note that make has special support for the -l<NAME> syntax: it looks for a library called lib<NAME>.a
# or lib<NAME>.so
cm_test: $(MOCK_OBJS) $(TEST_OBJS) -lCppUTest -lCfgMan 
	$(CXX) $(CXXFLAGS) $^ -o $@

.PHONY: clean
clean:
	rm -f ../*o
	rm -f *o
	rm -f ../../stubs/*o


# Auto-generate dependencies using gcc's MM option -- see 'Managing Projects with GNU Make'
include config_manager_setdef_null.d config_manager.d nvram.d config_manager_yaml.d \
 config_manager_tlv.d config_manager_yaml_store.d config_manager_tlv_store.d \
 config_manager_store.d config_manager_util.d \
 cm_simple_item_descriptorTest.d cm_composite_item_descriptorTest.d config_managerTest.d

%.d: %.cpp
	$(CXX) -MM $(CXXFLAGS) $< > $@.$$$$; \
	sed 's,\($*\)\.o[ :]*,\1.o $@ : ,g' < $@.$$$$ > $@; \
	rm -f $@.$$$$


