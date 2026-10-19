// Unit test using open-source unit test framework
// These tests the config manager as a whole, using the following interfaces:
// Input: character strings passed to Config_manager_implement::handleCmd
// Output:
//  Data stored in RAM.
//  Out to printf_spy, i.e. capture output to operator from cfg_mgr (not currently used)
// These tests are called 'common' because they test regardless of the format
// of the underlying store: TLV or JSON.
//

#include "gtest/gtest.h"
#include "cfg_mgr_implement.h"       // Unit under test
#include "cfg_mgr_printf_spy.h"
#include "cfg_mgr_setdef_null.h" // generic setdef function
#include "nvram_spy.h"
#include <vector>
#include "cfg_mgr_common_test.h"

using namespace std;
using namespace cfg_mgr;

////////////////////////////////////////////////////////////////////////////////
// test set 1, CONTAINED
// test set 1 data structure

// test set 1 user-defined functions
void setdef_t1(uint8_t *pItem, item_len_t len)
{
    // Sanity check
    assert(len == sizeof(short int));

    *((short int *)pItem) = 7;
}

struct m * C1_CONFIG = nullptr;

class CfgMgrContainedCommon : public testing::Test
{
protected:
    //Define data accessible to test group members here.
    unique_ptr<Nvram_spy> nvram;
    unique_ptr<Config_manager_implement> cm;

    virtual void SetUp()
    {
        nvram = make_unique<Nvram_spy>();
        cm = make_unique<Config_manager_implement>(&c1, cm_printf_spy, (uint8_t **)&C1_CONFIG, nvram.get());
        cm_printf_spy_init();
    }
};


// Verify default value (from function in metadata) is in RAM.
TEST_F(CfgMgrContainedCommon, default_value)
{
    EXPECT_EQ(7, C1_CONFIG->m2[0]);
    EXPECT_EQ(7, C1_CONFIG->m2[1]);
}

// Verify value modified by command is in RAM.
TEST_F(CfgMgrContainedCommon, modify)
{
    std::vector<char *> command = {(char *)"name1", (char *)"=", (char *)"\"fred\""};
    cm->handleCmd(command.size(), command.data());

    //std::cout << cm_printf_spy_get();
    EXPECT_STREQ("fred", C1_CONFIG->m1);
}

// String variables must be surrounded by quotes in cmd.
TEST_F(CfgMgrContainedCommon, set_str_invalid_start)
{
    std::vector<char *> command = {(char *)"name1", (char *)"=", (char *)"fred"};
    cm->handleCmd(command.size(), command.data());

    //std::cout << cm_printf_spy_get();
    EXPECT_EQ(0, strncmp("Invalid", cm_printf_spy_get(), 7));
}

// String variables must be surrounded by quotes in cmd.
TEST_F(CfgMgrContainedCommon, set_str_invalid_end)
{
    std::vector<char *> command = {(char *)"name1", (char *)"=", (char *)"\"fred"};
    cm->handleCmd(command.size(), command.data());

    //std::cout << cm_printf_spy_get();
    EXPECT_EQ(0, strncmp("Invalid", cm_printf_spy_get(), 7));
}

// Verify modified value is restored after save, modify, and restore operations.
TEST_F(CfgMgrContainedCommon, save_modify_and_restore)
{
    // Check default value, set by constructor of Config_manager_implement.
    std::vector<char *> command0 = {(char *)"name2", (char *)"0", (char *)"=", (char *)"11"};
    cm->handleCmd(command0.size(), command0.data());
    EXPECT_EQ(11, C1_CONFIG->m2[0]);

    std::vector<char *> command = {(char *)"save"};
    cm->handleCmd(command.size(), command.data());

    std::vector<char *> command2 = {(char *)"name2", (char *)"0", (char *)"=", (char *)"115"};
    cm->handleCmd(command2.size(), command2.data());
    EXPECT_EQ(115, C1_CONFIG->m2[0]);

    std::vector<char *> command3 = {(char *)"load"};
    cm->handleCmd(command3.size(), command3.data());
    EXPECT_EQ(11, C1_CONFIG->m2[0]);
}

TEST_F(CfgMgrContainedCommon, set_integer_out_of_range)
{
    std::vector<char *> command = {(char *)"name2", (char *)"0",  (char *)"=", (char *)"0x10000"};
    cm->handleCmd(command.size(), command.data());

    //std::cout << cm_printf_spy_get();
    EXPECT_EQ(0, strncmp("Invalid", cm_printf_spy_get(), 7));
    EXPECT_EQ(7, C1_CONFIG->m2[0]);
}

TEST_F(CfgMgrContainedCommon, set_integer_not_an_integer)
{
    std::vector<char *> command = {(char *)"name2", (char *)"0",  (char *)"=", (char *)"siete"};
    cm->handleCmd(command.size(), command.data());

    //std::cout << cm_printf_spy_get();
    EXPECT_EQ(0, strncmp("Invalid", cm_printf_spy_get(), 7));
    EXPECT_EQ(7, C1_CONFIG->m2[0]);
}

// The components with prt functions print, and those without prt function don't print.
TEST_F(CfgMgrContainedCommon, print_composite_with_unprintable_component)
{
    std::vector<char *> command = {(char *)"prt"};
    cm->handleCmd(command.size(), command.data());

    EXPECT_STREQ("name2 0 = 7\nname2 1 = 7\n", cm_printf_spy_get());
}
