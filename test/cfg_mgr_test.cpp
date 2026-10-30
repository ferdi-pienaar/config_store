// Unit test using open-source unit test framework
// These tests the config manager as a whole, using the following interfaces:
// 1. Input: character strings passed to Config_manager_implement::handleCmd
// 2. Input/output: the binary file containing TLV data used by Config_manager_implement
//    for non-volatile storage
// 3. Output to the operator via the print function passed by the test, cm_printf_spy.
// xxx how many of these tests belong in cm_tlvTest.cpp?
//

#include "gtest/gtest.h"
#include "cfg_mgr_implement.h"       // Unit under test
#include "cfg_mgr_simple_descriptor.h"       // Unit under test
#include "cfg_mgr_composite_descriptor.h"       // Unit under test
#include "cfg_mgr_contained_aggregate.h"       // Unit under test
#include "cfg_mgr_owned_aggregate.h"       // Unit under test
#include "item_identifier.h"       // Unit under test
#include "cfg_mgr_prt_int.h"  // Extensions to unit under test (generic "print" functions)
#include "cfg_mgr_prt_str.h"  // Extensions to unit under test (generic "print" functions)
#include "cfg_mgr_prt_hexstr.h"  // Extensions to unit under test (generic "print" functions)
#include "cfg_mgr_set_int.h"  // Extensions to unit under test (generic "set" functions)
#include "cfg_mgr_set_str.h"  // Extensions to unit under test (generic "set" functions)
#include "cfg_mgr_setdef_null.h" // generic setdef function
#include "nvram_spy.h"
#include "cfg_mgr_printf_spy.h"

#include <string>
#include <cstring> // memcmp, strncmp, etc

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
const Simple_metadata s1_d = {{"name1", 1, sizeof(int)}, nullptr, cm_setdef_null, cm_prt_hexstr};
Simple_descriptor s1(&s1_d);
const Aggregate_data ca1_d = {&s1, 1, offsetof(struct m, m1)};
Contained_aggregate ca1(&ca1_d);

const Simple_metadata s2_d = {{"name2", 2, sizeof(int)}, cm_set_int, setdef_t1, cm_prt_int};
Simple_descriptor s2(&s2_d);
const Aggregate_data ca2_d = {&s2, 1, offsetof(struct m, m2)};
Contained_aggregate ca2(&ca2_d);

Aggregate * const aggrList1[] = {&ca1, &ca2};
const Composite_metadata c1_d = {{"c1", 1, sizeof(struct m)}, aggrList1, sizeof(aggrList1)/sizeof(aggrList1[0])};
Composite_descriptor c1(&c1_d);

struct m * C1_CONFIG = nullptr;

class CfgMgrContained : public testing::Test
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

// An item whose descriptor doesn't have an assigned set function: command to set it is invalid.
TEST_F(CfgMgrContained, set_item_with_no_set_fn)
{
    char * commandWord[] = {(char *)"name1", (char *)"=", (char *)"71"};
    cm->handleCmd(3, commandWord);

    // The output to the user starts with "Invalid".
    EXPECT_EQ(0, strncmp("Invalid", cm_printf_spy_get(), 7));
}

// Verify data set with cmd '=' is printed to spy on cmd 'prt'.
TEST_F(CfgMgrContained, set_simple_and_print)
{
    char * commandWord[] = {(char *)"name2", (char *)"=", (char *)"71"};
    cm->handleCmd(3, commandWord);
    // Change context.
    cm->handleCmd(1, commandWord);
    char * commandPrt[] = {(char *)"prt"};
    cm->handleCmd(1, commandPrt);

    EXPECT_STREQ("= 71\n", cm_printf_spy_get());
}

// Invalid incomplete cmd.
TEST_F(CfgMgrContained, set_simple_without_value)
{
    char * commandWord[] = {(char *)"name2", (char *)"="};
    cm->handleCmd(2, commandWord);

    //std::cout << cm_printf_spy_get();
    EXPECT_EQ(0, strncmp("Invalid", cm_printf_spy_get(), 7));
}

// Verify cmd with invalid word (neither an operation nor a component's name) is rejected.
TEST_F(CfgMgrContained, invalid_cmd_word)
{
    char * commandWord[] = {(char *)"name3", (char *)"=", (char *)"71"};
    cm->handleCmd(3, commandWord);

    // The output to the user starts with "Invalid".
    EXPECT_EQ(0, strncmp("Invalid", cm_printf_spy_get(), 7));
}

// Use command setdef and print to verify that values are from the installed setdef functions.
TEST_F(CfgMgrContained, setdef)
{
    char * commandWord[] = {(char *)"setdef"};
    cm->handleCmd(1, commandWord);
    char * commandPrt[] = {(char *)"prt"};
    cm->handleCmd(1, commandPrt);

    EXPECT_STREQ("name1 = 00000000\nname2 = 7\n", cm_printf_spy_get());
}

// Use command setdef directly on simple component.
TEST_F(CfgMgrContained, setdef_simple)
{
    char * commandWord[] = {(char *)"name2", (char *)"=", (char *)"71"};
    cm->handleCmd(3, commandWord);
    EXPECT_EQ(71, C1_CONFIG->m2);

    char * commandSetdef[] = {(char *)"name2", (char *)"setdef"};
    cm->handleCmd(2, commandSetdef);

    EXPECT_EQ(7, C1_CONFIG->m2);
}

// Use command prtc directly on simple component.
TEST_F(CfgMgrContained, print_config_simple)
{
    char * commandPrtc[] = {(char *)"name2", (char *)"prtc"};
    cm->handleCmd(2, commandPrtc);

    EXPECT_STREQ("= 7\n", cm_printf_spy_get());
}

// Verify data saved to TLV, with default data in RAM as input to the test.
TEST_F(CfgMgrContained, save)
{
    uint8_t expectedTlv [20] =
    { 1,0, 16,0, 1,0, 4,0, 0,0,0,0, 2,0, 4,0, 7,0,0,0};
    /*T    L     T    L    V        T    L    V     */
    /* Assumes little-endian integers */

    char * commandWord[] = {(char *)"save"};
    cm->handleCmd(1, commandWord);

    EXPECT_TRUE(nvram->match(expectedTlv, sizeof(expectedTlv)));
}

// Load TLV file and verify what CUT saves in RAM.
TEST_F(CfgMgrContained, load)
{
    uint8_t tlv[20] =
    { 1,0, 16,0, 1,0, 4,0, 8,0,0,0, 2,0, 4,0, 9,0,0,0};
    /*T    L     T    L    V        T    L    V    */

    /* Create config file to be loaded */
    nvram->set(tlv, sizeof(tlv));

    char * commandWord[] = {(char *)"load"};
    cm->handleCmd(1, commandWord);

    EXPECT_EQ(8, C1_CONFIG->m1);
    EXPECT_EQ(9, C1_CONFIG->m2);
}

// Verify what's loaded into memory, given TLV file with L of 2nd simple
// component that doesn't match the item descriptor.
TEST_F(CfgMgrContained, loadChangedSimpleLen)
{
    uint8_t tlv[20] =
    { 1,0, 14,0, 1,0, 4,0, 8,0,0,0, 2,0, 2,0, 9,0};
    //T    L     T    L    V        T    L    V
    // Assumes little-endian integers

    /* Create config file to be loaded */
    nvram->set(tlv, sizeof(tlv));

    char * commandWord[] = {(char *)"load"};
    cm->handleCmd(1, commandWord);

    EXPECT_EQ(0, C1_CONFIG->m1); // Default
    EXPECT_EQ(7, C1_CONFIG->m2); // Default; not read from the file, because TLV's L is bad
}

// Verify what's loaded into memory, given a TLV file that's read on startup that contains
// an unknown Type value.
// Unknown type in file: the descriptor has no T=9, so it's ignored by cfg_mgr when found in file,
// but the item following it is loaded.
TEST_F(CfgMgrContained, loadUnknown)
{
    uint8_t tlv[28] =
    { 1,0, 24,0, 1,0, 4,0, 0,0,0,0, 9,0, 4,0, 0,0,0,0, 2,0, 4,0, 0,0,0,0};
    /*T    L     T    L    V        T    L    V        T    L    V    */
    uint8_t expectedTlv[20] =
    { 1,0, 16,0, 1,0, 4,0, 0,0,0,0, 2,0, 4,0, 0,0,0,0};
    /*T    L     T    L    V        T    L    V    */

    /* Create config file to be loaded */
    nvram->set(tlv, sizeof(tlv));

    char * commandWord[] = {(char *)"load"};
    cm->handleCmd(1, commandWord);
    char * commandWord2[] = {(char *)"save"};
    cm->handleCmd(1, commandWord2);

    // See what CM made of the file it loaded
    EXPECT_TRUE(nvram->match(expectedTlv, sizeof(expectedTlv)));
}

// Verify what's saved to TLV, given a TLV file that's read on startup that's
// missing an element of a structure.
// The element m1 that's not in the TLV is saved to TLV, populated with default value.
// The element m2 that is in TLV is restored to TLV.
TEST_F(CfgMgrContained, loadMissing)
{
    uint8_t tlv[12] =
    { 1,0, 8,0, 2,0, 4,0, 6,7,8,9};
    /*T    L    T    L    V    */
    uint8_t expectedTlv[20] =
    { 1,0, 16,0, 1,0, 4,0, 0,0,0,0, 2,0, 4,0, 6,7,8,9};
    /*T    L     T    L    V        T    L    V    */

    /* Create config file to be loaded */
    nvram->set(tlv, sizeof(tlv));
    char * commandWord[] = {(char *)"load"};
    cm->handleCmd(1, commandWord);
    char * commandWord2[] = {(char *)"save"};
    cm->handleCmd(1, commandWord2);

    // See what CM made of the file it loaded
    EXPECT_TRUE(nvram->match(expectedTlv, sizeof(expectedTlv)));
}

// Load file with last item missing from CONTAINED composite
// Load fails, so defaults are set.
TEST_F(CfgMgrContained, loadTruncated)
{
    uint8_t tlv[] =
    { 1,0, 16,0, 1,0, 4,0, 8,0,0,0 /*... */};
    /*T    L     T    L    V       ...    */

    /* Create config file to be loaded */
    nvram->set(tlv, sizeof(tlv));
    char * commandWord[] = {(char *)"load"};
    cm->handleCmd(1, commandWord);
    EXPECT_EQ(0, C1_CONFIG->m1);
    EXPECT_EQ(7, C1_CONFIG->m2);
}

// Load file with partial last item in CONTAINED composite
// Load fails, so defaults are set.
TEST_F(CfgMgrContained, loadTruncated1)
{
    uint8_t tlv[] =
    { 1,0, 16,0, 1,0, 4,0, 8,0,0,0, 2,0};
    //T    L     T    L    V        T ...
    // ssumes little-endian integers

    /* Create config file to be loaded */
    nvram->set(tlv, sizeof(tlv));
    char * commandWord[] = {(char *)"load"};
    cm->handleCmd(1, commandWord);
    EXPECT_EQ(0, C1_CONFIG->m1);
    EXPECT_EQ(7, C1_CONFIG->m2);
}

// Load file with partial last item in CONTAINED composite
// Load fails, so defaults are set.
TEST_F(CfgMgrContained, loadTruncated2)
{
    uint8_t tlv[] =
    { 1,0, 16,0, 1,0, 4,0, 8,0,0,0, 2,0, 4,0, 9,0,0};
    /*T    L     T    L    V        T    L    V (last byte missing) */

    /* Create config file to be loaded */
    nvram->set(tlv, sizeof(tlv));
    char * commandWord[] = {(char *)"load"};
    cm->handleCmd(1, commandWord);
    EXPECT_EQ(0, C1_CONFIG->m1);
    EXPECT_EQ(7, C1_CONFIG->m2);
}

// Load file with incoherent CONTAINED composite (sum of the sizes
// of components is larger than the size of the composite).
// Load fails, so defaults are set.
TEST_F(CfgMgrContained, DISABLED_loadIncoherent)
{
    uint8_t tlv[20] =
    { 1,0, 14,0, 1,0, 4,0, 8,0,0,0, 2,0, 4,0, 9,0,0,0};
    //T    L     T    L    V        T    L    V
    // Assumes little-endian integers

    /* Create config file to be loaded */
    nvram->set(tlv, sizeof(tlv));
    char * commandWord[] = {(char *)"load"};
    cm->handleCmd(1, commandWord);
    EXPECT_EQ(0, C1_CONFIG->m1);
    EXPECT_EQ(7, C1_CONFIG->m2);
}

// Load file with incoherent CONTAINED composite (sum of the sizes
// of components is smaller than the size of the composite).
// Load fails, so defaults are set.
TEST_F(CfgMgrContained, DISABLED_loadIncoherent1)
{
    uint8_t tlv[20] =
    { 1,0, 17,0, 1,0, 4,0, 8,0,0,0, 2,0, 4,0, 9,0,0,0};
    //T    L     T    L    V        T    L    V
    // Assumes little-endian integers

    /* Create config file to be loaded */
    nvram->set(tlv, sizeof(tlv));
    char * commandWord[] = {(char *)"load"};
    cm->handleCmd(1, commandWord);
    EXPECT_EQ(0, C1_CONFIG->m1);
    EXPECT_EQ(7, C1_CONFIG->m2);
}


////////////////////////////////////////////////////////////////////////////////
// test set 2, OWNED.
// test set 2 data structure
#define MAX_NUMBER_OWNED 2
struct m2
{
    unsigned cnt;
    int *    owned;
};

// test set 2 metadata
const Simple_metadata s3_d = {{"count", NOT_PERSISTENT, sizeof(unsigned)}, nullptr, nullptr, cm_prt_int};
Simple_descriptor s3(&s3_d);
const Aggregate_data ca3_d = {&s3, 1, offsetof(struct m2, cnt)};
Contained_aggregate ca3(&ca3_d);

const Simple_metadata s4_d = {{"owned", 4, sizeof(int)}, cm_set_int, nullptr, cm_prt_int};
Simple_descriptor s4(&s4_d);
const Owned_aggregate_data ca4_d = {{&s4, MAX_NUMBER_OWNED, offsetof(struct m2, owned)}, &ca3};
Owned_aggregate oa4(&ca4_d);

Aggregate * const aggrList2[] = {&ca3, &oa4};
const Composite_metadata c2_d = {{"c2", 1, sizeof(struct m2)}, aggrList2, sizeof(aggrList2)/sizeof(aggrList2[0])};
Composite_descriptor c2(&c2_d);

struct m2 * C2_CONFIG = nullptr;

class CfgMgrOwned : public testing::Test
{
protected:
    //Define data accessible to test group members here.
    unique_ptr<Nvram_spy> nvram;
    unique_ptr<Config_manager_implement> cm;

    virtual void SetUp()
    {
        nvram = make_unique<Nvram_spy>();
        cm = make_unique<Config_manager_implement>(&c2, cm_printf_spy, (uint8_t **)&C2_CONFIG, nvram.get());
        cm_printf_spy_init();
    }
};

// Set up data (with implicit add) and print the composite.
TEST_F(CfgMgrOwned, set_and_print)
{
    char * commandWord[] = {(char *)"owned", (char *)"0", (char *)"=", (char *)"73"};
    cm->handleCmd(4, commandWord);
    char * commandPrt[] = {(char *)"prt"};
    cm->handleCmd(1, commandPrt);

    EXPECT_STREQ("count = 1\nowned 0 = 73\n", cm_printf_spy_get());
}

// Set cmd invalid; index larger than max.
TEST_F(CfgMgrOwned, set_index_too_big)
{
    char * commandWord[] = {(char *)"owned", (char *)"9", (char *)"=", (char *)"73"};
    cm->handleCmd(4, commandWord);

    //std::cout << cm_printf_spy_get();
    EXPECT_EQ(0, strncmp("Invalid", cm_printf_spy_get(), 7));
}

// Set cmd invalid; index not provided.
TEST_F(CfgMgrOwned, set_missing_index)
{
    char * commandWord[] = {(char *)"owned", (char *)"=", (char *)"73"};
    cm->handleCmd(3, commandWord);

    //std::cout << cm_printf_spy_get();
    EXPECT_EQ(0, strncmp("Invalid", cm_printf_spy_get(), 7));
}

// Set cmd invalid: add beyond count limit.
TEST_F(CfgMgrOwned, add_too_many)
{
    char * commandAdd[] = {(char *)"add", (char *)"owned"};
    for (int i = 0; i < MAX_NUMBER_OWNED + 1; i++)
    {
        cm->handleCmd(2, commandAdd);
    }

    //std::cout << cm_printf_spy_get();
    EXPECT_EQ(0, strncmp("Invalid", cm_printf_spy_get(), 7));
}

// del cmd invalid: must give index since the max number owned is more than 1.
TEST_F(CfgMgrOwned, del_without_index)
{
    char * commandAdd[] = {(char *)"add", (char *)"owned"};
    cm->handleCmd(2, commandAdd);
    char * commandDel[] = {(char *)"del", (char *)"owned"};
    cm->handleCmd(2, commandDel);

    //std::cout << cm_printf_spy_get();
    EXPECT_EQ(0, strncmp("Invalid", cm_printf_spy_get(), 7));
}

// del cmd invalid: item not created.
TEST_F(CfgMgrOwned, del_non_existent_item)
{
    char * commandAdd[] = {(char *)"add", (char *)"owned"};
    cm->handleCmd(2, commandAdd);
    // Only 0th item exists; try to delete 1th.
    char * commandDel[] = {(char *)"del", (char *)"owned", (char *)"1"};
    cm->handleCmd(3, commandDel);

    //std::cout << cm_printf_spy_get();
    EXPECT_EQ(0, strncmp("Invalid", cm_printf_spy_get(), 7));
}

// Set cmd valid: create 2 owned items, set the first one (a set without implicit add)
TEST_F(CfgMgrOwned, set_index_less_than_count)
{
    char * commandAdd[] = {(char *)"add", (char *)"owned"};
    cm->handleCmd(2, commandAdd);
    cm->handleCmd(2, commandAdd);

    char * commandWord[] = {(char *)"owned", (char *)"0", (char *)"=", (char *)"13"};
    cm->handleCmd(4, commandWord);

    EXPECT_EQ(13, C2_CONFIG->owned[0]);
    EXPECT_EQ(0, C2_CONFIG->owned[1]);
}

// Set up data (with implicit add) and print the config, which excludes 'count', as it has persistent=false.
TEST_F(CfgMgrOwned, set_and_print_config)
{
    char * commandWord[] = {(char *)"owned", (char *)"0", (char *)"=", (char *)"73"};
    cm->handleCmd(4, commandWord);
    char * commandPrt[] = {(char *)"prtc"};
    cm->handleCmd(1, commandPrt);

    EXPECT_STREQ("owned 0 = 73\n", cm_printf_spy_get());
}

// Add with index to insert item before existing items.
TEST_F(CfgMgrOwned, add_insert)
{
    std::vector<char *> command1 = {(char *)"owned", (char *)"0", (char *)"=", (char *)"111"};
    cm->handleCmd(command1.size(), command1.data());
    std::vector<char *> commandAdd = {(char *)"add", (char *)"owned", (char *)"0"};
    cm->handleCmd(commandAdd.size(), commandAdd.data());

    EXPECT_EQ(2, C2_CONFIG->cnt);
    EXPECT_EQ(0, C2_CONFIG->owned[0]); // insert has default value
    EXPECT_EQ(111, C2_CONFIG->owned[1]);
}

TEST_F(CfgMgrOwned, add_bad_index)
{
    std::vector<char *> commandAdd = {(char *)"add", (char *)"owned", (char *)"zero"};
    cm->handleCmd(commandAdd.size(), commandAdd.data());

    //std::cout << cm_printf_spy_get();
    EXPECT_EQ(0, strncmp("Invalid", cm_printf_spy_get(), 7));
}

TEST_F(CfgMgrOwned, add_index_too_big)
{
    std::vector<char *> commandAdd = {(char *)"add", (char *)"owned", (char *)"1"};
    cm->handleCmd(commandAdd.size(), commandAdd.data());

    //std::cout << cm_printf_spy_get();
    EXPECT_EQ(0, strncmp("Invalid", cm_printf_spy_get(), 7));
}

// cmd 'add' without the name of the component to add is invalid
TEST_F(CfgMgrOwned, add_without_param)
{
    char * commandWord[] = {(char *)"add"};
    cm->handleCmd(1, commandWord);

    // The output to the user starts with "Invalid".
    EXPECT_EQ(0, strncmp("Invalid", cm_printf_spy_get(), 7));
}

// cmd 'add' with incorrect the name of the component to add is invalid
TEST_F(CfgMgrOwned, add_with_invalid_component)
{
    char * commandWord[] = {(char *)"add", (char *)"owoooo"};
    cm->handleCmd(2, commandWord);

    // The output to the user starts with "Invalid".
    EXPECT_EQ(0, strncmp("Invalid", cm_printf_spy_get(), 7));
}

// cmd 'del' without the name of the component to remove is invalid
TEST_F(CfgMgrOwned, del_without_param)
{
    char * commandWord[] = {(char *)"del"};
    cm->handleCmd(1, commandWord);

    // The output to the user starts with "Invalid".
    EXPECT_EQ(0, strncmp("Invalid", cm_printf_spy_get(), 7));
}

// cmd 'del' with incorrect the name of the component to remove is invalid
TEST_F(CfgMgrOwned, del_with_invalid_component)
{
    char * commandWord[] = {(char *)"del", (char *)"owoooo"};
    cm->handleCmd(2, commandWord);

    // The output to the user starts with "Invalid".
    EXPECT_EQ(0, strncmp("Invalid", cm_printf_spy_get(), 7));
}

// Verify what's loaded into memory, given TLV file that's read on startup.
TEST_F(CfgMgrOwned, load)
{
    uint8_t tlv[20] =
    { 1,0, 16,0, 4,0, 4,0, 7,0,0,0, 4,0, 4,0, 8,0,0,0};
    /*T    L     T    L    V        T    L    V      */

    /* Create config file to be loaded */
    nvram->set(tlv, sizeof(tlv));
    char * commandWord[] = {(char *)"load"};
    cm->handleCmd(1, commandWord);
    EXPECT_EQ(2, C2_CONFIG->cnt);
    ASSERT_TRUE(C2_CONFIG->owned != nullptr);
    EXPECT_EQ(7, C2_CONFIG->owned[0]);
    EXPECT_EQ(8, C2_CONFIG->owned[1]);
}

// Verify what's saved to TLV, given a TLV file that's read on startup that
// has more than the max number of an OWNED component.
//
TEST_F(CfgMgrOwned, loadTooMany)
{
    uint8_t tlv[28] =
    { 1,0, 24,0, 4,0, 4,0, 7,0,0,0, 4,0, 4,0, 8,0,0,0, 4,0, 4,0, 9,0,0,0};
    /*T    L     T    L    V        T    L    V        T    L    V       */
    uint8_t expectedTlv[20] =
    { 1,0, 16,0, 4,0, 4,0, 7,0,0,0, 4,0, 4,0, 8,0,0,0};
    /*T    L     T    L    V        T    L    V    */

    /* Create config file to be loaded */
    nvram->set(tlv, sizeof(tlv));

    char * commandWord[] = {(char *)"load"};
    cm->handleCmd(1, commandWord);
    EXPECT_EQ(2, C2_CONFIG->cnt);

    char * commandWord1[] = {(char *)"save"};
    cm->handleCmd(1, commandWord1);

    // See what CM made of the file it loaded
    EXPECT_TRUE(nvram->match(expectedTlv, sizeof(expectedTlv)));
}

// Verify what's saved to TLV, given a TLV file that's read on startup that
// contains the ID of a non-persistent item (the counter).  It should
// be ignored, like an unknown ID is.
//
TEST_F(CfgMgrOwned, loadNonPersistent)
{
    uint8_t tlv[20] =
    { 1,0, 16,0, 3,0, 4,0, 1,0,0,0, 4,0, 4,0, 5,0,0,0};
    /*T    L     T    L    V        T    L    V      */

    /* Create config file to be loaded */
    nvram->set(tlv, sizeof(tlv));
    char * commandWord[] = {(char *)"load"};
    cm->handleCmd(1, commandWord);
    EXPECT_EQ(1, C2_CONFIG->cnt);
    ASSERT_TRUE(C2_CONFIG->owned != nullptr);
    EXPECT_EQ(5, C2_CONFIG->owned[0]);
}

// Verify data saved to TLV, with default data in RAM as input to the test.
// The contained component is not saved because it is not persistent,
// and there is no owned component by default, so nothing is saved.
// Perhaps SUT shouldn't even create a file in this case.
TEST_F(CfgMgrOwned, save)
{
    uint8_t expectedTlv [] = {};

    char * commandWord[] = {(char *)"save"};
    cm->handleCmd(1, commandWord);

    EXPECT_TRUE(nvram->match(expectedTlv, sizeof(expectedTlv)));
}

// A command that references (without setting) a non-existent owned item, is invalid.
TEST_F(CfgMgrOwned, refNonExistentItemFail)
{

    char * commandWord[] = {(char *)"owned", (char *)"0"}; // reference owned item 0
    cm->handleCmd(2, commandWord);

    // The output to the user starts with "Invalid".
    EXPECT_EQ(0, strncmp("Invalid", cm_printf_spy_get(), 7));
}

// From default RAM start, do implicit add and set and check what's saved to TLV
TEST_F(CfgMgrOwned, implicitAdd)
{
    uint8_t expectedTlv [12] =
    { 1,0, 8,0, 4,0, 4,0, 7,0,0,0};
    /*T    L    T    L    V    */

    EXPECT_EQ(0, C2_CONFIG->cnt);
    EXPECT_EQ(nullptr, C2_CONFIG->owned);

    char * commandWord[] = {(char *)"owned", (char *)"0", (char *)"=", (char *)"7"}; // set item 0, causing implicit add
    cm->handleCmd(4, commandWord);

    EXPECT_EQ(1, C2_CONFIG->cnt);
    ASSERT_TRUE(C2_CONFIG->owned != nullptr);
    EXPECT_EQ(7, C2_CONFIG->owned[0]);

    char * commandWord2[] = {(char *)"save"};
    cm->handleCmd(1, commandWord2);

    EXPECT_TRUE(nvram->match(expectedTlv, sizeof(expectedTlv)));
}

// From default RAM start, do explicit add and check what's saved to TLV
TEST_F(CfgMgrOwned, explicitAdd)
{
    uint8_t expectedTlv [12] =
    { 1,0, 8,0, 4,0, 4,0, 0,0,0,0};
    /*T    L    T    L    V    */

    EXPECT_EQ(0, C2_CONFIG->cnt);
    EXPECT_EQ(nullptr, C2_CONFIG->owned);

    char * commandWord[] = {(char *)"add", (char *)"owned"};
    cm->handleCmd(2, commandWord);

    EXPECT_EQ(1, C2_CONFIG->cnt);
    EXPECT_TRUE(C2_CONFIG->owned != nullptr);

    char * commandWord2[] = {(char *)"save"};
    cm->handleCmd(1, commandWord2);

    EXPECT_TRUE(nvram->match(expectedTlv, sizeof(expectedTlv)));
}

// Verify what's saved to TLV, given a TLV file that's read on startup,
// and a delete operation.
// xxx this should save empty file -- better way to test this than memcmp?
// Or replace with a test where something remains after deletion?
TEST_F(CfgMgrOwned, del)
{
    uint8_t tlv[12] =
    { 1,0, 8,0, 4,0, 4,0, 5,0,0,0};
    //T    L    T    L    V
    // Assumes little-endian integers

    uint8_t expectedTlv[] = {};

    /* Create config file to be loaded */
    nvram->set(tlv, sizeof(tlv));
    char * commandWord[] = {(char *)"load"};
    cm->handleCmd(1, commandWord);
    EXPECT_EQ(1, C2_CONFIG->cnt);
    ASSERT_TRUE(C2_CONFIG->owned != nullptr);
    EXPECT_EQ(5, C2_CONFIG->owned[0]);

    char * commandWord2[] = {(char *)"del", (char *)"owned", (char *)"0"};
    cm->handleCmd(3, commandWord2);

    EXPECT_EQ(0, C2_CONFIG->cnt);
    EXPECT_EQ(nullptr, C2_CONFIG->owned);

    char * commandWord3[] = {(char *)"save"};
    cm->handleCmd(1, commandWord3);

    // See what CM made of the file it loaded
    EXPECT_TRUE(nvram->match(expectedTlv, sizeof(expectedTlv)));
}


////////////////////////////////////////////////////////////////////////////////
// test set 3, CONTAINED array
// test set 3 data structure
#define T3_ARRAY_SIZE 2
struct m3
{
    short int m1[T3_ARRAY_SIZE];
};

// test set 3 metadata
const Simple_metadata s5_d = {{"name1", 1, sizeof(short int)}, nullptr, nullptr, nullptr};
Simple_descriptor s5(&s5_d);
const Aggregate_data ca5_d = {&s5, T3_ARRAY_SIZE, offsetof(struct m3, m1)};
Contained_aggregate ca5(&ca5_d);

Aggregate * const aggrList3[] = {&ca5};
const Composite_metadata c3_d = {{"c3", 1, sizeof(struct m3)}, aggrList3, sizeof(aggrList3)/sizeof(aggrList3[0])};
Composite_descriptor c3(&c3_d);

struct m3 * C3_CONFIG = nullptr;

class CfgMgrContainedArray : public testing::Test
{
protected:
    //Define data accessible to test group members here.
    unique_ptr<Nvram_spy> nvram;
    unique_ptr<Config_manager_implement> cm;

    virtual void SetUp()
    {
        nvram = make_unique<Nvram_spy>();
        cm = make_unique<Config_manager_implement>(&c3, cm_printf_spy, (uint8_t **)&C3_CONFIG, nvram.get());
        cm_printf_spy_init();
    }
};

// Set cmd is invalid.
TEST_F(CfgMgrContainedArray, set_index_too_big)
{
    char * commandWord[] = {(char *)"name1", (char *)"11", (char *)"=", (char *)"71"};
    cm->handleCmd(4, commandWord);

    // The output to the user starts with "Invalid".
    EXPECT_EQ(0, strncmp("Invalid", cm_printf_spy_get(), 7));
}

// Set cmd is invalid.
TEST_F(CfgMgrContainedArray, set_missing_index)
{
    char * commandWord[] = {(char *)"name1", (char *)"=", (char *)"71"};
    cm->handleCmd(3, commandWord);

    // The output to the user starts with "Invalid".
    //std::cout << cm_printf_spy_get();
    EXPECT_EQ(0, strncmp("Invalid", cm_printf_spy_get(), 7));
}

// 'add' cmd is invalid for contained array.
TEST_F(CfgMgrContainedArray, add)
{
    char * commandWord[] = {(char *)"add", (char *)"name1"};
    cm->handleCmd(2, commandWord);

    // The output to the user starts with "Invalid".
    //std::cout << cm_printf_spy_get();
    EXPECT_EQ(0, strncmp("Invalid", cm_printf_spy_get(), 7));
}

// 'del' cmd is invalid for contained array.
TEST_F(CfgMgrContainedArray, del)
{
    char * commandWord[] = {(char *)"del", (char *)"name1", (char *)"0"};
    cm->handleCmd(3, commandWord);

    // The output to the user starts with "Invalid".
    //std::cout << cm_printf_spy_get();
    EXPECT_EQ(0, strncmp("Invalid", cm_printf_spy_get(), 7));
}

// Verify what's loaded into memory, given TLV file that's read on startup.
TEST_F(CfgMgrContainedArray, load)
{
    uint8_t tlv[16] =
    { 1,0, 12,0, 1,0, 2,0, 7,0, 1,0, 2,0, 8,0};
    /*T    L     T    L    V    T    L    V */

    /* Create config file to be loaded */
    nvram->set(tlv, sizeof(tlv));

    char * commandWord[] = {(char *)"load"};
    cm->handleCmd(1, commandWord);

    EXPECT_EQ(7, C3_CONFIG->m1[0]);
    EXPECT_EQ(8, C3_CONFIG->m1[1]);
}

// Verify what's saved to TLV, given a TLV file that's read on startup that
// has more than the max number of a CONTAINED component.
//
TEST_F(CfgMgrContainedArray, loadTooMany)
{
    uint8_t tlv[22] =
    { 1,0, 18,0, 1,0, 2,0, 7,0, 1,0, 2,0, 8,0, 1,0, 2,0, 9,0};
    /*T    L     T    L    V    T    L    V    T    L    V       */
    uint8_t expectedTlv[16] =
    { 1,0, 12,0, 1,0, 2,0, 7,0, 1,0, 2,0, 8,0};
    /*T    L     T    L    V    T    L    V    */

    /* Create config file to be loaded */
    nvram->set(tlv, sizeof(tlv));
    char * commandWord[] = {(char *)"load"};
    cm->handleCmd(1, commandWord);
    char * commandWord2[] = {(char *)"save"};
    cm->handleCmd(1, commandWord2);

    // See what CM made of the file it loaded
    EXPECT_TRUE(nvram->match(expectedTlv, sizeof(expectedTlv)));
}


////////////////////////////////////////////////////////////////////////////////
// test set 4, CONTAINED arrays, to check we handle resetting the index to 0
// test set 4 data structure
static const unsigned T6_ARRAY_SIZE = 2;
static const unsigned T7_ARRAY_SIZE = 2;
struct m6
{
    short int m1[T6_ARRAY_SIZE];
    short int m2[T7_ARRAY_SIZE];
};

// test set 4 metadata
const Simple_metadata s6_d = {{"name1", 1, sizeof(short int)}, nullptr, nullptr, nullptr};
Simple_descriptor s6(&s6_d);
const Aggregate_data ca6_d = {&s6, T6_ARRAY_SIZE, offsetof(struct m6, m1)};
Contained_aggregate ca6(&ca6_d);

const Simple_metadata s7_d = {{"name2", 2, sizeof(short int)}, nullptr, nullptr, nullptr};
Simple_descriptor s7(&s7_d);
const Aggregate_data ca7_d = {&s7, T7_ARRAY_SIZE, offsetof(struct m6, m2)};
Contained_aggregate ca7(&ca7_d);

Aggregate * const aggrList4[] = {&ca6, &ca7};
const Composite_metadata c4_d = {{"c4", 1, sizeof(struct m6)}, aggrList4, sizeof(aggrList4)/sizeof(aggrList4[0])};
Composite_descriptor c4(&c4_d);

struct m6 * C4_CONFIG = nullptr;

class CfgMgrContainedArrays : public testing::Test
{
protected:
    //Define data accessible to test group members here.
    unique_ptr<Nvram_spy> nvram;
    unique_ptr<Config_manager_implement> cm;

    virtual void SetUp()
    {
        nvram = make_unique<Nvram_spy>();
        cm = make_unique<Config_manager_implement>(&c4, cm_printf_spy, (uint8_t **)&C4_CONFIG, nvram.get());
        cm_printf_spy_init();
    }
};


// Verify what's loaded into memory, given TLV file that's read on startup.
TEST_F(CfgMgrContainedArrays, load)
{
    uint8_t tlv[28] =
    { 1,0, 24,0, 1,0, 2,0, 4,0, 1,0, 2,0, 5,0, 2,0, 2,0, 6,0, 2,0, 2,0, 7,0};
    /*T    L     T    L    V    T    L    V    T    L    V    T    L    V */

    nvram->set(tlv, sizeof(tlv));
    char * commandWord[] = {(char *)"load"};
    cm->handleCmd(1, commandWord);
    EXPECT_EQ(4, C4_CONFIG->m1[0]);
    EXPECT_EQ(5, C4_CONFIG->m1[1]);
    EXPECT_EQ(6, C4_CONFIG->m2[0]);
    EXPECT_EQ(7, C4_CONFIG->m2[1]);
}


////////////////////////////////////////////////////////////////////////////////
// test set 5, OWNED in OWNED.
namespace test_set_5 {
struct m1
{
    long long int * pint;
};

struct m2
{
    m1 * pm1;
};

// test set 5 metadata
const Simple_metadata s1_d = {{"pint", NOT_PERSISTENT, sizeof(long long int)}, cm_set_int, nullptr, cm_prt_int};
Simple_descriptor s1(&s1_d);
const Owned_aggregate_data oa1_d = {{&s1, 1, offsetof(struct m1, pint)}, nullptr}; // no counter ref for single
Owned_aggregate oa1(&oa1_d);

Aggregate * const aggrList1[] = {&oa1};
const Composite_metadata c1_d = {{"m1", NOT_PERSISTENT, sizeof(struct m1)}, aggrList1, sizeof(aggrList1)/sizeof(aggrList1[0])};
Composite_descriptor c1(&c1_d);
const Owned_aggregate_data oa2_d = {{&c1, 1, offsetof(struct m2, pm1)}, nullptr}; // no counter ref for single
Owned_aggregate oa2(&oa2_d);

Aggregate * const aggrList2[] = {&oa2};
const Composite_metadata c2_d = {{"m2", NOT_PERSISTENT, sizeof(struct m2)}, aggrList2, sizeof(aggrList2)/sizeof(aggrList2[0])};
Composite_descriptor c2(&c2_d);

struct m2 * CONFIG = nullptr;
} // namespace

class CfgMgrOwnedOwned : public testing::Test
{
protected:
    //Define data accessible to test group members here.
    unique_ptr<Nvram_spy> nvram;
    unique_ptr<Config_manager_implement> cm;

    virtual void SetUp()
    {
        nvram = make_unique<Nvram_spy>();
        cm = make_unique<Config_manager_implement>(&test_set_5::c2, cm_printf_spy, (uint8_t **)&test_set_5::CONFIG, nvram.get());
        cm_printf_spy_init();
    }
};

// Can't add to a composite that hasn't been created.
TEST_F(CfgMgrOwnedOwned, invalid_add)
{
    char * commandWord[] = {(char *)"m1", (char *)"add", (char *)"pint"};
    cm->handleCmd(3, commandWord);

    //std::cout << cm_printf_spy_get();
    EXPECT_EQ(0, strncmp("Invalid", cm_printf_spy_get(), 7));
}

// Can't delete from composite that hasn't been created.
TEST_F(CfgMgrOwnedOwned, invalid_del)
{
    char * commandWord[] = {(char *)"m1", (char *)"del", (char *)"pint"};
    cm->handleCmd(3, commandWord);

    //std::cout << cm_printf_spy_get();
    EXPECT_EQ(0, strncmp("Invalid", cm_printf_spy_get(), 7));
}

// Add a contained item and change context to it.
TEST_F(CfgMgrOwnedOwned, change_context)
{
    char * commandWord[] = {(char *)"add", (char *)"m1"};
    cm->handleCmd(2, commandWord);
    char * commandContex[] = {(char *)"m1"};
    cm->handleCmd(1, commandContex);

    EXPECT_STREQ("m2 m1 ", cm->getPromptString());
}

// Print config skips this non-persitent composite, although it has content
TEST_F(CfgMgrOwnedOwned, print_config_non_persistent_composite)
{
    char * commandWord[] = {(char *)"m1", (char *)"pint", (char *)"=", (char *)"441"};
    cm->handleCmd(4, commandWord);
    char * commandPrtc[] = {(char *)"prtc"};
    cm->handleCmd(1, commandPrtc);
    EXPECT_STREQ("", cm_printf_spy_get());

    char * commandPrt[] = {(char *)"prt"};
    cm->handleCmd(1, commandPrt);
    EXPECT_STREQ("m1 pint = 441\n", cm_printf_spy_get());
}

namespace owned_key_string {
constexpr unsigned KEY_LEN = 12;
constexpr unsigned MAX_NUM_OWNED = 2;

struct t_owned
{
    char key[KEY_LEN];
    unsigned id;
};

struct t_device
{
    unsigned cnt;
    t_owned *owned;
};

// key is contained in owned
const Simple_metadata key_d = {{"key", 1, KEY_LEN}, cm_set_str, nullptr, cm_prt_str};
Simple_descriptor key_s(&key_d);
const Aggregate_data key_ad = {&key_s, 1, offsetof(t_owned, key)};
Contained_aggregate key_a(&key_ad);

// id is contained in owned
const Simple_metadata id_d = {{"id", 2, sizeof(unsigned)}, cm_set_int, cm_setdef_null, cm_prt_int};
Simple_descriptor id_s(&id_d);
const Aggregate_data id_ad = {&id_s, 1, offsetof(t_owned, id)};
Contained_aggregate id_a(&id_ad);

// cnt is contained in t_device
const Simple_metadata cnt_d = {{"cnt", NOT_PERSISTENT, sizeof(unsigned)}, nullptr, nullptr, cm_prt_int};
Simple_descriptor cnt_s(&cnt_d);
const Aggregate_data cnt_ad = {&cnt_s, 1, offsetof(t_device, cnt)};
Contained_aggregate cnt_a(&cnt_ad);

// owned is a composite (of key and id) owned by t_device
Aggregate * const owned_l[] = {&key_a, &id_a};
const Composite_metadata owned_d = {{"owned", 1, sizeof(t_owned)}, owned_l, sizeof(owned_l)/sizeof(owned_l[0])};
Composite_descriptor owned_c(&owned_d);
// owned has a counter (a sibling) and a key (a component).
const Owned_aggregate_data owned_ad = {{&owned_c, MAX_NUM_OWNED, offsetof(t_device, owned)}, &cnt_a, &key_a};
Owned_aggregate owned_a(&owned_ad);

// device is a composite of cnt and owned.
Aggregate * const device_l[] = {&cnt_a, &owned_a};
const Composite_metadata device_d = {{"device", 1, sizeof(t_device)}, device_l, sizeof(device_l)/sizeof(device_l[0])};
Composite_descriptor device_c(&device_d);

t_device * DEVICE = nullptr;

class CfgMgrOwnedKey : public testing::Test
{
protected:
    //Define data accessible to test group members here.
    unique_ptr<Nvram_spy> nvram;
    unique_ptr<Config_manager_implement> cm;

    virtual void SetUp()
    {
        nvram = make_unique<Nvram_spy>();
        cm = make_unique<Config_manager_implement>(&device_c, cm_printf_spy, (uint8_t **)&DEVICE, nvram.get());
        cm_printf_spy_init();
    }
};

TEST_F(CfgMgrOwnedKey, add_and_print)
{
    std::vector<char *> command = {(char *)"add", (char *)"owned", (char *)"\"fred\""};
    cm->handleCmd(command.size(), command.data());

    ASSERT_EQ(1, DEVICE->cnt);
    EXPECT_STREQ("fred", DEVICE->owned[0].key);

    std::vector<char *> command_prt = {(char *)"prt"};
    cm->handleCmd(command_prt.size(), command_prt.data());

    EXPECT_STREQ("cnt = 1\nowned \"fred\" key = \"fred\"\nowned \"fred\" id = 0\n", cm_printf_spy_get());
}

TEST_F(CfgMgrOwnedKey, add_implicit)
{
    std::vector<char *> command = {(char *)"owned", (char *)"\"wilma\"", (char *)"id", (char *)"=", (char *)"14"};
    cm->handleCmd(command.size(), command.data());

    ASSERT_EQ(1, DEVICE->cnt);
    EXPECT_STREQ("wilma", DEVICE->owned[0].key);
    EXPECT_EQ(14, DEVICE->owned[0].id);
}

TEST_F(CfgMgrOwnedKey, cmd_no_key_fail)
{
    std::vector<char *> command = {(char *)"owned"};
    cm->handleCmd(command.size(), command.data());

    std::cout << cm_printf_spy_get();
    EXPECT_EQ(0, strncmp("Invalid", cm_printf_spy_get(), 7));
}

TEST_F(CfgMgrOwnedKey, del)
{
    std::vector<char *> command = {(char *)"add", (char *)"owned", (char *)"\"fred\""};
    cm->handleCmd(command.size(), command.data());
    ASSERT_EQ(1, DEVICE->cnt);

    std::vector<char *> command_del = {(char *)"del", (char *)"owned", (char *)"\"fred\""};
    cm->handleCmd(command_del.size(), command_del.data());

    EXPECT_EQ(0, DEVICE->cnt);
}

TEST_F(CfgMgrOwnedKey, add_twice_fail)
{
    std::vector<char *> command = {(char *)"add", (char *)"owned", (char *)"\"fred\""};
    cm->handleCmd(command.size(), command.data());
    cm->handleCmd(command.size(), command.data());

    EXPECT_EQ(1, DEVICE->cnt);
    //std::cout << cm_printf_spy_get();
    EXPECT_EQ(0, strncmp("Invalid", cm_printf_spy_get(), 7));
}

TEST_F(CfgMgrOwnedKey, add_invalid_key_fail)
{
    std::vector<char *> command = {(char *)"add", (char *)"owned", (char *)"noquotes"};
    cm->handleCmd(command.size(), command.data());

    //std::cout << cm_printf_spy_get();
    EXPECT_EQ(0, strncmp("Invalid", cm_printf_spy_get(), 7));
}

TEST_F(CfgMgrOwnedKey, del_wrong_key_fail)
{
    std::vector<char *> command = {(char *)"add", (char *)"owned", (char *)"\"fred\""};
    cm->handleCmd(command.size(), command.data());
    std::vector<char *> command_del = {(char *)"del", (char *)"owned", (char *)"\"wilma\""};
    cm->handleCmd(command_del.size(), command_del.data());

    EXPECT_EQ(1, DEVICE->cnt);
    //std::cout << cm_printf_spy_get();
    EXPECT_EQ(0, strncmp("Invalid", cm_printf_spy_get(), 7));
}

TEST_F(CfgMgrOwnedKey, del_invalid_key_fail)
{
    std::vector<char *> command = {(char *)"add", (char *)"owned", (char *)"\"fred\""};
    cm->handleCmd(command.size(), command.data());
    std::vector<char *> command_del = {(char *)"del", (char *)"owned", (char *)"noquotes"};
    cm->handleCmd(command_del.size(), command_del.data());

    EXPECT_EQ(1, DEVICE->cnt);
    //std::cout << cm_printf_spy_get();
    EXPECT_EQ(0, strncmp("Invalid", cm_printf_spy_get(), 7));
}

// check that we can save, delete with setdef, and restore with load.
TEST_F(CfgMgrOwnedKey, save_setdef_load)
{
    std::vector<char *> command = {(char *)"add", (char *)"owned", (char *)"\"Barney\""};
    cm->handleCmd(command.size(), command.data());
    EXPECT_EQ(1, DEVICE->cnt);
    std::vector<char *> command_save= {(char *)"save"};
    cm->handleCmd(command_save.size(), command_save.data());
    std::vector<char *> command_setdef = {(char *)"setdef"};
    cm->handleCmd(command_setdef.size(), command_setdef.data());
    EXPECT_EQ(0, DEVICE->cnt);

    std::vector<char *> command_load = {(char *)"load"};
    cm->handleCmd(command_load.size(), command_load.data());
    ASSERT_EQ(1, DEVICE->cnt);
    EXPECT_STREQ("Barney", DEVICE->owned[0].key);
}

} // namespace owned_key_string

namespace owned_key_in_owned {
constexpr unsigned KEY_LEN = 12;
constexpr unsigned MAX_NUM_OWNED = 2;

struct t_owned
{
    char key[KEY_LEN];
    unsigned id;
};

struct t_optional
{
    unsigned cnt;
    t_owned *owned;
};

struct t_device
{
    t_optional *optional;
};

// key is contained in owned
const Simple_metadata key_d = {{"key", 1, KEY_LEN}, cm_set_str, nullptr, cm_prt_str};
Simple_descriptor key_s(&key_d);
const Aggregate_data key_ad = {&key_s, 1, offsetof(t_owned, key)};
Contained_aggregate key_a(&key_ad);

// id is contained in owned
const Simple_metadata id_d = {{"id", 2, sizeof(unsigned)}, cm_set_int, cm_setdef_null, cm_prt_int};
Simple_descriptor id_s(&id_d);
const Aggregate_data id_ad = {&id_s, 1, offsetof(t_owned, id)};
Contained_aggregate id_a(&id_ad);

// cnt is contained in t_optional
const Simple_metadata cnt_d = {{"cnt", NOT_PERSISTENT, sizeof(unsigned)}, nullptr, nullptr, cm_prt_int};
Simple_descriptor cnt_s(&cnt_d);
const Aggregate_data cnt_ad = {&cnt_s, 1, offsetof(t_optional, cnt)};
Contained_aggregate cnt_a(&cnt_ad);

// owned is a composite (of key and id) owned by t_optional
Aggregate * const owned_l[] = {&key_a, &id_a};
const Composite_metadata owned_d = {{"owned", 1, sizeof(t_owned)}, owned_l, sizeof(owned_l)/sizeof(owned_l[0])};
Composite_descriptor owned_c(&owned_d);
// owned has a counter (a sibling) and a key (a component).
const Owned_aggregate_data owned_ad = {{&owned_c, MAX_NUM_OWNED, offsetof(t_optional, owned)}, &cnt_a, &key_a};
Owned_aggregate owned_a(&owned_ad);

// optional is a composite (of cnt and owned) owned by t_device
Aggregate * const optional_l[] = {&cnt_a, &owned_a};
const Composite_metadata optional_d = {{"optional", 1, sizeof(t_optional)}, optional_l, sizeof(optional_l)/sizeof(optional_l[0])};
Composite_descriptor optional_c(&optional_d);
const Owned_aggregate_data optional_ad = {{&optional_c, 1, offsetof(t_device, optional)}};
Owned_aggregate optional_a(&optional_ad);

// device is a composite of optional.
Aggregate * const device_l[] = {&optional_a};
const Composite_metadata device_d = {{"device", 1, sizeof(t_device)}, device_l, sizeof(device_l)/sizeof(device_l[0])};
Composite_descriptor device_c(&device_d);

t_device * DEVICE = nullptr;

class CfgMgrOwnedKeyOwned : public testing::Test
{
protected:
    //Define data accessible to test group members here.
    unique_ptr<Nvram_spy> nvram;
    unique_ptr<Config_manager_implement> cm;

    virtual void SetUp()
    {
        nvram = make_unique<Nvram_spy>();
        cm = make_unique<Config_manager_implement>(&device_c, cm_printf_spy, (uint8_t **)&DEVICE, nvram.get());
        cm_printf_spy_init();
    }
};

TEST_F(CfgMgrOwnedKeyOwned, add_implicit)
{
    std::vector<char *> command = {(char *)"optional", (char *)"owned", (char*)"\"fred\"", (char*)"id", (char*)"=", (char*)"19"};
    cm->handleCmd(command.size(), command.data());

    ASSERT_NE(DEVICE->optional, nullptr);
    ASSERT_EQ(1, DEVICE->optional->cnt);
    EXPECT_STREQ("fred", DEVICE->optional->owned[0].key);
    EXPECT_EQ(19, DEVICE->optional->owned[0].id);
}

} // namespace owned_key_in_owned

namespace owned_key_int {

constexpr unsigned MAX_NUM_OWNED = 2;

struct t_owned
{
    unsigned id;
};

struct t_device
{
    unsigned cnt;
    t_owned *owned;
};

// id is contained in owned
const Simple_metadata id_d = {{"id", 2, sizeof(unsigned)}, cm_set_int, cm_setdef_null, cm_prt_int};
Simple_descriptor id_s(&id_d);
const Aggregate_data id_ad = {&id_s, 1, offsetof(t_owned, id)};
Contained_aggregate id_a(&id_ad);

// cnt is contained in t_device
const Simple_metadata cnt_d = {{"cnt", NOT_PERSISTENT, sizeof(unsigned)}, nullptr, nullptr, cm_prt_int};
Simple_descriptor cnt_s(&cnt_d);
const Aggregate_data cnt_ad = {&cnt_s, 1, offsetof(t_device, cnt)};
Contained_aggregate cnt_a(&cnt_ad);

// owned is a composite (of id) owned by t_device
Aggregate * const owned_l[] = {&id_a};
const Composite_metadata owned_d = {{"owned", 1, sizeof(t_owned)}, owned_l, sizeof(owned_l)/sizeof(owned_l[0])};
Composite_descriptor owned_c(&owned_d);
// owned has a counter (a sibling) and a key (a component).
const Owned_aggregate_data owned_ad = {{&owned_c, MAX_NUM_OWNED, offsetof(t_device, owned)}, &cnt_a, &id_a};
Owned_aggregate owned_a(&owned_ad);

// device is a composite of cnt and owned.
Aggregate * const device_l[] = {&cnt_a, &owned_a};
const Composite_metadata device_d = {{"device", 1, sizeof(t_device)}, device_l, sizeof(device_l)/sizeof(device_l[0])};
Composite_descriptor device_c(&device_d);

t_device * DEVICE = nullptr;

class CfgMgrOwnedKeyInt : public testing::Test
{
protected:
    //Define data accessible to test group members here.
    unique_ptr<Nvram_spy> nvram;
    unique_ptr<Config_manager_implement> cm;

    virtual void SetUp()
    {
        nvram = make_unique<Nvram_spy>();
        cm = make_unique<Config_manager_implement>(&device_c, cm_printf_spy, (uint8_t **)&DEVICE, nvram.get());
        cm_printf_spy_init();
    }
};

TEST_F(CfgMgrOwnedKeyInt, add_and_print)
{
    std::vector<char *> command = {(char *)"add", (char *)"owned", (char *)"99"};
    cm->handleCmd(command.size(), command.data());

    ASSERT_EQ(1, DEVICE->cnt);
    EXPECT_EQ(99, DEVICE->owned[0].id);

    std::vector<char *> command_prt = {(char *)"prt"};
    cm->handleCmd(command_prt.size(), command_prt.data());

    EXPECT_STREQ("cnt = 1\nowned 99 id = 99\n", cm_printf_spy_get());
}

} // namespace owned_key_int

} // namespace
