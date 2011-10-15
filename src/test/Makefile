CXXFLAGS = -Wall -g -I ../../../CppUTest/include -I .. -I ../../export -I ../../stubs
CXXFLAGS += -D M_DEFINED_STORE_TYPE=TLV_STORE
#CXXFLAGS += -D M_DEFINED_STORE_TYPE=YAML_STORE
#CXXFLAGS += -save-temps
#CXXFLAGS += -D DEBUG_PRINT
VPATH = ../../../CppUTest/lib # so make can find the library
OBJS = ../config_manager.o ../config_manager_util.o \
 ../config_manager_setdef_null.o ../config_manager_store.o ../nvram.o \

OBJS += ../config_manager_tlv_store.o ../config_manager_tlv.o

OBJS += ../config_manager_yaml_store.o ../config_manager_yaml.o

OBJSTLV = ../config_manager_tlv.o ../../stubs/nvram_spy.o

all: simpleTestTag compositeTestTag tlvTestTag cmTestTag

# Execute tests, only if executable is new -- the empty tag files timestamp the last time the test ran
simpleTestTag: cm_simple_item_descriptorTest
	./cm_simple_item_descriptorTest
	touch simpleTestTag

compositeTestTag: cm_composite_item_descriptorTest
	./cm_composite_item_descriptorTest
	touch compositeTestTag

tlvTestTag: cm_tlvTest
	./cm_tlvTest
	touch tlvTestTag

cmTestTag: config_managerTest
	./config_managerTest
	touch cmTestTag

# Make unit tests, using CppUTest framework
cm_simple_item_descriptorTest: $(OBJS) cm_simple_item_descriptorTest.o -lCppUTest
	$(CXX) $(CXXFLAGS) $^ -o $@

cm_composite_item_descriptorTest: $(OBJS) cm_composite_item_descriptorTest.o -lCppUTest
	$(CXX) $(CXXFLAGS) $^ -o $@

config_managerTest: $(OBJS) config_managerTest.o -lCppUTest
	$(CXX) $(CXXFLAGS) $^ -o $@

cm_tlvTest: $(OBJSTLV) cm_tlvTest.o -lCppUTest
	$(CXX) $(CXXFLAGS) $^ -o $@

# Dependencies, manually arrived at!
# None of the test .o depend on dbg.h
# config_manager.o is dependent on all h files
# util is not dependent on dbg.h
# tlv is not dependent on util.h
cm_simple_item_descriptorTest.o: cm_simple_item_descriptorTest.cpp \
 ../../export/config_manager.h ../config_manager_types.h ../config_manager_tlv.h ../config_manager_util.h

cm_composite_item_descriptorTest.o: cm_composite_item_descriptorTest.cpp \
 ../../export/config_manager.h ../config_manager_types.h ../config_manager_tlv.h ../config_manager_util.h

config_managerTest.o: config_managerTest.cpp \
 ../../export/config_manager.h  ../config_manager_types.h ../config_manager_tlv.h ../config_manager_util.h

config_manager.o: config_manager.cpp \
 ../../export/config_manager.h ../config_manager_types.h ../config_manager_tlv.h ../config_manager_dbg.h ../config_manager_util.h

config_manager_util.o: config_manager_util.cpp \
 ../config_manager_types.h ../config_manager_util.h

config_manager_store.o: config_manager_store.cpp \
 ../config_manager_store.h  ../config_manager_tlv_store.h ../config_manager_yaml_store.h ../config_manager_dbg.h

config_manager_tlv_store.o: config_manager_tlv_store.cpp \
 ../config_manager_store.h  ../config_manager_tlv_store.h ../config_manager_dbg.h

config_manager_yaml_store.o: config_manager_yaml_store.cpp \
 ../config_manager_store.h  ../config_manager_yaml_store.h ../config_manager_dbg.h

config_manager_tlv.o: config_manager_tlv.cpp \
 ../../export/config_manager.h ../config_manager_types.h ../config_manager_tlv.h ../config_manager_dbg.h

config_manager_yaml.o: config_manager_yamlt.cpp \
 ../../export/config_manager.h ../config_manager_types.h ../config_manager_yaml.h ../config_manager_dbg.h

config_manager_setdef_null.o: config_manager_setdef_null.cpp \
 ../config_manager_types.h ../config_manager_setdef_null.h

nvram.o: nvram.cpp

.PHONY: clean
clean:
	rm -f ../*o
	rm -f *o
	rm -f ../../stubs/*o

