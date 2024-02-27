// Unit test using open-source unit test framework
// These tests the config manager as a whole, using the following interfaces:
// 1. Input: character strings passed to Config_manager_implement::handleCmd
// xxx
// These tests are called 'common' because they test regardless of the format
// of the underlying store: TLV or JSON.
//

#include "gtest/gtest.h"
#include "cfg_mgr_implement.h"       // Unit under test
#include "cfg_mgr_simple_descriptor.h"       // Unit under test
#include "cfg_mgr_composite_descriptor.h"       // Unit under test
#include "cfg_mgr_contained_aggregate.h"       // Unit under test
#include "cfg_mgr_owned_aggregate.h"       // Unit under test
#include "cfg_mgr_set_int.h"  // Extensions to unit under test (generic "set" functions)
#include "cfg_mgr_prt_int.h"  // Extensions to unit under test (generic "print" functions)
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
struct m
{
    int m1;
    int m2;
};

// test set 1 user-defined functions
void setdef_t1(uint8_t *pItem, item_len_t len)
{
    // Sanity check
    assert(len == sizeof(int));

    *((int *)pItem) = 7;
}


// test set 1 metadata
const Simple_metadata s1_d = {{"name1", 1, sizeof(int), true}, cm_set_int, cm_setdef_null, cm_prt_int};
const Simple_descriptor s1(&s1_d);
const Aggregate_data ca1_d = {&s1, 1, offsetof(struct m, m1)};
const Contained_aggregate ca1(&ca1_d);

const Simple_metadata s2_d = {{"name2", 2, sizeof(int), true}, cm_set_int, setdef_t1, cm_prt_int};
const Simple_descriptor s2(&s2_d);
const Aggregate_data ca2_d = {&s2, 1, offsetof(struct m, m2)};
const Contained_aggregate ca2(&ca2_d);

const Aggregate * const aggrList1[] = {&ca1, &ca2};
const Composite_metadata c1_d = {{"c1", 1, sizeof(struct m), true}, aggrList1, sizeof(aggrList1)/sizeof(aggrList1[0])};
const Composite_descriptor c1(&c1_d);

#define GET_C1_CONFIG ((struct m *)cm->getConfig())

class CfgMgrContainedCommon : public testing::Test
{
protected:
    Nvram_spy * nvram;
    Config_manager_implement * cm;

    //Define data accessible to test group members here.
    virtual void SetUp()
    {
        nvram = new Nvram_spy;
        cm = new Config_manager_implement(&c1, nvram);
    }

    virtual void TearDown()
    {
        delete cm;
        delete nvram;
    }
};


// Verify default value (from function in metadata) is in RAM.
TEST_F(CfgMgrContainedCommon, default_value)
{
    EXPECT_EQ(7, GET_C1_CONFIG->m2);
}

// Verify value modified by command is in RAM.
TEST_F(CfgMgrContainedCommon, modify)
{
    std::vector<char *> command = {(char *)"name2", (char *)"=", (char *)"11"};
    cm->handleCmd(command.size(), command.data());
    EXPECT_EQ(11, GET_C1_CONFIG->m2);
}

// Verify modified value is restored after save, modify, and restore operations.
TEST_F(CfgMgrContainedCommon, save_modify_and_restore)
{
    // Check default value, set by constructor of Config_manager_implement.
    std::vector<char *> command0 = {(char *)"name2", (char *)"=", (char *)"11"};
    cm->handleCmd(command0.size(), command0.data());
    EXPECT_EQ(11, GET_C1_CONFIG->m2);

    std::vector<char *> command = {(char *)"save"};
    cm->handleCmd(command.size(), command.data());

    std::vector<char *> command2 = {(char *)"name2", (char *)"=", (char *)"115"};
    cm->handleCmd(command2.size(), command2.data());
    EXPECT_EQ(115, GET_C1_CONFIG->m2);

    std::vector<char *> command3 = {(char *)"load"};
    cm->handleCmd(command3.size(), command3.data());
    EXPECT_EQ(11, GET_C1_CONFIG->m2);
}

} // namespace
