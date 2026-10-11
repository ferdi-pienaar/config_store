// Unit test using open-source unit test framework
// These tests the config manager as a whole, using the following interfaces:
// Input: character strings passed to Config_manager_implement::handleCmd
// Output:
//  Data stored in RAM.
//  Out to printf_spy, i.e. capture output to operator from cfg_mgr (not currently used)
// These tests are called regardless of the format of the underlying store: TLV or JSON.
//

#include "gtest/gtest.h"
#include "cfg_mgr_implement.h"       // Unit under test
#include "cfg_mgr_simple_descriptor.h"       // Unit under test
#include "cfg_mgr_composite_descriptor.h"       // Unit under test
#include "cfg_mgr_contained_aggregate.h"       // Unit under test
#include "cfg_mgr_owned_aggregate.h"       // Unit under test
#include "cfg_mgr_set_int.h"  // Extensions to unit under test (generic "set" functions)
#include "cfg_mgr_prt_int.h"  // Extensions to unit under test (generic "print" functions)
#include "cfg_mgr_set_str.h"  // Extensions to unit under test (generic "set" functions)
#include "cfg_mgr_prt_str.h"  // Extensions to unit under test (generic "print" functions)
#include "cfg_mgr_printf_spy.h"
#include "cfg_mgr_setdef_null.h" // generic setdef function
#include "nvram_spy.h"
#include <string>
#include <vector>

using namespace std;
using namespace cfg_mgr;

namespace {

////////////////////////////////////////////////////////////////////////////////
// test set 1, CONTAINED
// test set 1 data structure
static constexpr unsigned STRING_ARRAY_LEN = 8;
static constexpr unsigned ARRAY_LEN = 2;
struct m
{
    char m1[STRING_ARRAY_LEN]; // String is represented as an array, with metadata: size=len, and 1 instance in aggregate data.
    short int m2[ARRAY_LEN];
};

// test set 1 user-defined functions
void setdef_t1(uint8_t *pItem, item_len_t len)
{
    // Sanity check
    assert(len == sizeof(short int));

    *((short int *)pItem) = 7;
}


// test set 1 metadata
const Simple_metadata s1_d = {{"name1", 1, sizeof(char) * STRING_ARRAY_LEN, false}, cm_set_str, cm_setdef_null, nullptr};
Simple_descriptor s1(&s1_d);
const Aggregate_data ca1_d = {&s1, 1, offsetof(struct m, m1)};
Contained_aggregate ca1(&ca1_d);

const Simple_metadata s2_d = {{"name2", 2, sizeof(short int), true}, cm_set_int, setdef_t1, cm_prt_int};
Simple_descriptor s2(&s2_d);
const Aggregate_data ca2_d = {&s2, ARRAY_LEN, offsetof(struct m, m2)};
Contained_aggregate ca2(&ca2_d);

Aggregate * const aggrList1[] = {&ca1, &ca2};
const Composite_metadata c1_d = {{"c1", 1, sizeof(struct m), true}, aggrList1, sizeof(aggrList1)/sizeof(aggrList1[0])};
Composite_descriptor c1(&c1_d);

struct m * C1_CONFIG = nullptr;

class CfgMgrContainedHelp : public testing::Test
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

TEST_F(CfgMgrContainedHelp, help)
{
    std::vector<char *> help_cmd = {(char *)"?"};
    cm->handleCmd(help_cmd.size(), help_cmd.data());

    EXPECT_STREQ("c1 len 12 persistent\n  name1 [1] len 8 not persistent\n  name2 [2] len 2 persistent\n", cm_printf_spy_get());
}

TEST_F(CfgMgrContainedHelp, help_component)
{
    std::vector<char *> help_cmd = {(char *)"?", (char *)"name2"};
    cm->handleCmd(help_cmd.size(), help_cmd.data());

    EXPECT_STREQ("name2 len 2 persistent\n", cm_printf_spy_get());
}

TEST_F(CfgMgrContainedHelp, help_depth)
{
    std::vector<char *> help_cmd = {(char *)"?", (char *)"depth", (char *)"1"};
    cm->handleCmd(help_cmd.size(), help_cmd.data());

    EXPECT_STREQ("c1 len 12 persistent\n", cm_printf_spy_get());
}

TEST_F(CfgMgrContainedHelp, help_invalid_component)
{
    std::vector<char *> help_cmd = {(char *)"?", (char *)"XXX"};
    cm->handleCmd(help_cmd.size(), help_cmd.data());

    //std::cout << cm_printf_spy_get();
    EXPECT_EQ(0, strncmp("Invalid", cm_printf_spy_get(), 7));
}

TEST_F(CfgMgrContainedHelp, help_invalid_depth)
{
    std::vector<char *> help_cmd = {(char *)"?", (char *)"depth", (char *)"all"};
    cm->handleCmd(help_cmd.size(), help_cmd.data());

    //std::cout << cm_printf_spy_get();
    EXPECT_EQ(0, strncmp("Invalid", cm_printf_spy_get(), 7));
}

} // namespace
