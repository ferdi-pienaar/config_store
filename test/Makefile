#CXXFLAGS = -Wall -g -I ../../CppUnitLite -I .. -DDEBUG_PRINT # -save-temps
CXXFLAGS = -Wall -g -I ../../CppUnitLite -I .. # -save-temps

all: cm_simple_item_descriptorTest cm_composite_item_descriptorTest config_managerTest

# Make unit tests, using CppUnitLite framework.  @todo: We SHOULD compile CppUnitLite as a static library.
cm_simple_item_descriptorTest: ../config_manager.o ../config_manager_util.o ../config_manager_tlv.o \
    ../../CppUnitLite/Failure.o ../../CppUnitLite/Test.o ../../CppUnitLite/TestRegistry.o ../../CppUnitLite/TestResult.o ../../CppUnitLite/SimpleString.o

cm_composite_item_descriptorTest: ../config_manager.o ../config_manager_util.o ../config_manager_tlv.o \
    ../../CppUnitLite/Failure.o ../../CppUnitLite/Test.o ../../CppUnitLite/TestRegistry.o ../../CppUnitLite/TestResult.o ../../CppUnitLite/SimpleString.o
               
config_managerTest: ../config_manager.o ../config_manager_util.o ../config_manager_tlv.o \
    ../../CppUnitLite/Failure.o ../../CppUnitLite/Test.o ../../CppUnitLite/TestRegistry.o ../../CppUnitLite/TestResult.o ../../CppUnitLite/SimpleString.o

