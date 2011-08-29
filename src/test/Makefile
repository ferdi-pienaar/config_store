#CXXFLAGS = -Wall -g -I ../../../CppUTest/include -I .. -DDEBUG_PRINT # -save-temps
CXXFLAGS = -Wall -g -I ../../../CppUTest/include -I .. -I ../../export # -save-temps
VPATH = ../../../CppUTest/lib # so make can find the library
OBJS = ../config_manager.o ../config_manager_util.o ../config_manager_tlv.o \
 ../config_manager_setdef_null.o ../nvram.o

all: simpleTestTag compositeTestTag cmTestTag

# Execute tests, only if executable is new -- the empty tag files timestamp the last time the test ran
simpleTestTag: cm_simple_item_descriptorTest
	./cm_simple_item_descriptorTest
	touch simpleTestTag

compositeTestTag: cm_composite_item_descriptorTest
	./cm_composite_item_descriptorTest
	touch compositeTestTag

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

config_manager_tlv.o: config_manager_tlv.cpp \
 ../../export/config_manager.h ../config_manager_types.h ../config_manager_tlv.h ../config_manager_dbg.h

config_manager_setdef_null.o: config_manager_setdef_null.cpp \
 ../config_manager_types.h ../config_manager_setdef_null.h

nvram.o: nvram.cpp

.PHONY: clean
clean:
	rm -f ../*o
	rm -f *o

