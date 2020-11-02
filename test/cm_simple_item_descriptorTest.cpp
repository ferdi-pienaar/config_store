// Unit test using open-source unit test framework
// These tests use the Simple_descriptor's public interface
// to test it, and a spy to verify what the CUT prints to its console.
// We also read/write the items themselves.
//

#include "gtest/gtest.h"
#include "cfg_mgr_simple_descriptor.h"  // Unit under test
#include "cfg_mgr_set_int.h"     // Extensions to unit under test (generic "set" functions)
#include "cfg_mgr_printf_spy.h"
#include <cstring> // strncmp

#include <string>
using namespace std;
using namespace cfg_mgr;

// setdef function used in tests
void setint11(uint8_t *pItem, item_len_t len)
{
    // Sanity check
    assert(len == sizeof(int));

    *((int *)pItem) = 11;
}

namespace {

class SimpleDescriptor : public testing::Test
{
protected:
    void SetUp()
    {
        cm_printf_spy_init();
    }
};

TEST_F(SimpleDescriptor, getLen)
{
    Simple_metadata d_d = {{"d01", 1, 55, true}, NULL, NULL, NULL};

    Simple_descriptor d(&d_d);
    EXPECT_EQ(55, d.getLen());
}


TEST_F(SimpleDescriptor, print)
{
    string prefix = "";
    unsigned mem = 7;
    Simple_metadata d_d = {{"d01", 1, sizeof(mem), true}, NULL, NULL, NULL};

    Simple_descriptor d(&d_d);
    d.print((uint8_t *)&mem, prefix, false);
    EXPECT_STREQ("= 07000000\n", cm_printf_spy_get());
}


TEST_F(SimpleDescriptor, set)
{
    int mem = 0;
    Simple_metadata d_d = {{"d01", 1, sizeof(mem), true}, cm_set_int, NULL, NULL};

    Simple_descriptor d(&d_d);

    d.set((uint8_t *)&mem, "4");
    EXPECT_EQ(4, mem);
}


// Check setdef() calls the function installed in metadata
TEST_F(SimpleDescriptor, setdefFunc)
{
    int mem = 8;
    Simple_metadata d_d = {{"d01", 1, sizeof(mem), true}, NULL, setint11, NULL};

    Simple_descriptor d(&d_d);

    d.setDefault((uint8_t *)&mem);
    EXPECT_EQ(11, mem);
}


// Check setdef() does nothing if there's no setdef function installed in metadata
TEST_F(SimpleDescriptor, setdef)
{
    int mem = 8;
    Simple_metadata d_d = {{"d01", 1, sizeof(mem), true}, NULL, NULL, NULL};

    Simple_descriptor d(&d_d);

    d.setDefault((uint8_t *)&mem);
    EXPECT_EQ(8, mem);
}
} // namespace

