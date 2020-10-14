// Unit test using open-source unit test framework
// These tests the config manager as a whole, using the following interfaces:
// 1. Input: character strings passed to Config_manager::handleCmd
// 2. Input/output: the binary file containing TLV data used by Config_manager
//    for non-volatile storage
//
// xxx how many of these tests belong in cm_tlvTest.cpp?
//

#include "gtest/gtest.h"
#include "cfg_mgr.h"       // Unit under test
#include "cfg_mgr_descriptor.h"       // Unit under test
#include "cfg_mgr_aggregate.h"       // Unit under test
#include "cfg_mgr_set_int.h"  // Extensions to unit under test (generic "set" functions)
#include "cfg_mgr_setdef_null.h" // generic setdef function
#include "nvram_spy.h"

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
const Simple_metadata s1_d = {{"name1", 1, sizeof(int), true}, nullptr, cm_setdef_null, nullptr};
const Simple_descriptor s1(&s1_d);
const Aggregate_data ca1_d = {&s1, 1, offsetof(struct m, m1)};
const Contained_aggregate ca1(&ca1_d);

const Simple_metadata s2_d = {{"name2", 2, sizeof(int), true}, nullptr, setdef_t1, nullptr};
const Simple_descriptor s2(&s2_d);
const Aggregate_data ca2_d = {&s2, 1, offsetof(struct m, m2)};
const Contained_aggregate ca2(&ca2_d);

const Aggregate * const aggrList1[] = {&ca1, &ca2};
const Composite_metadata c1_d = {{"c1", 1, sizeof(struct m), true}, aggrList1, sizeof(aggrList1)/sizeof(aggrList1[0])};
const Composite_descriptor c1(&c1_d);

#define GET_C1_CONFIG ((struct m *)cm->getConfig())

class CfgMgrContained : public testing::Test
{
protected:
    Nvram_spy * nvram;
    Config_manager * cm;

    //Define data accessible to test group members here.
    virtual void SetUp()
    {
        nvram = new Nvram_spy;
        cm = new Config_manager(&c1, nvram);
    }

    virtual void TearDown()
    {
        delete cm;
        delete nvram;
    }
};


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

    EXPECT_EQ(8, GET_C1_CONFIG->m1);
    EXPECT_EQ(9, GET_C1_CONFIG->m2);
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

    EXPECT_EQ(0, GET_C1_CONFIG->m1); // Default
    EXPECT_EQ(7, GET_C1_CONFIG->m2); // Default; not read from the file, because TLV's L is bad
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
    EXPECT_EQ(0, GET_C1_CONFIG->m1);
    EXPECT_EQ(7, GET_C1_CONFIG->m2);
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
    EXPECT_EQ(0, GET_C1_CONFIG->m1);
    EXPECT_EQ(7, GET_C1_CONFIG->m2);
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
    EXPECT_EQ(0, GET_C1_CONFIG->m1);
    EXPECT_EQ(7, GET_C1_CONFIG->m2);
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
    EXPECT_EQ(0, GET_C1_CONFIG->m1);
    EXPECT_EQ(7, GET_C1_CONFIG->m2);
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
    EXPECT_EQ(0, GET_C1_CONFIG->m1);
    EXPECT_EQ(7, GET_C1_CONFIG->m2);
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
const Simple_metadata s3_d = {{"count", 3, sizeof(unsigned), false}, nullptr, nullptr, nullptr};
const Simple_descriptor s3(&s3_d);
const Aggregate_data ca3_d = {&s3, 1, offsetof(struct m2, cnt)};
const Contained_aggregate ca3(&ca3_d);

const Simple_metadata s4_d = {{"owned", 4, sizeof(int), true}, cm_set_int, nullptr, nullptr};
const Simple_descriptor s4(&s4_d);
const Aggregate_data ca4_d = {&s4, MAX_NUMBER_OWNED, offsetof(struct m2, owned)};
const Owned_aggregate oa4(&ca4_d, &ca3);

const Aggregate * const aggrList2[] = {&ca3, &oa4};
const Composite_metadata c2_d = {{"c2", 1, sizeof(struct m2), true}, aggrList2, sizeof(aggrList2)/sizeof(aggrList2[0])};
const Composite_descriptor c2(&c2_d);

#define GET_C2_CONFIG ((struct m2 *)cm->getConfig())

class CfgMgrOwned : public testing::Test
{
protected:
    Nvram_spy * nvram;
    Config_manager * cm;

    //Define data accessible to test group members here.
    virtual void SetUp()
    {
        nvram = new Nvram_spy;
        cm = new Config_manager(&c2, nvram);
    }

    virtual void TearDown()
    {
        delete cm;
        delete nvram;
    }
};

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
    EXPECT_EQ(2, GET_C2_CONFIG->cnt);
    ASSERT_TRUE(GET_C2_CONFIG->owned != nullptr);
    EXPECT_EQ(7, GET_C2_CONFIG->owned[0]);
    EXPECT_EQ(8, GET_C2_CONFIG->owned[1]);
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
    EXPECT_EQ(2, GET_C2_CONFIG->cnt);

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
    EXPECT_EQ(1, GET_C2_CONFIG->cnt);
    ASSERT_TRUE(GET_C2_CONFIG->owned != nullptr);
    EXPECT_EQ(5, GET_C2_CONFIG->owned[0]);
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


// From default RAM start, do implicit add and check what's saved to TLV
TEST_F(CfgMgrOwned, implicitAdd)
{
    uint8_t expectedTlv [12] =
    { 1,0, 8,0, 4,0, 4,0, 0,0,0,0};
    /*T    L    T    L    V    */

    EXPECT_EQ(0, GET_C2_CONFIG->cnt);
    EXPECT_EQ(nullptr, GET_C2_CONFIG->owned);

    char * commandWord[] = {(char *)"owned", (char *)"0"}; // reference owned item 0, causing implicit add
    cm->handleCmd(2, commandWord);

    EXPECT_EQ(1, GET_C2_CONFIG->cnt);

    char * commandWord2[] = {(char *)"save"};
    cm->handleCmd(1, commandWord2);

    EXPECT_TRUE(nvram->match(expectedTlv, sizeof(expectedTlv)));
}


// From default RAM start, do implicit add and set and check what's saved to TLV
TEST_F(CfgMgrOwned, implicitAddnSet)
{
    uint8_t expectedTlv [12] =
    { 1,0, 8,0, 4,0, 4,0, 7,0,0,0};
    /*T    L    T    L    V    */

    EXPECT_EQ(0, GET_C2_CONFIG->cnt);
    EXPECT_EQ(nullptr, GET_C2_CONFIG->owned);

    char * commandWord[] = {(char *)"owned", (char *)"0", (char *)"=", (char *)"7"}; // set item 0, causing implicit add
    cm->handleCmd(4, commandWord);

    EXPECT_EQ(1, GET_C2_CONFIG->cnt);
    ASSERT_TRUE(GET_C2_CONFIG->owned != nullptr);
    EXPECT_EQ(7, GET_C2_CONFIG->owned[0]);

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

    EXPECT_EQ(0, GET_C2_CONFIG->cnt);
    EXPECT_EQ(nullptr, GET_C2_CONFIG->owned);

    char * commandWord[] = {(char *)"add", (char *)"owned"};
    cm->handleCmd(2, commandWord);

    EXPECT_EQ(1, GET_C2_CONFIG->cnt);
    EXPECT_TRUE(GET_C2_CONFIG->owned != nullptr);

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
    EXPECT_EQ(1, GET_C2_CONFIG->cnt);
    ASSERT_TRUE(GET_C2_CONFIG->owned != nullptr);
    EXPECT_EQ(5, GET_C2_CONFIG->owned[0]);

    char * commandWord2[] = {(char *)"del", (char *)"owned", (char *)"0"};
    cm->handleCmd(3, commandWord2);

    EXPECT_EQ(0, GET_C2_CONFIG->cnt);
    EXPECT_EQ(nullptr, GET_C2_CONFIG->owned);

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
const Simple_metadata s5_d = {{"name1", 1, sizeof(short int), true}, nullptr, nullptr, nullptr};
const Simple_descriptor s5(&s5_d);
const Aggregate_data ca5_d = {&s5, T3_ARRAY_SIZE, offsetof(struct m3, m1)};
const Contained_aggregate ca5(&ca5_d);

const Aggregate * const aggrList3[] = {&ca5};
const Composite_metadata c3_d = {{"c3", 1, sizeof(struct m3), true}, aggrList3, sizeof(aggrList3)/sizeof(aggrList3[0])};
const Composite_descriptor c3(&c3_d);

#define GET_C3_CONFIG ((struct m3 *)cm->getConfig())

class CfgMgrContainedArray : public testing::Test
{
protected:
    Nvram_spy * nvram;
    Config_manager * cm;

    //Define data accessible to test group members here.
    virtual void SetUp()
    {
        nvram = new Nvram_spy;
        cm = new Config_manager(&c3, nvram);
    }

    virtual void TearDown()
    {
        delete cm;
        delete nvram;
    }
};


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

    EXPECT_EQ(7, GET_C3_CONFIG->m1[0]);
    EXPECT_EQ(8, GET_C3_CONFIG->m1[1]);
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
const Simple_metadata s6_d = {{"name1", 1, sizeof(short int), true}, nullptr, nullptr, nullptr};
const Simple_descriptor s6(&s6_d);
const Aggregate_data ca6_d = {&s6, T6_ARRAY_SIZE, offsetof(struct m6, m1)};
const Contained_aggregate ca6(&ca6_d);

const Simple_metadata s7_d = {{"name2", 2, sizeof(short int), true}, nullptr, nullptr, nullptr};
const Simple_descriptor s7(&s7_d);
const Aggregate_data ca7_d = {&s7, T7_ARRAY_SIZE, offsetof(struct m6, m2)};
const Contained_aggregate ca7(&ca7_d);

const Aggregate * const aggrList4[] = {&ca6, &ca7};
const Composite_metadata c4_d = {{"c4", 1, sizeof(struct m6), true}, aggrList4, sizeof(aggrList4)/sizeof(aggrList4[0])};
const Composite_descriptor c4(&c4_d);

#define GET_C4_CONFIG ((struct m6 *)cm->getConfig())

class CfgMgrContainedArrays : public testing::Test
{
protected:
    Nvram_spy * nvram;
    Config_manager * cm;

    //Define data accessible to test group members here.
    virtual void SetUp()
    {
        nvram = new Nvram_spy;
        cm = new Config_manager(&c4, nvram);
    }

    virtual void TearDown()
    {
        delete cm;
        delete nvram;
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
    EXPECT_EQ(4, GET_C4_CONFIG->m1[0]);
    EXPECT_EQ(5, GET_C4_CONFIG->m1[1]);
    EXPECT_EQ(6, GET_C4_CONFIG->m2[0]);
    EXPECT_EQ(7, GET_C4_CONFIG->m2[1]);
}
} // namespace
