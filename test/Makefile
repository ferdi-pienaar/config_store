#CXXFLAGS = -Wall -g -I ../../CppUnitLite -I .. -DDEBUG_PRINT # -save-temps
CXXFLAGS = -Wall -g -I ../../CppUnitLite -I .. # -save-temps
VPATH = ../../CppUnitLite # so make can find the library
OBJS = ../config_manager.o ../config_manager_util.o ../config_manager_tlv.o

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

# Make unit tests, using CppUnitLite framework
cm_simple_item_descriptorTest: $(OBJS) cm_simple_item_descriptorTest.o -lCppUnitLite
	$(CXX) $(CXXFLAGS) $^ -o $@

cm_composite_item_descriptorTest: $(OBJS) cm_composite_item_descriptorTest.o -lCppUnitLite
	$(CXX) $(CXXFLAGS) $^ -o $@

config_managerTest: $(OBJS) config_managerTest.o -lCppUnitLite
	$(CXX) $(CXXFLAGS) $^ -o $@

# Dependencies, manually arrived at!
# None of the test .o depend on dbg.h
# config_manager.o is dependent on all h files
# util is not dependent on dbg.h
# tlv is not dependent on util.h
cm_simple_item_descriptorTest.o: cm_simple_item_descriptorTest.cpp \
 ../config_manager.h ../config_manager_types.h ../config_manager_tlv.h ../config_manager_util.h

cm_composite_item_descriptorTest.o: cm_composite_item_descriptorTest.cpp \
 ../config_manager.h ../config_manager_types.h ../config_manager_tlv.h ../config_manager_util.h

config_managerTest.o: config_managerTest.cpp \
 ../config_manager.h  ../config_manager_types.h ../config_manager_tlv.h ../config_manager_util.h

config_manager.o: config_manager.cpp \
 ../config_manager.h ../config_manager_types.h ../config_manager_tlv.h ../config_manager_dbg.h ../config_manager_util.h

config_manager_util.o: config_manager_util.cpp \
 ../config_manager.h ../config_manager_types.h ../config_manager_tlv.h ../config_manager_util.h

config_manager_tlv.o: config_manager_tlv.cpp \
 ../config_manager.h ../config_manager_types.h ../config_manager_tlv.h ../config_manager_dbg.h


.PHONY: clean
clean:
	rm -f ../*o
	rm -f *o

