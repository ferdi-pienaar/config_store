// Unit test using open-source unit test framework
// These tests use the cm_simple_descriptor's public interface
// to test it, and a spy to verify what the CUT prints to its console.
// We also read/write the items themselves. 
//

#include "CppUTest/TestHarness.h"
#include "config_manager.h"  // Unit under test
#include "config_manager_util.h"     // Extensions to unit under test (generic "set" functions)
#include "config_manager_printf_spy.h"
#include <string.h> // strncmp

#include <string>
using namespace std;


// setdef function used in tests
void setint11(uint8_t *pItem, cm_item_len_t len)
{
    // Sanity check
    assert(len == sizeof(int));

    *((int *)pItem) = 11;
}


TEST_GROUP(cm_simple_descriptor)
{
    //Define data accessible to test group members here.
    void setup()
    {
        //initialization steps are executed before each TEST
    }
    
    void teardown()
    {
        //clean up steps are executed after each TEST
    }
};


TEST(cm_simple_descriptor, getLen)
{
    cm_simple_metadata d_d = {{"d01", 1 , 55, true}, NULL, NULL, NULL};

    cm_simple_descriptor d(&d_d);
    LONGS_EQUAL(55, d.getLen());
}


TEST(cm_simple_descriptor, print)
{
    string prefix = "";
    unsigned mem = 7;
    char outstring[64];
    cm_simple_metadata d_d = {{"d01", 1 , sizeof(mem), true}, NULL, NULL, NULL};

    cm_simple_descriptor d(&d_d);
    d.print((uint8_t *)&mem, prefix);
    STRCMP_EQUAL("= 07000000\n", cm_printf_spy_get());
}


TEST(cm_simple_descriptor, set)
{
    int mem = 0;
    cm_simple_metadata d_d = {{"d01", 1 , sizeof(mem), true}, cm_set_int, NULL, NULL};

    cm_simple_descriptor d(&d_d);
    
    d.set((uint8_t *)&mem, "4");
    LONGS_EQUAL(4, mem);
}


// Check setdef() calls the function installed in metadata
TEST(cm_simple_descriptor, setdefFunc)
{
    int mem = 8;
    cm_simple_metadata d_d = {{"d01", 1 , sizeof(mem), true}, NULL, setint11, NULL};

    cm_simple_descriptor d(&d_d);
    
    d.setDefault((uint8_t *)&mem);
    LONGS_EQUAL(11, mem);
}


// Check setdef() does nothing if there's no setdef function installed in metadata
TEST(cm_simple_descriptor, setdef)
{
    int mem = 8;
    cm_simple_metadata d_d = {{"d01", 1 , sizeof(mem), true}, NULL, NULL, NULL};

    cm_simple_descriptor d(&d_d);
    
    d.setDefault((uint8_t *)&mem);
    LONGS_EQUAL(8, mem);
}


