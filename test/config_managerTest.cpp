// Unit test using open-source unit test framework
// These tests the config manager as a whole, using the following interfaces:
// 1. Input: character strings passed to config_manager::handleCmd
// 2. Input/output: the binary file containing TLV data used by config_manager
//    for non-volatile storage
//
// xxx how many of these tests belong in cm_tlvTest.cpp?
// 

#include "gtest/gtest.h"
#include "config_manager.h"       // Unit under test
#include "config_manager_util.h"  // Extensions to unit under test (generic "set" functions)
#include "config_manager_setdef_null.h" // generic setdef function
#include "nvram_spy.h"

#include <string>
#include <string.h> // memcmp, strncmp, etc

using namespace std;

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
void setdef_t1(uint8_t *pItem, cm_item_len_t len)
{
    // Sanity check
    assert(len == sizeof(int));

    *((int *)pItem) = 7;
}


// test set 1 metadata
const cm_simple_metadata s1_d = {{"name1", 1, sizeof(int), true}, NULL, cm_setdef_null, NULL};
const cm_simple_descriptor s1(&s1_d);
const cm_aggregate_data ca1_d = {&s1, 1, offsetof(struct m, m1)};
const cm_contained_aggregate ca1(&ca1_d);

const cm_simple_metadata s2_d = {{"name2", 2, sizeof(int), true}, NULL, setdef_t1, NULL};
const cm_simple_descriptor s2(&s2_d);
const cm_aggregate_data ca2_d = {&s2, 1, offsetof(struct m, m2)};
const cm_contained_aggregate ca2(&ca2_d);

const cm_aggregate * const aggrList1[] = {&ca1, &ca2};
const cm_composite_metadata c1_d = {{"c1", 1, sizeof(struct m), true}, aggrList1, sizeof(aggrList1)/sizeof(aggrList1[0])};
const cm_composite_descriptor c1(&c1_d);

#define GET_C1_CONFIG ((struct m *)config_manager::getInstance()->getConfig())

class Contained : public testing::Test 
{
protected:
    config_manager * cm;

    //Define data accessible to test group members here.
    virtual void SetUp()
    {
        cm = config_manager::getInstance();
        nvram_spy_init();
    }

};


// Verify data saved to TLV, with default data in RAM as input to the test.
TEST_F(Contained, save)
{
    uint8_t expectedTlv [20] =
    /* The following assumes little-endian integers */
    /*T    L     T    L    V        T    L    V    */
    { 1,0, 16,0, 1,0, 4,0, 0,0,0,0, 2,0, 4,0, 7,0,0,0};

    cm->init(&c1);

    char * commandWord[] = {(char *)"save"};
    cm->handleCmd(1, commandWord);

    EXPECT_TRUE(nvram_spy_match(expectedTlv, sizeof(expectedTlv)));
}


// Verify what's loaded into memory, given TLV file that's read on startup.
TEST_F(Contained, load)
{
    uint8_t tlv[20] =
    /* The following assumes little-endian integers */
    /*T    L     T    L    V        T    L    V    */
    { 1,0, 16,0, 1,0, 4,0, 8,0,0,0, 2,0, 4,0, 9,0,0,0};


    /* Create config file to be loaded */
    nvram_spy_set(tlv, sizeof(tlv));

    cm->init(&c1);

    EXPECT_EQ(8, GET_C1_CONFIG->m1);
    EXPECT_EQ(9, GET_C1_CONFIG->m2);
}


// Verify what's loaded into memory, given TLV file with L of 2nd simple
// component that doesn't match the item descriptor.
TEST_F(Contained, loadChangedSimpleLen)
{
    uint8_t tlv[20] =
    /* The following assumes little-endian integers */
    /*T    L     T    L    V        T    L    V    */
    { 1,0, 14,0, 1,0, 4,0, 8,0,0,0, 2,0, 2,0, 9,0};


    /* Create config file to be loaded */
    nvram_spy_set(tlv, sizeof(tlv));

    cm->init(&c1);

    EXPECT_EQ(8, GET_C1_CONFIG->m1);
    EXPECT_EQ(7, GET_C1_CONFIG->m2); // Default; not read from the file, because TLV's L is bad
}


// Verify what's loaded into memory, given a TLV file that's read on startup that contains
// an unknown Type value.
// Unknown type in file: the descriptor has no T=9, so it's ignored by cfg_man when found in file,
// but the item following it is loaded.
TEST_F(Contained, loadUnknown)
{
    uint8_t tlv[28] =
    /* The following assumes little-endian integers */
    /*T    L     T    L    V        T    L    V        T    L    V    */
    { 1,0, 24,0, 1,0, 4,0, 0,0,0,0, 9,0, 4,0, 0,0,0,0, 2,0, 4,0, 0,0,0,0};
    uint8_t expectedTlv[20] =
    /* The following assumes little-endian integers */
    /*T    L     T    L    V        T    L    V    */
    { 1,0, 16,0, 1,0, 4,0, 0,0,0,0, 2,0, 4,0, 0,0,0,0};


    /* Create config file to be loaded */
    nvram_spy_set(tlv, sizeof(tlv));

    cm->init(&c1);

    char * commandWord[] = {(char *)"save"};
    cm->handleCmd(1, commandWord);

    // See what CM made of the file it loaded
    EXPECT_TRUE(nvram_spy_match(expectedTlv, sizeof(expectedTlv)));
}


// Verify what's saved to TLV, given a TLV file that's read on startup that's
// missing an element of a structure.
// The element m1 that's not in the TLV is saved to TLV, populated with default value.
// The element m2 that is in TLV is restored to TLV.
TEST_F(Contained, loadMissing)
{
    uint8_t tlv[12] =
    /* The following assumes little-endian integers */
    /*T    L    T    L    V    */
    { 1,0, 8,0, 2,0, 4,0, 6,7,8,9};
    uint8_t expectedTlv[20] =
    /* The following assumes little-endian integers */
    /*T    L     T    L    V        T    L    V    */
    { 1,0, 16,0, 1,0, 4,0, 0,0,0,0, 2,0, 4,0, 6,7,8,9};


    /* Create config file to be loaded */
    nvram_spy_set(tlv, sizeof(tlv));

    cm->init(&c1);

    char * commandWord[] = {(char *)"save"};
    cm->handleCmd(1, commandWord);

    // See what CM made of the file it loaded
    EXPECT_TRUE(nvram_spy_match(expectedTlv, sizeof(expectedTlv)));
}


// Load file with last item missing from CONTAINED composite
// Load fails, so defaults are set.
TEST_F(Contained, loadTruncated)
{
    uint8_t tlv[] =
    /* The following assumes little-endian integers */
    /*T    L     T    L    V       ...    */
    { 1,0, 16,0, 1,0, 4,0, 8,0,0,0 /*... */};


    /* Create config file to be loaded */
    nvram_spy_set(tlv, sizeof(tlv));
    
    cm->init(&c1);

    EXPECT_EQ(0, GET_C1_CONFIG->m1);
    EXPECT_EQ(7, GET_C1_CONFIG->m2);
}


// Load file with partial last item in CONTAINED composite
// Load fails, so defaults are set.
TEST_F(Contained, loadTruncated1)
{
    uint8_t tlv[] =
    /* The following assumes little-endian integers */
    /*T    L     T    L    V        T ...    */
    { 1,0, 16,0, 1,0, 4,0, 8,0,0,0, 2,0};


    /* Create config file to be loaded */
    nvram_spy_set(tlv, sizeof(tlv));

    cm->init(&c1);

    EXPECT_EQ(0, GET_C1_CONFIG->m1);
    EXPECT_EQ(7, GET_C1_CONFIG->m2);
}


// Load file with partial last item in CONTAINED composite
// Load fails, so defaults are set.
TEST_F(Contained, loadTruncated2)
{
    uint8_t tlv[] =
    /* The following assumes little-endian integers */
    /*T    L     T    L    V        T    L    V (last byte missing) */
    { 1,0, 16,0, 1,0, 4,0, 8,0,0,0, 2,0, 4,0, 9,0,0};


    /* Create config file to be loaded */
    nvram_spy_set(tlv, sizeof(tlv));

    cm->init(&c1);

    EXPECT_EQ(0, GET_C1_CONFIG->m1);
    EXPECT_EQ(7, GET_C1_CONFIG->m2);
}


// Load file with incoherent CONTAINED composite (sum of the sizes
// of components is larger than the size of the composite).
// Load fails, so defaults are set.
TEST_F(Contained, loadIncoherent)
{
    uint8_t tlv[20] =
    /* The following assumes little-endian integers */
    /*T    L     T    L    V        T    L    V    */
    { 1,0, 14,0, 1,0, 4,0, 8,0,0,0, 2,0, 4,0, 9,0,0,0};


    /* Create config file to be loaded */
    nvram_spy_set(tlv, sizeof(tlv));

    cm->init(&c1);

    EXPECT_EQ(0, GET_C1_CONFIG->m1);
    EXPECT_EQ(7, GET_C1_CONFIG->m2);
}


// Load file with incoherent CONTAINED composite (sum of the sizes
// of components is larger than the size of the composite).
// Load fails, so defaults are set.
TEST_F(Contained, loadIncoherent1)
{
    uint8_t tlv[20] =
    /* The following assumes little-endian integers */
    /*T    L     T    L    V        T    L    V    */
    { 1,0, 17,0, 1,0, 4,0, 8,0,0,0, 2,0, 4,0, 9,0,0,0};


    /* Create config file to be loaded */
    nvram_spy_set(tlv, sizeof(tlv));

    cm->init(&c1);

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
const cm_simple_metadata s3_d = {{"count", 3, sizeof(unsigned), false}, NULL, NULL, NULL};
const cm_simple_descriptor s3(&s3_d);
const cm_aggregate_data ca3_d = {&s3, 1, offsetof(struct m2, cnt)};
const cm_contained_aggregate ca3(&ca3_d);

const cm_simple_metadata s4_d = {{"owned", 4, sizeof(int), true}, cm_set_int, NULL, NULL};
const cm_simple_descriptor s4(&s4_d);
const cm_aggregate_data ca4_d = {&s4, MAX_NUMBER_OWNED, offsetof(struct m2, owned)};
const cm_owned_aggregate oa4(&ca4_d, &ca3);

const cm_aggregate * const aggrList2[] = {&ca3, &oa4};
const cm_composite_metadata c2_d = {{"c2", 1, sizeof(struct m2), true}, aggrList2, sizeof(aggrList2)/sizeof(aggrList2[0])};
const cm_composite_descriptor c2(&c2_d);

#define GET_C2_CONFIG ((struct m2 *)config_manager::getInstance()->getConfig())

class Owned : public testing::Test 
{
protected:
    config_manager * cm;

    //Define data accessible to test group members here.
    virtual void SetUp()
    {
        cm = config_manager::getInstance();
        nvram_spy_init();
    }
};

// Verify what's loaded into memory, given TLV file that's read on startup.
TEST_F(Owned, load)
{
    uint8_t tlv[20] =
    /*T    L     T    L    V        T    L    V      */
    { 1,0, 16,0, 4,0, 4,0, 7,0,0,0, 4,0, 4,0, 8,0,0,0};


    /* Create config file to be loaded */
    nvram_spy_set(tlv, sizeof(tlv));

    cm->init(&c2);

    EXPECT_EQ(2, GET_C2_CONFIG->cnt);
    ASSERT_TRUE(GET_C2_CONFIG->owned != NULL);
    EXPECT_EQ(7, GET_C2_CONFIG->owned[0]);
    EXPECT_EQ(8, GET_C2_CONFIG->owned[1]);
}


// Verify what's saved to TLV, given a TLV file that's read on startup that
// has more than the max number of an OWNED component.
//
TEST_F(Owned, loadTooMany)
{
    uint8_t tlv[28] =
    /*T    L     T    L    V        T    L    V        T    L    V       */
    { 1,0, 24,0, 4,0, 4,0, 7,0,0,0, 4,0, 4,0, 8,0,0,0, 4,0, 4,0, 9,0,0,0};
    uint8_t expectedTlv[20] =
    /* The following assumes little-endian integers */
    /*T    L     T    L    V        T    L    V    */
    { 1,0, 16,0, 4,0, 4,0, 7,0,0,0, 4,0, 4,0, 8,0,0,0};


    /* Create config file to be loaded */
    nvram_spy_set(tlv, sizeof(tlv));

    cm->init(&c2);

    EXPECT_EQ(2, GET_C2_CONFIG->cnt);

    char * commandWord[] = {(char *)"save"};
    cm->handleCmd(1, commandWord);

    // See what CM made of the file it loaded
    EXPECT_TRUE(nvram_spy_match(expectedTlv, sizeof(expectedTlv)));
}


// Verify what's saved to TLV, given a TLV file that's read on startup that
// contains the ID of a non-persistent item (the counter).  It should
// be ignored, like an unknown ID is.
//
TEST_F(Owned, loadNonPersistent)
{
    uint8_t tlv[20] =
    /*T    L     T    L    V        T    L    V      */
    { 1,0, 16,0, 3,0, 4,0, 1,0,0,0, 4,0, 4,0, 5,0,0,0};


    /* Create config file to be loaded */
    nvram_spy_set(tlv, sizeof(tlv));

    cm->init(&c2);

    EXPECT_EQ(1, GET_C2_CONFIG->cnt);
    ASSERT_TRUE(GET_C2_CONFIG->owned != NULL);
    EXPECT_EQ(5, GET_C2_CONFIG->owned[0]);
}


// Verify data saved to TLV, with default data in RAM as input to the test.
// The contained component is saved, but not the owned, which does not exist.
TEST_F(Owned, save)
{
    uint8_t expectedTlv [] =
    /* The following assumes little-endian integers */
    /*T    L     T    L    V     */
    {1,0,  8,0,  3,0, 4,0, 0,0,0,0};

    cm->init(&c2);

    char * commandWord[] = {(char *)"save"};
    cm->handleCmd(1, commandWord);

    EXPECT_TRUE(nvram_spy_match(expectedTlv, sizeof(expectedTlv)));
}


// From default RAM start, do implicit add and check what's saved to TLV
TEST_F(Owned, implicitAdd)
{
    uint8_t expectedTlv [12] =
    /* The following assumes little-endian integers */
    /*T    L    T    L    V    */
    { 1,0, 8,0, 4,0, 4,0, 0,0,0,0};

    cm->init(&c2);

    EXPECT_EQ(0, GET_C2_CONFIG->cnt);
    EXPECT_EQ(NULL, GET_C2_CONFIG->owned);   

    char * commandWord[] = {(char *)"owned", (char *)"0"}; // reference owned item 0, causing implicit add
    cm->handleCmd(2, commandWord);

    EXPECT_EQ(1, GET_C2_CONFIG->cnt);

    char * commandWord2[] = {(char *)"save"};
    cm->handleCmd(1, commandWord2);

    EXPECT_TRUE(nvram_spy_match(expectedTlv, sizeof(expectedTlv)));
}


// From default RAM start, do implicit add and set and check what's saved to TLV
TEST_F(Owned, implicitAddnSet)
{
    uint8_t expectedTlv [12] =
    /* The following assumes little-endian integers */
    /*T    L    T    L    V    */
    { 1,0, 8,0, 4,0, 4,0, 7,0,0,0};

    cm->init(&c2);

    EXPECT_EQ(0, GET_C2_CONFIG->cnt);
    EXPECT_EQ(NULL, GET_C2_CONFIG->owned);   

    char * commandWord[] = {(char *)"owned", (char *)"0", (char *)"=", (char *)"7"}; // set item 0, causing implicit add
    cm->handleCmd(4, commandWord);

    EXPECT_EQ(1, GET_C2_CONFIG->cnt);
    ASSERT_TRUE(GET_C2_CONFIG->owned != NULL);
    EXPECT_EQ(7, GET_C2_CONFIG->owned[0]);   

    char * commandWord2[] = {(char *)"save"};
    cm->handleCmd(1, commandWord2);

    EXPECT_TRUE(nvram_spy_match(expectedTlv, sizeof(expectedTlv)));
}


// From default RAM start, do explicit add and check what's saved to TLV
TEST_F(Owned, explicitAdd)
{
    uint8_t expectedTlv [12] =
    /* The following assumes little-endian integers */
    /*T    L    T    L    V    */
    { 1,0, 8,0, 4,0, 4,0, 0,0,0,0};


    cm->init(&c2);

    EXPECT_EQ(0, GET_C2_CONFIG->cnt);
    EXPECT_EQ(NULL, GET_C2_CONFIG->owned);

    char * commandWord[] = {(char *)"add", (char *)"owned"};
    cm->handleCmd(2, commandWord);

    EXPECT_EQ(1, GET_C2_CONFIG->cnt);
    EXPECT_TRUE(GET_C2_CONFIG->owned != NULL);

    char * commandWord2[] = {(char *)"save"};
    cm->handleCmd(1, commandWord2);

    EXPECT_TRUE(nvram_spy_match(expectedTlv, sizeof(expectedTlv)));
}


// Verify what's saved to TLV, given a TLV file that's read on startup,
// and a delete operation.
// xxx this should save empty file -- better way to test this than memcmp?
// Or replace with a test where something remains after deletion?
TEST_F(Owned, del)
{
    uint8_t tlv[12] =
    /* The following assumes little-endian integers */
    /*T    L    T    L    V    */
    { 1,0, 8,0, 4,0, 4,0, 5,0,0,0};
    uint8_t expectedTlv [0] =
    /* The following assumes little-endian integers */
    /*T    L  */
    {};


    /* Create config file to be loaded */
    nvram_spy_set(tlv, sizeof(tlv));

    cm->init(&c2);

    EXPECT_EQ(1, GET_C2_CONFIG->cnt);
    ASSERT_TRUE(GET_C2_CONFIG->owned != NULL);
    EXPECT_EQ(5, GET_C2_CONFIG->owned[0]);

    char * commandWord[] = {(char *)"del", (char *)"owned"};
    cm->handleCmd(2, commandWord);

    EXPECT_EQ(0, GET_C2_CONFIG->cnt);
    EXPECT_EQ(NULL, GET_C2_CONFIG->owned);

    char * commandWord2[] = {(char *)"save"};
    cm->handleCmd(1, commandWord2);

    // See what CM made of the file it loaded
    EXPECT_TRUE(nvram_spy_match(expectedTlv, sizeof(expectedTlv)));
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
const cm_simple_metadata s5_d = {{"name1", 1, sizeof(short int), true}, NULL, NULL, NULL};
const cm_simple_descriptor s5(&s5_d);
const cm_aggregate_data ca5_d = {&s5, T3_ARRAY_SIZE, offsetof(struct m3, m1)};
const cm_contained_aggregate ca5(&ca5_d);

const cm_aggregate * const aggrList3[] = {&ca5};
const cm_composite_metadata c3_d = {{"c3", 1, sizeof(struct m3), true}, aggrList3, sizeof(aggrList3)/sizeof(aggrList3[0])};
const cm_composite_descriptor c3(&c3_d);

#define GET_C3_CONFIG ((struct m3 *)config_manager::getInstance()->getConfig())

class ContainedArray : public testing::Test 
{
protected:
    config_manager * cm;

    //Define data accessible to test group members here.
    virtual void SetUp()
    {
        cm = config_manager::getInstance();
        nvram_spy_init();
    }
};


// Verify what's loaded into memory, given TLV file that's read on startup.
TEST_F(ContainedArray, load)
{
    uint8_t tlv[16] =
    /*T    L     T    L    V    T    L    V */
    { 1,0, 12,0, 1,0, 2,0, 7,0, 1,0, 2,0, 8,0};

    /* Create config file to be loaded */
    nvram_spy_set(tlv, sizeof(tlv));
    cm->init(&c3);
    EXPECT_EQ(7, GET_C3_CONFIG->m1[0]);
    EXPECT_EQ(8, GET_C3_CONFIG->m1[1]);
}


// Verify what's saved to TLV, given a TLV file that's read on startup that
// has more than the max number of a CONTAINED component.
//
TEST_F(ContainedArray, loadTooMany)
{
    uint8_t tlv[22] =
    /*T    L     T    L    V    T    L    V    T    L    V       */
    { 1,0, 18,0, 1,0, 2,0, 7,0, 1,0, 2,0, 8,0, 1,0, 2,0, 9,0};
    uint8_t expectedTlv[16] =
    /* The following assumes little-endian integers */
    /*T    L     T    L    V    T    L    V    */
    { 1,0, 12,0, 1,0, 2,0, 7,0, 1,0, 2,0, 8,0};

    /* Create config file to be loaded */
    nvram_spy_set(tlv, sizeof(tlv));

    cm->init(&c3);

    char * commandWord[] = {(char *)"save"};
    cm->handleCmd(1, commandWord);

    // See what CM made of the file it loaded
    EXPECT_TRUE(nvram_spy_match(expectedTlv, sizeof(expectedTlv)));
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
const cm_simple_metadata s6_d = {{"name1", 1, sizeof(short int), true}, NULL, NULL, NULL};
const cm_simple_descriptor s6(&s6_d);
const cm_aggregate_data ca6_d = {&s6, T6_ARRAY_SIZE, offsetof(struct m6, m1)};
const cm_contained_aggregate ca6(&ca6_d);

const cm_simple_metadata s7_d = {{"name2", 2, sizeof(short int), true}, NULL, NULL, NULL};
const cm_simple_descriptor s7(&s7_d);
const cm_aggregate_data ca7_d = {&s7, T7_ARRAY_SIZE, offsetof(struct m6, m2)};
const cm_contained_aggregate ca7(&ca7_d);

const cm_aggregate * const aggrList4[] = {&ca6, &ca7};
const cm_composite_metadata c4_d = {{"c4", 1, sizeof(struct m6), true}, aggrList4, sizeof(aggrList4)/sizeof(aggrList4[0])};
const cm_composite_descriptor c4(&c4_d);

#define GET_C4_CONFIG ((struct m6 *)config_manager::getInstance()->getConfig())

class ContainedArrays : public testing::Test 
{
protected:
    config_manager * cm;

    //Define data accessible to test group members here.
    virtual void SetUp()
    {
        cm = config_manager::getInstance();
        nvram_spy_init();
    }
};


// Verify what's loaded into memory, given TLV file that's read on startup.
TEST_F(ContainedArrays, load)
{    
    uint8_t tlv[28] =
    /* The following assumes little-endian integers */
    /*T    L     T    L    V    T    L    V    T    L    V    T    L    V */
    { 1,0, 24,0, 1,0, 2,0, 4,0, 1,0, 2,0, 5,0, 2,0, 2,0, 6,0, 2,0, 2,0, 7,0};

    nvram_spy_set(tlv, sizeof(tlv));

    cm->init(&c4);

    EXPECT_EQ(4, GET_C4_CONFIG->m1[0]);
    EXPECT_EQ(5, GET_C4_CONFIG->m1[1]);
    EXPECT_EQ(6, GET_C4_CONFIG->m2[0]);
    EXPECT_EQ(7, GET_C4_CONFIG->m2[1]);
}
} // namespace