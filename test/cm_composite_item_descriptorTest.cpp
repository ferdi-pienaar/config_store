// Unit test using open-source unit test framework

#include "TestHarness.h"
#include "config_manager.h"  // Unit under test
#include "config_manager_util.h"     // Extensions to unit under test (generic "set" functions)

#include <string>


int main()
{
	TestResult tr;
	TestRegistry::runAllTests(tr);
	return 0;
}


// first test set, CONTAINED
// first test set data structure
struct m
{
    int m1;
    int m2;
};

// first test set metadata
const cm_simple_item_descriptor s1("name1", 1, sizeof(int), NULL, NULL, NULL);
const cm_simple_item_descriptor s2("name2", 2, sizeof(int), NULL, NULL, NULL);
const cm_contained_aggregate ca1(&s1, 1, offsetof(struct m, m1));
const cm_contained_aggregate ca2(&s2, 1, offsetof(struct m, m2));
const cm_aggregate * const aggrList1[] = {&ca1, &ca2};
const cm_composite_item_descriptor c1("c1", 1, sizeof(struct m), aggrList1, sizeof(aggrList1)/sizeof(aggrList1[0]));

// second test set, OWNED.
// second test set data structure
struct m2
{
    unsigned cnt;
    int *    owned;
};

// second test set metadata
const cm_simple_item_descriptor s3("count", 1, sizeof(int), NULL, NULL, NULL);
const cm_simple_item_descriptor s4("owned", 2, sizeof(int), NULL, NULL, NULL);
const cm_contained_aggregate ca3(&s3, 1, offsetof(struct m2, cnt));
const cm_owned_aggregate oa4(&s4, 10, offsetof(struct m2, owned), &ca3);
const cm_aggregate * const aggrList2[] = {&ca3, &oa4};
const cm_composite_item_descriptor c2("c2", 1, sizeof(struct m2), aggrList2, sizeof(aggrList2)/sizeof(aggrList2[0]));


TEST(getLen, cm_composite_item_descriptor)
{
    cm_composite_item_descriptor d("c1", 1, 55 , NULL, 0);

    CHECK(d.getLen() == 55);
}


TEST(printContained, cm_composite_item_descriptor)
{
    string prefix = "";
    char outstring[64];
    struct m mem = {3, 5}; // Test data
    

    // Redirect STDOUT to a file, so the test can examine what UUT writes there
    if (freopen("testout.txt", "w", stdout) == NULL)
    {
        cout << "redirecting stdout failed" << endl;
    }
    c1.print((unsigned char *)&mem, prefix);

    freopen("/dev/console", "w", stdout);

    FILE * resf = fopen("testout.txt", "r");
    fgets(outstring, sizeof(outstring), resf);
    CHECK(strncmp(outstring, "name1 = 03000000\n", sizeof(outstring)) == 0);
    fgets(outstring, sizeof(outstring), resf);
    CHECK(strncmp(outstring, "name2 = 05000000\n", sizeof(outstring)) == 0);
}


// Owned component in metadata, but not allocated
TEST(printOwnedNull, cm_composite_item_descriptor)
{
    string prefix = "";
    char outstring[64];
    struct m2 mem = {0, NULL}; // Test data

    
    // Redirect STDOUT to a file, so the test can examine what UUT writes there
    if (freopen("testout.txt", "w", stdout) == NULL)
    {
        cout << "redirecting stdout failed" << endl;
    }
    c2.print((unsigned char *)&mem, prefix);

    freopen("/dev/console", "w", stdout);

    FILE * resf = fopen("testout.txt", "r");
    fgets(outstring, sizeof(outstring), resf);
    CHECK(strncmp(outstring, "count = 00000000\n", sizeof(outstring)) == 0);
}


// Owned component in metadata, correctly allocated
TEST(printOwnedData, cm_composite_item_descriptor)
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
    c2.print((unsigned char *)&mem, prefix);

    freopen("/dev/console", "w", stdout);

    FILE * resf = fopen("testout.txt", "r");
    fgets(outstring, sizeof(outstring), resf);
    CHECK(strncmp(outstring, "count = 02000000\n", sizeof(outstring)) == 0);
    fgets(outstring, sizeof(outstring), resf);
    CHECK(strncmp(outstring, "owned 0 = 07000000\n", sizeof(outstring)) == 0);
    fgets(outstring, sizeof(outstring), resf);
    CHECK(strncmp(outstring, "owned 1 = 08000000\n", sizeof(outstring)) == 0);
}


TEST(addFirst, cm_composite_item_descriptor)
{
    string prefix = "";
    char outstring[64];
    struct m2 mem = {0, NULL}; // Test data
    char * commandWord = "owned";

    
    c2.handleAdd(1, &commandWord, (unsigned char *)&mem);

    // Counter is incremented and ptr to owned item is no longer NULL
    CHECK(mem.cnt == 1);
    CHECK(mem.owned != NULL);

    // Item initialized to "default default"
    CHECK(mem.owned[0] == 0);
}


TEST(delNull, cm_composite_item_descriptor)
{
    string prefix = "";
    char outstring[64];
    struct m2 mem = {0, NULL};
    char * commandWord[] = {"owned", "1"};

    
    c2.del(2, commandWord, (unsigned char *)&mem);

    // The fails since there's nothing to delete; verify count remains unchanged
    CHECK(mem.cnt == 0);
}


TEST(delEnd, cm_composite_item_descriptor)
{
    #define NUM_OWNED 2

    string prefix = "";
    char outstring[64];
    struct m2 mem;
    char * commandWord[] = {"owned", "1"};

    // We have to malloc, not use automatic variables, since the del operation calls free() for owned memory
    int * owned = (int *)malloc(NUM_OWNED * sizeof(int));
    owned[0] = 7;
    mem.cnt = NUM_OWNED;
    mem.owned = owned;
    
    c2.del(2, commandWord, (unsigned char *)&mem);

    // Counter is decremented
    CHECK(mem.cnt == NUM_OWNED - 1);

    // But the 0th item is unaffected by the deletion of the 1th item
    CHECK(mem.owned[0] == 7);
}


TEST(delFirst, cm_composite_item_descriptor)
{
    #define NUM_OWNED 2

    string prefix = "";
    char outstring[64];
    struct m2 mem;
    char * commandWord[] = {"owned", "0"};

    // We have to malloc, not use automatic variables, since the del operation calls free() for owned memory
    int * owned = (int *)malloc(NUM_OWNED * sizeof(int));
    owned[0] = 7;
    owned[1] = 8;
    mem.cnt = NUM_OWNED;
    mem.owned = owned;
    
    c2.del(2, commandWord, (unsigned char *)&mem);

    // Counter is decremented
    CHECK(mem.cnt == NUM_OWNED - 1);

    // And the 1th item has become the 0th item
    CHECK(mem.owned[0] == 8);
}


TEST(delOnly, cm_composite_item_descriptor)
{
    #define NUM_OWNED 1

    string prefix = "";
    char outstring[64];
    struct m2 mem;
    char * commandWord[] = {"owned", "0"};

    // We have to malloc, not use automatic variables, since the del operation calls free() for owned memory
    int * owned = (int *)malloc(NUM_OWNED * sizeof(int));
    mem.cnt = NUM_OWNED;
    mem.owned = owned;
    
    c2.del(2, commandWord, (unsigned char *)&mem);

    // Counter is decremented to 0
    CHECK(mem.cnt == 0);

    // xxx And the pointer to owned is set to NULL after owned memory freed??
    // Currently, we don't do this.
    // CHECK(mem.owned == NULL);
}




