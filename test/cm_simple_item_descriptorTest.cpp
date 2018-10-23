// Unit test using open-source unit test framework
// These tests use the cm_simple_descriptor's public interface
// to test it, and a spy to verify what the CUT prints to its console.
// We also read/write the items themselves. 
//

#include "gtest/gtest.h"
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

namespace {
    
class CmSimpleDescriptor : public testing::Test 
{
protected:   
    void SetUp()
    {
        cm_printf_spy_init();
    }
};

TEST_F(CmSimpleDescriptor, getLen)
{
    cm_simple_metadata d_d = {{"d01", 1 , 55, true}, NULL, NULL, NULL};

    cm_simple_descriptor d(&d_d);
    EXPECT_EQ(55, d.getLen());
}


TEST_F(CmSimpleDescriptor, print)
{
    string prefix = "";
    unsigned mem = 7;
    cm_simple_metadata d_d = {{"d01", 1 , sizeof(mem), true}, NULL, NULL, NULL};

    cm_simple_descriptor d(&d_d);
    d.print((uint8_t *)&mem, prefix, false);
    EXPECT_STREQ("= 07000000\n", cm_printf_spy_get());
}


TEST_F(CmSimpleDescriptor, set)
{
    int mem = 0;
    cm_simple_metadata d_d = {{"d01", 1 , sizeof(mem), true}, cm_set_int, NULL, NULL};

    cm_simple_descriptor d(&d_d);
    
    d.set((uint8_t *)&mem, "4");
    EXPECT_EQ(4, mem);
}


// Check setdef() calls the function installed in metadata
TEST_F(CmSimpleDescriptor, setdefFunc)
{
    int mem = 8;
    cm_simple_metadata d_d = {{"d01", 1 , sizeof(mem), true}, NULL, setint11, NULL};

    cm_simple_descriptor d(&d_d);
    
    d.setDefault((uint8_t *)&mem);
    EXPECT_EQ(11, mem);
}


// Check setdef() does nothing if there's no setdef function installed in metadata
TEST_F(CmSimpleDescriptor, setdef)
{
    int mem = 8;
    cm_simple_metadata d_d = {{"d01", 1 , sizeof(mem), true}, NULL, NULL, NULL};

    cm_simple_descriptor d(&d_d);
    
    d.setDefault((uint8_t *)&mem);
    EXPECT_EQ(8, mem);
}
} // namespace

