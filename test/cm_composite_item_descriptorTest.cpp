// Unit test using open-source unit test framework
// These tests use the cm_composite_descriptor's public interface
// to test it.  This includes redirecting to a file output that it sends to
// stdout, so that it can be read from the file and compared to the
// expected output.
// We also read/write the items themselves.
//


#include "TestHarness.h"
#include "config_manager.h"       // Unit under test
#include "config_manager_util.h"  // Extensions to unit under test (generic "set" functions)

#include <string>
#include <stdlib.h> // malloc
#include <string.h> // strncmp, etc


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

// test set 1 metadata
const cm_simple_metadata s1_d = {{"name1", 1, sizeof(int)}, NULL, NULL, NULL};
const cm_simple_descriptor s1(&s1_d, false);
const cm_aggregate_data ca1_d = {&s1, 1, offsetof(struct m, m1)};
const cm_contained_aggregate ca1(&ca1_d);

const cm_simple_metadata s2_d = {{"name2", 2, sizeof(int)}, NULL, NULL, NULL};
const cm_simple_descriptor s2(&s2_d, false);
const cm_aggregate_data ca2_d = {&s2, 1, offsetof(struct m, m2)};
const cm_contained_aggregate ca2(&ca2_d);

const cm_aggregate * const aggrList1[] = {&ca1, &ca2};
const cm_composite_metadata c1_d = {{"c1", 1, sizeof(struct m)}, aggrList1, sizeof(aggrList1)/sizeof(aggrList1[0])};
const cm_composite_descriptor c1(&c1_d, false);


////////////////////////////////////////////////////////////////////////////////
// test set 2, OWNED with a visible (printing) counter
// test set 2 data structure
#define MAX_NUMBER_OWNED_SET2 10
struct m2
{
    unsigned cnt;
    int *    owned;
};

// test set 2 metadata
const cm_simple_metadata s3_d = {{"count", 1, sizeof(int)}, NULL, NULL, NULL};
const cm_simple_descriptor s3(&s3_d, false);
const cm_aggregate_data ca3_d = {&s3, 1, offsetof(struct m2, cnt)};
const cm_contained_aggregate ca3(&ca3_d);

const cm_simple_metadata s4_d = {{"owned", 2, sizeof(int)}, cm_set_int, NULL, NULL};
const cm_simple_descriptor s4(&s4_d, false);
const cm_aggregate_data oa4_d = {&s4, MAX_NUMBER_OWNED_SET2, offsetof(struct m2, owned)};
const cm_owned_aggregate oa4(&oa4_d, &ca3);

const cm_aggregate * const aggrList2[] = {&ca3, &oa4};
const cm_composite_metadata c2_d = {{"c2", 1, sizeof(struct m2)}, aggrList2, sizeof(aggrList2)/sizeof(aggrList2[0])};
const cm_composite_descriptor c2(&c2_d, false);


////////////////////////////////////////////////////////////////////////////////
// xxx test set 3, OWNED with a non-printing counter.
// The counter is still available to the application, but the user doesn't
// want to see it.


////////////////////////////////////////////////////////////////////////////////
// test set 4, OWNED with no counter => cntr for cm_owned_aggregate() is NULL
// (When the max number of owned items is 1, no counter is needed, since
// the pointer to the owned data is either NULL or not.)
// test set 4 data structure
#define MAX_NUMBER_OWNED_SET4 1
struct m4
{
    int * owned;
};

// test set 4 metadata
const cm_simple_metadata s7_d = {{"owned", 1, sizeof(int)}, NULL, NULL, NULL};
const cm_simple_descriptor s7(&s7_d, false);
const cm_aggregate_data oa7_d = {&s7, MAX_NUMBER_OWNED_SET4, offsetof(struct m4, owned)};
const cm_owned_aggregate oa7(&oa7_d, NULL); // NULL => no counter

const cm_aggregate * const aggrList4[] = {&oa7};
const cm_composite_metadata c4_d = {{"c4", 1, sizeof(struct m4)}, aggrList4, sizeof(aggrList4)/sizeof(aggrList4[0])};
const cm_composite_descriptor c4(&c4_d, false);


TEST(getLen, cm_composite_descriptor)
{
    cm_composite_metadata d_d  = {{"c1", 1, 55}, NULL, 0};
    cm_composite_descriptor d(&d_d, false);

    CHECK(d.getLen() == 55);
}


TEST(printContained, cm_composite_descriptor)
{
    string prefix = "";
    char outstring[64];
    struct m mem = {3, 5}; // Test data
    

    // Redirect STDOUT to a file, so the test can examine what UUT writes there
    if (freopen("testout.txt", "w", stdout) == NULL)
    {
        cout << "redirecting stdout failed" << endl;
    }
    c1.print((uint8_t *)&mem, prefix);

    freopen("/dev/console", "w", stdout);

    FILE * resf = fopen("testout.txt", "r");
    fgets(outstring, sizeof(outstring), resf);
    CHECK(strncmp(outstring, "name1 = 03000000\n", sizeof(outstring)) == 0);
    fgets(outstring, sizeof(outstring), resf);
    CHECK(strncmp(outstring, "name2 = 05000000\n", sizeof(outstring)) == 0);
}


// Owned component in metadata, but not allocated
TEST(printOwnedNull, cm_composite_descriptor)
{
    string prefix = "";
    char outstring[64];
    struct m2 mem = {0, NULL}; // Test data

    
    // Redirect STDOUT to a file, so the test can examine what UUT writes there
    if (freopen("testout.txt", "w", stdout) == NULL)
    {
        cout << "redirecting stdout failed" << endl;
    }
    c2.print((uint8_t *)&mem, prefix);

    freopen("/dev/console", "w", stdout);

    FILE * resf = fopen("testout.txt", "r");
    fgets(outstring, sizeof(outstring), resf);
    CHECK(strncmp(outstring, "count = 00000000\n", sizeof(outstring)) == 0);
}


// Owned component in metadata, correctly allocated
TEST(printOwnedData, cm_composite_descriptor)
{
    #define NUM_OWNED 2

    string prefix = "";
    char outstring[64];
    // Test items
    int owned[NUM_OWNED] = {7,8};
    struct m2 mem = {NUM_OWNED, owned};

    
    // Redirect STDOUT to a file, so the test can examine what UUT writes there
    if (freopen("testout.txt", "w", stdout) == NULL)
    {
        cout << "redirecting stdout failed" << endl;
    }
    c2.print((uint8_t *)&mem, prefix);

    freopen("/dev/console", "w", stdout);

    FILE * resf = fopen("testout.txt", "r");
    fgets(outstring, sizeof(outstring), resf);
    CHECK(strncmp(outstring, "count = 02000000\n", sizeof(outstring)) == 0);
    fgets(outstring, sizeof(outstring), resf);
    CHECK(strncmp(outstring, "owned 0 = 07000000\n", sizeof(outstring)) == 0);
    fgets(outstring, sizeof(outstring), resf);
    CHECK(strncmp(outstring, "owned 1 = 08000000\n", sizeof(outstring)) == 0);
}


TEST(addFirst, cm_composite_descriptor)
{
    struct m2 mem = {0, NULL}; // Test data
    char * commandWord = (char *)"owned";

    
    c2.handleAdd(1, &commandWord, (uint8_t *)&mem);

    // Counter is incremented and ptr to owned item is no longer NULL
    CHECK(mem.cnt == 1);
    CHECK(mem.owned != NULL);

    // Item initialized to "default default"
    CHECK(mem.owned[0] == 0);
}


TEST(addAnother, cm_composite_descriptor)
{
    struct m2 mem = {0, NULL}; // Test data
    char * commandWord = (char *)"owned";

    
    c2.handleAdd(1, &commandWord, (uint8_t *)&mem);
    c2.handleAdd(1, &commandWord, (uint8_t *)&mem);

    // Counter is incremented and ptr to owned item is no longer NULL
    CHECK(mem.cnt == 2);
    CHECK(mem.owned != NULL);

    // Items initialized to "default default"
    CHECK(mem.owned[0] == 0);
    CHECK(mem.owned[1] == 0);
}


TEST(delNull, cm_composite_descriptor)
{
    struct m2 mem = {0, NULL};
    char * commandWord[] = {(char *)"owned", (char *)"1"};

    
    c2.handleDel(2, commandWord, (uint8_t *)&mem);

    // The fails since there's nothing to delete; verify count remains unchanged
    CHECK(mem.cnt == 0);
}


// Delete the 2nd of two owned items; the first is unchanged
TEST(delEnd, cm_composite_descriptor)
{
    #undef NUM_OWNED
    #define NUM_OWNED 2

    struct m2 mem;
    char * commandWord[] = {(char *)"owned", (char *)"1"};

    // We have to malloc, not use automatic variables, since the del operation calls free() for owned memory
    int * owned = (int *)malloc(NUM_OWNED * sizeof(int));
    owned[0] = 7;
    mem.cnt = NUM_OWNED;
    mem.owned = owned;
    
    c2.handleDel(2, commandWord, (uint8_t *)&mem);

    // Counter is decremented
    CHECK(mem.cnt == NUM_OWNED - 1);

    // But the 0th item is unaffected by the deletion of the 1th item
    CHECK(mem.owned[0] == 7);
}


// Delete the 1st of two owned items; the 2nd moves down
TEST(delFirst, cm_composite_descriptor)
{
    #undef NUM_OWNED
    #define NUM_OWNED 2

    struct m2 mem;
    char * commandWord[] = {(char *)"owned", (char *)"0"};

    // We have to malloc, not use automatic variables, since the del operation calls free() for owned memory
    int * owned = (int *)malloc(NUM_OWNED * sizeof(int));
    owned[0] = 7;
    owned[1] = 8;
    mem.cnt = NUM_OWNED;
    mem.owned = owned;
    
    c2.handleDel(2, commandWord, (uint8_t *)&mem);

    // Counter is decremented
    CHECK(mem.cnt == NUM_OWNED - 1);

    // And the 1th item has become the 0th item
    CHECK(mem.owned[0] == 8);
}


TEST(delSingle, cm_composite_descriptor)
{
    #undef NUM_OWNED
    #define NUM_OWNED 1

    struct m2 mem;
    char * commandWord[] = {(char *)"owned", (char *)"0"};

    // We have to malloc, not use automatic variables, since the del operation calls free() for owned memory
    int * owned = (int *)malloc(NUM_OWNED * sizeof(int));
    mem.cnt = NUM_OWNED;
    mem.owned = owned;
    
    c2.handleDel(2, commandWord, (uint8_t *)&mem);

    // Counter is decremented to 0
    CHECK(mem.cnt == 0);

    // And the pointer to owned is set to NULL after owned memory freed
    CHECK(mem.owned == NULL);
}


// Add the sole, uncounted OWNed item
TEST(addOnly, cm_composite_descriptor)
{
    struct m4 mem = {NULL};
    char * commandWord = (char *)"owned";
    
    c4.handleAdd(1, &commandWord, (uint8_t *)&mem);

    // Pointer updated
    CHECK(mem.owned != NULL);

    // Item initialized to "default default"
    CHECK(*(mem.owned) == 0);
}


// Delete the sole, uncounted OWNed item
TEST(delOnly, cm_composite_descriptor)
{
    struct m4 mem;
    char * commandWord = (char *)"owned";
    
    // We have to malloc, not use automatic variables, since the del operation calls free() for owned memory
    int * owned = (int *)malloc(NUM_OWNED * sizeof(int));
    
    mem.owned = owned;

    c4.handleDel(1, &commandWord, (uint8_t *)&mem);

    // And the pointer to owned is set to NULL after owned memory freed
    CHECK(mem.owned == NULL);
}


// Allocate memory as side-effect of set command
TEST(implicitAdd, cm_composite_descriptor)
{
    struct m2 mem = {0, NULL}; // Test data
    char * commandWord[] = {(char *)"owned", (char *)"0", (char *)"=", (char *)"42"};
    cm_context ctxt;

    
    c2.handleCmd(4, commandWord, (uint8_t *)&mem, ctxt);

    // Counter is incremented and ptr to owned item is no longer NULL
    CHECK(mem.cnt == 1);
    CHECK(mem.owned != NULL);
    CHECK(mem.owned[0] == 42);
}
 

// Do not (permanently) allocate memory as side-effect of executing invalid command
TEST(implicitAddFail, cm_composite_descriptor)
{
    struct m2 mem = {0, NULL}; // Test data
    char * commandWord[] = {(char *)"owned", (char *)"0", (char *)"blabla"};
    cm_context ctxt;

    
    c2.handleCmd(3, commandWord, (uint8_t *)&mem, ctxt);

    // Counter is not incremented and ptr to owned item is NULL
    CHECK(mem.cnt == 0);
    CHECK(mem.owned == NULL);
}


