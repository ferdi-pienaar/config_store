// Unit test using open-source unit test framework
// These tests the config manager as a whole, using the following interfaces:
// 1. Input: character strings passed to config_manager::handleCmd
// 2. Input/output: the binary file containing TLV data used by config_manager
//    for non-volatile storage
//
//

#include "TestHarness.h"
#include "config_manager.h"       // Unit under test
#include "config_manager_util.h"  // Extensions to unit under test (generic "set" functions)

#include <string>
using namespace std;


int main()
{
	TestResult tr;
	TestRegistry::runAllTests(tr);
	return 0;
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
void setdef_t1(uint8_t *pItem, cm_item_len len)
{
    // Sanity check
    assert(len == sizeof(int));

    *((int *)pItem) = 7;
}


// test set 1 metadata
const cm_basic_item_descriptor s1("name1", 1, sizeof(int), NULL, NULL, NULL);
const cm_basic_item_descriptor s2("name2", 2, sizeof(int), NULL, setdef_t1, NULL);
const cm_contained_aggregate ca1(&s1, 1, offsetof(struct m, m1));
const cm_contained_aggregate ca2(&s2, 1, offsetof(struct m, m2));
const cm_aggregate * const aggrList1[] = {&ca1, &ca2};
const cm_composite_item_descriptor c1("c1", 1, sizeof(struct m), aggrList1, sizeof(aggrList1)/sizeof(aggrList1[0]));

#define GET_C1_CONFIG ((struct m *)config_manager::getInstance()->getConfig())


////////////////////////////////////////////////////////////////////////////////
// test set 2, OWNED.
// test set 2 data structure
struct m2
{
    unsigned cnt;
    int *    owned;
};

// test set 2 metadata
const cm_cntr_item_descriptor s3("count", 3, sizeof(int), NULL);
const cm_basic_item_descriptor s4("owned", 4, sizeof(int), cm_set_int, NULL, NULL);
const cm_contained_aggregate ca3(&s3, 1, offsetof(struct m2, cnt));
const cm_owned_aggregate oa4(&s4, 2, offsetof(struct m2, owned), &ca3);
const cm_aggregate * const aggrList2[] = {&ca3, &oa4};
const cm_composite_item_descriptor c2("c2", 1, sizeof(struct m2), aggrList2, sizeof(aggrList2)/sizeof(aggrList2[0]));

#define GET_C2_CONFIG ((struct m2 *)config_manager::getInstance()->getConfig())


////////////////////////////////////////////////////////////////////////////////
// test set 3, CONTAINED
// test set 3 data structure
#define T3_ARRAY_SIZE 2
struct m3
{
    short int m1[T3_ARRAY_SIZE];
};

// test set 3 metadata
const cm_basic_item_descriptor s5("name1", 1, sizeof(short int), NULL, NULL, NULL);
const cm_contained_aggregate ca5(&s5, T3_ARRAY_SIZE, offsetof(struct m3, m1));
const cm_aggregate * const aggrList3[] = {&ca5};
const cm_composite_item_descriptor c3("c3", 1, sizeof(struct m3), aggrList3, sizeof(aggrList3)/sizeof(aggrList3[0]));

#define GET_C3_CONFIG ((struct m3 *)config_manager::getInstance()->getConfig())


// Verify data saved to TLV, with default data in RAM as input to the test.
TEST(saveContained, config_manager)
{
    FILE * fp;
    config_manager * cm = config_manager::getInstance();
    uint8_t expectedTlv [20] =
    /* The following assumes little-endian integers */
    /*T    L     T    L    V        T    L    V    */
    { 1,0, 16,0, 1,0, 4,0, 0,0,0,0, 2,0, 4,0, 7,0,0,0};
    uint8_t actualTlv [20];

    // Remove the bin file to ensure RAM is init'd with default values
    remove(CFG_FILE_NAME);

    cm->init(&c1);

    char * commandWord[] = {"save"};
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
TEST(loadContained, config_manager)
{
    FILE * fp;
    config_manager * cm = config_manager::getInstance();
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
TEST(loadChangedSimpleLen, config_manager)
{
    FILE * fp;
    config_manager * cm = config_manager::getInstance();
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


// Verify what's loaded into memory, given TLV file that's read on startup.
TEST(loadContainedArray, config_manager)
{
    FILE * fp;
    config_manager * cm = config_manager::getInstance();
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


// Verify what's loaded into memory, given TLV file that's read on startup.
TEST(loadOwned, config_manager)
{
    FILE * fp;
    config_manager * cm = config_manager::getInstance();
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


// Verify what's loaded into memory, given a TLV file that's read on startup that contains
// an unknown Type value.
// Unknown type in file: the descriptor has no T=9, so it's ignored by cfg_man when found in file,
// but the item following it is loaded.
TEST(loadUnknownContained, config_manager)
{
    FILE * fp;
    config_manager * cm = config_manager::getInstance();
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

    char * commandWord[] = {"save"};
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
TEST(loadMissingContained, config_manager)
{
    FILE * fp;
    config_manager * cm = config_manager::getInstance();
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

    char * commandWord[] = {"save"};
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


// Verify what's saved to TLV, given a TLV file that's read on startup that
// has more than the max number of an OWNED component.
//
TEST(loadTooManyOwned, config_manager)
{
    FILE * fp;
    config_manager * cm = config_manager::getInstance();
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

    char * commandWord[] = {"save"};
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


// Verify what's saved to TLV, given a TLV file that's read on startup that
// has more than the max number of a CONTAINED component.
//
TEST(loadTooManyContained, config_manager)
{
    FILE * fp;
    config_manager * cm = config_manager::getInstance();
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

    char * commandWord[] = {"save"};
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
TEST(saveOwned, config_manager)
{
    FILE * fp;
    config_manager * cm = config_manager::getInstance();
    uint8_t expectedTlv [4] =
    /* The following assumes little-endian integers */
    /*T    L     T    L    V        T    L    V    */
    { 1,0, 0,0};
    uint8_t actualTlv [4];


    // Remove the bin file to ensure RAM is init'd with default values
    remove(CFG_FILE_NAME);    

    cm->init(&c2);

    char * commandWord[] = {"save"};
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
TEST(implicitAdd, config_manager)
{
    FILE * fp;
    config_manager * cm = config_manager::getInstance();
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

    char * commandWord[] = {"owned", "0"}; // reference owned item 0, causing implicit add
    cm->handleCmd(2, commandWord);

    CHECK(GET_C2_CONFIG->cnt == 1);

    char * commandWord2[] = {"save"};
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
TEST(implicitAddnSet, config_manager)
{
    FILE * fp;
    config_manager * cm = config_manager::getInstance();
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

    char * commandWord[] = {"owned", "0", "=", "7"}; // set item 0, causing implicit add
    cm->handleCmd(4, commandWord);

    CHECK(GET_C2_CONFIG->cnt == 1);
    CHECK(GET_C2_CONFIG->owned[0] == 7);   

    char * commandWord2[] = {"save"};
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
TEST(explicitAdd, config_manager)
{
    FILE * fp;
    config_manager * cm = config_manager::getInstance();
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

    char * commandWord[] = {"add", "owned"};
    cm->handleCmd(2, commandWord);

    CHECK(GET_C2_CONFIG->cnt == 1);
    CHECK(GET_C2_CONFIG->owned != NULL);

    char * commandWord2[] = {"save"};
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
TEST(del, config_manager)
{
    FILE * fp;
    config_manager * cm = config_manager::getInstance();
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

    char * commandWord[] = {"del", "owned"};
    cm->handleCmd(2, commandWord);

    CHECK(GET_C2_CONFIG->cnt == 0);
    CHECK(GET_C2_CONFIG->owned == NULL);

    char * commandWord2[] = {"save"};
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

