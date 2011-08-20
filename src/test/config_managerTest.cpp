// Unit test using open-source unit test framework
// These tests the config manager as a whole, using the following interfaces:
// 1. Input: character strings passed to config_manager::handleCmd
// 2. Input/output: the binary file containing TLV data used by config_manager
//    for non-volatile storage
//
//

#include "CppUTest/TestHarness.h"
#include "CppUTest/CommandLineTestRunner.h"
#include "config_manager.h"       // Unit under test
#include "config_manager_util.h"  // Extensions to unit under test (generic "set" functions)
#include "config_manager_setdef_null.h" // generic setdef function

#include <string>
#include <string.h> // memcmp, strncmp, etc

using namespace std;


int main(int argc, char** argv)
{
    return RUN_ALL_TESTS(argc, argv);
}


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
const cm_simple_metadata s1_d = {{"name1", 1, sizeof(int)}, NULL, cm_setdef_null, NULL};
const cm_simple_descriptor s1(&s1_d, true);
const cm_aggregate_data ca1_d = {&s1, 1, offsetof(struct m, m1)};
const cm_contained_aggregate ca1(&ca1_d);

const cm_simple_metadata s2_d = {{"name2", 2, sizeof(int)}, NULL, setdef_t1, NULL};
const cm_simple_descriptor s2(&s2_d, true);
const cm_aggregate_data ca2_d = {&s2, 1, offsetof(struct m, m2)};
const cm_contained_aggregate ca2(&ca2_d);

const cm_aggregate * const aggrList1[] = {&ca1, &ca2};
const cm_composite_metadata c1_d = {{"c1", 1, sizeof(struct m)}, aggrList1, sizeof(aggrList1)/sizeof(aggrList1[0])};
const cm_composite_descriptor c1(&c1_d, true);

#define GET_C1_CONFIG ((struct m *)config_manager::getInstance()->getConfig())

TEST_GROUP(contained)
{
    FILE *           fp;
    config_manager * cm;

    //Define data accessible to test group members here.
    void setup()
    {
        cm = config_manager::getInstance();

    }
    
    void teardown()
    {
        //clean up steps are executed after each TEST
    }
};


// Verify data saved to TLV, with default data in RAM as input to the test.
TEST(contained, save)
{
    uint8_t expectedTlv [20] =
    /* The following assumes little-endian integers */
    /*T    L     T    L    V        T    L    V    */
    { 1,0, 16,0, 1,0, 4,0, 0,0,0,0, 2,0, 4,0, 7,0,0,0};
    uint8_t actualTlv [20];

    // Remove the bin file to ensure RAM is init'd with default values
    remove(CFG_FILE_NAME);

    cm->init(&c1);

    char * commandWord[] = {(char *)"save"};
    cm->handleCmd(1, commandWord);

    if ((fp = fopen(CFG_FILE_NAME, "rb")) == NULL)
    {
        FAIL("Couldn't open file");
    }

    fread(actualTlv, sizeof(actualTlv), 1, fp);

    CHECK(memcmp(expectedTlv, actualTlv, sizeof(expectedTlv)) == 0);
    fclose(fp);
}


// Verify what's loaded into memory, given TLV file that's read on startup.
TEST(contained, load)
{
    uint8_t tlv[20] =
    /* The following assumes little-endian integers */
    /*T    L     T    L    V        T    L    V    */
    { 1,0, 16,0, 1,0, 4,0, 8,0,0,0, 2,0, 4,0, 9,0,0,0};


    /* Create config file to be loaded */
    if ((fp = fopen(CFG_FILE_NAME, "wb")) == NULL)
    {
        FAIL("Couldn't open file");
    }

    fwrite(tlv, sizeof(tlv), 1, fp);
    fclose(fp);

    cm->init(&c1);

    CHECK(GET_C1_CONFIG->m1 == 8);
    CHECK(GET_C1_CONFIG->m2 == 9);
}


// Verify what's loaded into memory, given TLV file with L of 2nd simple
// component that doesn't match the item descriptor.
TEST(contained, loadChangedSimpleLen)
{
    uint8_t tlv[20] =
    /* The following assumes little-endian integers */
    /*T    L     T    L    V        T    L    V    */
    { 1,0, 14,0, 1,0, 4,0, 8,0,0,0, 2,0, 2,0, 9,0};


    /* Create config file to be loaded */
    if ((fp = fopen(CFG_FILE_NAME, "wb")) == NULL)
    {
        FAIL("Couldn't open file");
    }

    fwrite(tlv, sizeof(tlv), 1, fp);
    fclose(fp);

    cm->init(&c1);

    CHECK(GET_C1_CONFIG->m1 == 8);
    CHECK(GET_C1_CONFIG->m2 == 7); // Default; not read from the file, because TLV's L is bad
}


// Verify what's loaded into memory, given a TLV file that's read on startup that contains
// an unknown Type value.
// Unknown type in file: the descriptor has no T=9, so it's ignored by cfg_man when found in file,
// but the item following it is loaded.
TEST(contained, loadUnknown)
{
    uint8_t tlv[28] =
    /* The following assumes little-endian integers */
    /*T    L     T    L    V        T    L    V        T    L    V    */
    { 1,0, 24,0, 1,0, 4,0, 0,0,0,0, 9,0, 4,0, 0,0,0,0, 2,0, 4,0, 0,0,0,0};
    uint8_t expectedTlv[20] =
    /* The following assumes little-endian integers */
    /*T    L     T    L    V        T    L    V    */
    { 1,0, 16,0, 1,0, 4,0, 0,0,0,0, 2,0, 4,0, 0,0,0,0};
    uint8_t savedTlv[20];


    /* Create config file to be loaded */
    if ((fp = fopen(CFG_FILE_NAME, "wb")) == NULL)
    {
        FAIL("Couldn't open file");
    }

    fwrite(tlv, sizeof(tlv), 1, fp);
    fclose(fp);    

    cm->init(&c1);

    char * commandWord[] = {(char *)"save"};
    cm->handleCmd(1, commandWord);

    // See what CM made of the file it loaded
    if ((fp = fopen(CFG_FILE_NAME, "rb")) == NULL)
    {
        FAIL("Couldn't open file");
    }

    fread(savedTlv, sizeof(savedTlv), 1, fp);

    CHECK(memcmp(expectedTlv, savedTlv, sizeof(savedTlv)) == 0);
    fclose(fp);    
}


// Verify what's saved to TLV, given a TLV file that's read on startup that's
// missing an element of a structure.
// The element that's not in the TLV is saved to TLV, populated with default value.
TEST(contained, loadMissing)
{
    uint8_t tlv[12] =
    /* The following assumes little-endian integers */
    /*T    L     T    L    V    */
    { 1,0, 8,0, 2,0, 4,0, 0,0,0,0};
    uint8_t expectedTlv[20] =
    /* The following assumes little-endian integers */
    /*T    L     T    L    V        T    L    V    */
    { 1,0, 16,0, 1,0, 4,0, 0,0,0,0, 2,0, 4,0, 0,0,0,0};
    uint8_t savedTlv[20];


    /* Create config file to be loaded */
    if ((fp = fopen(CFG_FILE_NAME, "wb")) == NULL)
    {
        FAIL("Couldn't open file");
    }

    fwrite(tlv, sizeof(tlv), 1, fp);
    fclose(fp);    

    cm->init(&c1);

    char * commandWord[] = {(char *)"save"};
    cm->handleCmd(1, commandWord);

    // See what CM made of the file it loaded
    if ((fp = fopen(CFG_FILE_NAME, "rb")) == NULL)
    {
        FAIL("Couldn't open file");
    }

    fread(savedTlv, sizeof(savedTlv), 1, fp);

    CHECK(memcmp(expectedTlv, savedTlv, sizeof(savedTlv)) == 0);
    fclose(fp);    
}


// Load file with last item missing from CONTAINED composite
// Load fails, so defaults are set.
TEST(contained, loadTruncated)
{
    uint8_t tlv[] =
    /* The following assumes little-endian integers */
    /*T    L     T    L    V       ...    */
    { 1,0, 16,0, 1,0, 4,0, 8,0,0,0 /*... */};


    /* Create config file to be loaded */
    if ((fp = fopen(CFG_FILE_NAME, "wb")) == NULL)
    {
        FAIL("Couldn't open file");
    }

    fwrite(tlv, sizeof(tlv), 1, fp);
    fclose(fp);

    cm->init(&c1);

    CHECK(GET_C1_CONFIG->m1 == 0);
    CHECK(GET_C1_CONFIG->m2 == 7);
}


// Load file with partial last item in CONTAINED composite
// Load fails, so defaults are set.
TEST(contained, loadTruncated1)
{
    uint8_t tlv[] =
    /* The following assumes little-endian integers */
    /*T    L     T    L    V        T ...    */
    { 1,0, 16,0, 1,0, 4,0, 8,0,0,0, 2,0};


    /* Create config file to be loaded */
    if ((fp = fopen(CFG_FILE_NAME, "wb")) == NULL)
    {
        FAIL("Couldn't open file");
    }

    fwrite(tlv, sizeof(tlv), 1, fp);
    fclose(fp);

    cm->init(&c1);

    CHECK(GET_C1_CONFIG->m1 == 0);
    CHECK(GET_C1_CONFIG->m2 == 7);
}


// Load file with partial last item in CONTAINED composite
// Load fails, so defaults are set.
TEST(contained, loadTruncated2)
{
    uint8_t tlv[] =
    /* The following assumes little-endian integers */
    /*T    L     T    L    V        T    L    V (last byte missing) */
    { 1,0, 16,0, 1,0, 4,0, 8,0,0,0, 2,0, 4,0, 9,0,0};


    /* Create config file to be loaded */
    if ((fp = fopen(CFG_FILE_NAME, "wb")) == NULL)
    {
        FAIL("Couldn't open file");
    }

    fwrite(tlv, sizeof(tlv), 1, fp);
    fclose(fp);

    cm->init(&c1);

    CHECK(GET_C1_CONFIG->m1 == 0);
    CHECK(GET_C1_CONFIG->m2 == 7);
}


// Load file with incoherent CONTAINED composite (sum of the sizes
// of components is larger than the size of the composite).
// Load fails, so defaults are set.
TEST(contained, loadIncoherent)
{
    uint8_t tlv[20] =
    /* The following assumes little-endian integers */
    /*T    L     T    L    V        T    L    V    */
    { 1,0, 14,0, 1,0, 4,0, 8,0,0,0, 2,0, 4,0, 9,0,0,0};


    /* Create config file to be loaded */
    if ((fp = fopen(CFG_FILE_NAME, "wb")) == NULL)
    {
        FAIL("Couldn't open file");
    }

    fwrite(tlv, sizeof(tlv), 1, fp);
    fclose(fp);

    cm->init(&c1);

    CHECK(GET_C1_CONFIG->m1 == 0);
    CHECK(GET_C1_CONFIG->m2 == 7);
}


// Load file with incoherent CONTAINED composite (sum of the sizes
// of components is larger than the size of the composite).
// Load fails, so defaults are set.
TEST(contained, loadIncoherent1)
{
    uint8_t tlv[20] =
    /* The following assumes little-endian integers */
    /*T    L     T    L    V        T    L    V    */
    { 1,0, 17,0, 1,0, 4,0, 8,0,0,0, 2,0, 4,0, 9,0,0,0};


    /* Create config file to be loaded */
    if ((fp = fopen(CFG_FILE_NAME, "wb")) == NULL)
    {
        FAIL("Couldn't open file");
    }

    fwrite(tlv, sizeof(tlv), 1, fp);
    fclose(fp);

    cm->init(&c1);

    CHECK(GET_C1_CONFIG->m1 == 0);
    CHECK(GET_C1_CONFIG->m2 == 7);
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
const cm_simple_metadata s3_d = {{"count", 3, sizeof(unsigned)}, NULL, NULL, NULL};
const cm_simple_descriptor s3(&s3_d, false); // false => counter is not saved to NVRAM
const cm_aggregate_data ca3_d = {&s3, 1, offsetof(struct m2, cnt)};
const cm_contained_aggregate ca3(&ca3_d);

const cm_simple_metadata s4_d = {{"owned", 4, sizeof(int *)}, cm_set_int, NULL, NULL};
const cm_simple_descriptor s4(&s4_d, true);
const cm_aggregate_data ca4_d = {&s4, MAX_NUMBER_OWNED, offsetof(struct m2, owned)};
const cm_owned_aggregate oa4(&ca4_d, &ca3);

const cm_aggregate * const aggrList2[] = {&ca3, &oa4};
const cm_composite_metadata c2_d = {{"c2", 1, sizeof(struct m2)}, aggrList2, sizeof(aggrList2)/sizeof(aggrList2[0])};
const cm_composite_descriptor c2(&c2_d, true);

#define GET_C2_CONFIG ((struct m2 *)config_manager::getInstance()->getConfig())

TEST_GROUP(owned)
{
    FILE *           fp;
    config_manager * cm;

    //Define data accessible to test group members here.
    void setup()
    {
        cm = config_manager::getInstance();

    }
    
    void teardown()
    {
        //clean up steps are executed after each TEST
    }
};


// Verify what's loaded into memory, given TLV file that's read on startup.
TEST(owned, load)
{
    uint8_t tlv[20] =
    /*T    L     T    L    V        T    L    V      */
    { 1,0, 16,0, 4,0, 4,0, 7,0,0,0, 4,0, 4,0, 8,0,0,0};


    /* Create config file to be loaded */
    if ((fp = fopen(CFG_FILE_NAME, "wb")) == NULL)
    {
        FAIL("Couldn't open file");
    }

    fwrite(tlv, sizeof(tlv), 1, fp);
    fclose(fp);

    cm->init(&c2);

    CHECK(GET_C2_CONFIG->cnt == 2);
    CHECK(GET_C2_CONFIG->owned[0] == 7);
    CHECK(GET_C2_CONFIG->owned[1] == 8);
}


// Verify what's saved to TLV, given a TLV file that's read on startup that
// has more than the max number of an OWNED component.
//
TEST(owned, loadTooMany)
{
    uint8_t tlv[28] =
    /*T    L     T    L    V        T    L    V        T    L    V       */
    { 1,0, 24,0, 4,0, 4,0, 7,0,0,0, 4,0, 4,0, 8,0,0,0, 4,0, 4,0, 9,0,0,0};
    uint8_t expectedTlv[20] =
    /* The following assumes little-endian integers */
    /*T    L     T    L    V        T    L    V    */
    { 1,0, 16,0, 4,0, 4,0, 7,0,0,0, 4,0, 4,0, 8,0,0,0};
    uint8_t savedTlv[20];


    /* Create config file to be loaded */
    if ((fp = fopen(CFG_FILE_NAME, "wb")) == NULL)
    {
        FAIL("Couldn't open file");
    }

    fwrite(tlv, sizeof(tlv), 1, fp);
    fclose(fp);    

    cm->init(&c2);

    CHECK(GET_C2_CONFIG->cnt == 2);

    char * commandWord[] = {(char *)"save"};
    cm->handleCmd(1, commandWord);

    // See what CM made of the file it loaded
    if ((fp = fopen(CFG_FILE_NAME, "rb")) == NULL)
    {
        FAIL("Couldn't open file");
    }

    fread(savedTlv, sizeof(savedTlv), 1, fp);

    CHECK(memcmp(expectedTlv, savedTlv, sizeof(savedTlv)) == 0);
    fclose(fp);    
}


// Verify data saved to TLV, with default data in RAM as input to the test.
TEST(owned, save)
{
    uint8_t expectedTlv [4] =
    /* The following assumes little-endian integers */
    /*T    L     T    L    V        T    L    V    */
    { 1,0, 0,0};
    uint8_t actualTlv [4];


    // Remove the bin file to ensure RAM is init'd with default values
    remove(CFG_FILE_NAME);    

    cm->init(&c2);

    char * commandWord[] = {(char *)"save"};
    cm->handleCmd(1, commandWord);

    if ((fp = fopen(CFG_FILE_NAME, "rb")) == NULL)
    {
        FAIL("Couldn't open file");
    }

    fread(actualTlv, sizeof(actualTlv), 1, fp);

    CHECK(memcmp(expectedTlv, actualTlv, sizeof(expectedTlv)) == 0);
    fclose(fp);    
}


// From default RAM start, do implicit add and check what's saved to TLV
TEST(owned, implicitAdd)
{
    uint8_t expectedTlv [12] =
    /* The following assumes little-endian integers */
    /*T    L    T    L    V    */
    { 1,0, 8,0, 4,0, 4,0, 0,0,0,0};
    uint8_t actualTlv [12];


    // Remove the bin file to ensure RAM is init'd with default values
    remove(CFG_FILE_NAME);    

    cm->init(&c2);

    CHECK(GET_C2_CONFIG->cnt == 0);
    CHECK(GET_C2_CONFIG->owned == NULL);   

    char * commandWord[] = {(char *)"owned", (char *)"0"}; // reference owned item 0, causing implicit add
    cm->handleCmd(2, commandWord);

    CHECK(GET_C2_CONFIG->cnt == 1);

    char * commandWord2[] = {(char *)"save"};
    cm->handleCmd(1, commandWord2);

    if ((fp = fopen(CFG_FILE_NAME, "rb")) == NULL)
    {
        FAIL("Couldn't open file");
    }

    fread(actualTlv, sizeof(actualTlv), 1, fp);

    CHECK(memcmp(expectedTlv, actualTlv, sizeof(expectedTlv)) == 0);
    fclose(fp);
}


// From default RAM start, do implicit add and set and check what's saved to TLV
TEST(owned, implicitAddnSet)
{
    uint8_t expectedTlv [12] =
    /* The following assumes little-endian integers */
    /*T    L    T    L    V    */
    { 1,0, 8,0, 4,0, 4,0, 7,0,0,0};
    uint8_t actualTlv [12];


    // Remove the bin file to ensure RAM is init'd with default values
    remove(CFG_FILE_NAME);    

    cm->init(&c2);

    CHECK(GET_C2_CONFIG->cnt == 0);
    CHECK(GET_C2_CONFIG->owned == NULL);   

    char * commandWord[] = {(char *)"owned", (char *)"0", (char *)"=", (char *)"7"}; // set item 0, causing implicit add
    cm->handleCmd(4, commandWord);

    CHECK(GET_C2_CONFIG->cnt == 1);
    CHECK(GET_C2_CONFIG->owned[0] == 7);   

    char * commandWord2[] = {(char *)"save"};
    cm->handleCmd(1, commandWord2);

    if ((fp = fopen(CFG_FILE_NAME, "rb")) == NULL)
    {
        FAIL("Couldn't open file");
    }

    fread(actualTlv, sizeof(actualTlv), 1, fp);

    CHECK(memcmp(expectedTlv, actualTlv, sizeof(expectedTlv)) == 0);
    fclose(fp);    
}


// From default RAM start, do explicit add and check what's saved to TLV
TEST(owned, explicitAdd)
{
    uint8_t expectedTlv [12] =
    /* The following assumes little-endian integers */
    /*T    L    T    L    V    */
    { 1,0, 8,0, 4,0, 4,0, 0,0,0,0};
    uint8_t actualTlv [12];


    // Remove the bin file to ensure RAM is init'd with default values
    remove(CFG_FILE_NAME);    

    cm->init(&c2);

    CHECK(GET_C2_CONFIG->cnt == 0);
    CHECK(GET_C2_CONFIG->owned == NULL);

    char * commandWord[] = {(char *)"add", (char *)"owned"};
    cm->handleCmd(2, commandWord);

    CHECK(GET_C2_CONFIG->cnt == 1);
    CHECK(GET_C2_CONFIG->owned != NULL);

    char * commandWord2[] = {(char *)"save"};
    cm->handleCmd(1, commandWord2);

    if ((fp = fopen(CFG_FILE_NAME, "rb")) == NULL)
    {
        FAIL("Couldn't open file");
    }

    fread(actualTlv, sizeof(actualTlv), 1, fp);

    CHECK(memcmp(expectedTlv, actualTlv, sizeof(expectedTlv)) == 0);
    fclose(fp);    
}


// Verify what's saved to TLV, given a TLV file that's read on startup,
// and a delete operation.
TEST(owned, del)
{
    uint8_t tlv[12] =
    /* The following assumes little-endian integers */
    /*T    L    T    L    V    */
    { 1,0, 8,0, 4,0, 4,0, 5,0,0,0};
    uint8_t expectedTlv [4] =
    /* The following assumes little-endian integers */
    /*T    L  */
    { 1,0, 0,0};
    uint8_t savedTlv[4];


    /* Create config file to be loaded */
    if ((fp = fopen(CFG_FILE_NAME, "wb")) == NULL)
    {
        FAIL("Couldn't open file");
    }

    fwrite(tlv, sizeof(tlv), 1, fp);
    fclose(fp);

    cm->init(&c2);

    CHECK(GET_C2_CONFIG->cnt == 1);
    CHECK(GET_C2_CONFIG->owned[0] == 5);

    char * commandWord[] = {(char *)"del", (char *)"owned"};
    cm->handleCmd(2, commandWord);

    CHECK(GET_C2_CONFIG->cnt == 0);
    CHECK(GET_C2_CONFIG->owned == NULL);

    char * commandWord2[] = {(char *)"save"};
    cm->handleCmd(1, commandWord2);

    // See what CM made of the file it loaded
    if ((fp = fopen(CFG_FILE_NAME, "rb")) == NULL)
    {
        FAIL("Couldn't open file");
    }

    fread(savedTlv, sizeof(savedTlv), 1, fp);

    CHECK(memcmp(expectedTlv, savedTlv, sizeof(savedTlv)) == 0);
    fclose(fp);
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
const cm_simple_metadata s5_d = {{"name1", 1, sizeof(short int)}, NULL, NULL, NULL};
const cm_simple_descriptor s5(&s5_d, true);
const cm_aggregate_data ca5_d = {&s5, T3_ARRAY_SIZE, offsetof(struct m3, m1)};
const cm_contained_aggregate ca5(&ca5_d);

const cm_aggregate * const aggrList3[] = {&ca5};
const cm_composite_metadata c3_d = {{"c3", 1, sizeof(struct m3)}, aggrList3, sizeof(aggrList3)/sizeof(aggrList3[0])};
const cm_composite_descriptor c3(&c3_d, true);

#define GET_C3_CONFIG ((struct m3 *)config_manager::getInstance()->getConfig())

TEST_GROUP(containedArray)
{
    FILE *           fp;
    config_manager * cm;

    //Define data accessible to test group members here.
    void setup()
    {
        cm = config_manager::getInstance();

    }
    
    void teardown()
    {
        //clean up steps are executed after each TEST
    }
};


// Verify what's loaded into memory, given TLV file that's read on startup.
TEST(containedArray, load)
{
    uint8_t tlv[16] =
    /*T    L     T    L    V    T    L    V */
    { 1,0, 12,0, 1,0, 2,0, 7,0, 1,0, 2,0, 8,0};


    /* Create config file to be loaded */
    if ((fp = fopen(CFG_FILE_NAME, "wb")) == NULL)
    {
        FAIL("Couldn't open file");
    }

    fwrite(tlv, sizeof(tlv), 1, fp);
    fclose(fp);

    cm->init(&c3);

    CHECK(GET_C3_CONFIG->m1[0] == 7);
    CHECK(GET_C3_CONFIG->m1[1] == 8);
}


// Verify what's saved to TLV, given a TLV file that's read on startup that
// has more than the max number of a CONTAINED component.
//
TEST(containedArray, loadTooMany)
{
    uint8_t tlv[22] =
    /*T    L     T    L    V    T    L    V    T    L    V       */
    { 1,0, 18,0, 1,0, 2,0, 7,0, 1,0, 2,0, 8,0, 1,0, 2,0, 9,0};
    uint8_t expectedTlv[16] =
    /* The following assumes little-endian integers */
    /*T    L     T    L    V    T    L    V    */
    { 1,0, 12,0, 1,0, 2,0, 7,0, 1,0, 2,0, 8,0};
    uint8_t savedTlv[16];


    /* Create config file to be loaded */
    if ((fp = fopen(CFG_FILE_NAME, "wb")) == NULL)
    {
        FAIL("Couldn't open file");
    }

    fwrite(tlv, sizeof(tlv), 1, fp);
    fclose(fp);    

    cm->init(&c3);

    char * commandWord[] = {(char *)"save"};
    cm->handleCmd(1, commandWord);

    // See what CM made of the file it loaded
    if ((fp = fopen(CFG_FILE_NAME, "rb")) == NULL)
    {
        FAIL("Couldn't open file");
    }

    fread(savedTlv, sizeof(savedTlv), 1, fp);

    CHECK(memcmp(expectedTlv, savedTlv, sizeof(savedTlv)) == 0);
    fclose(fp);    
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
const cm_simple_metadata s6_d = {{"name1", 1, sizeof(short int)}, NULL, NULL, NULL};
const cm_simple_descriptor s6(&s6_d, true);
const cm_aggregate_data ca6_d = {&s6, T6_ARRAY_SIZE, offsetof(struct m6, m1)};
const cm_contained_aggregate ca6(&ca6_d);

const cm_simple_metadata s7_d = {{"name2", 2, sizeof(short int)}, NULL, NULL, NULL};
const cm_simple_descriptor s7(&s7_d, true);
const cm_aggregate_data ca7_d = {&s7, T7_ARRAY_SIZE, offsetof(struct m6, m2)};
const cm_contained_aggregate ca7(&ca7_d);

const cm_aggregate * const aggrList4[] = {&ca6, &ca7};
const cm_composite_metadata c4_d = {{"c4", 1, sizeof(struct m6)}, aggrList4, sizeof(aggrList4)/sizeof(aggrList4[0])};
const cm_composite_descriptor c4(&c4_d, true);

#define GET_C4_CONFIG ((struct m6 *)config_manager::getInstance()->getConfig())

TEST_GROUP(containedArrays)
{
    FILE *           fp;
    config_manager * cm;

    //Define data accessible to test group members here.
    void setup()
    {
        cm = config_manager::getInstance();

    }
    
    void teardown()
    {
        //clean up steps are executed after each TEST
    }
};

// Verify what's loaded into memory, given TLV file that's read on startup.
// Verify what's loaded into memory, given TLV file that's read on startup.
TEST(containedArrays, load)
{    
    uint8_t tlv[28] =
    /* The following assumes little-endian integers */
    /*T    L     T    L    V    T    L    V    T    L    V    T    L    V */
    { 1,0, 24,0, 1,0, 2,0, 4,0, 1,0, 2,0, 5,0, 2,0, 2,0, 6,0, 2,0, 2,0, 7,0};


    /* Create config file to be loaded */
    if ((fp = fopen(CFG_FILE_NAME, "wb")) == NULL)
    {
        FAIL("Couldn't open file");
    }

    fwrite(tlv, sizeof(tlv), 1, fp);
    fclose(fp);

    cm->init(&c4);

    CHECK(GET_C4_CONFIG->m1[0] == 4);
    CHECK(GET_C4_CONFIG->m1[1] == 5);
    CHECK(GET_C4_CONFIG->m2[0] == 6);
    CHECK(GET_C4_CONFIG->m2[1] == 7);
}

