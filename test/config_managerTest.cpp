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
const cm_simple_item_descriptor s3("count", 3, sizeof(int), NULL, NULL, NULL);
const cm_simple_item_descriptor s4("owned", 4, sizeof(int), NULL, NULL, NULL);
const cm_contained_aggregate ca3(&s3, 1, offsetof(struct m2, cnt));
const cm_owned_aggregate oa4(&s4, 10, offsetof(struct m2, owned), &ca3);
const cm_aggregate * const aggrList2[] = {&ca3, &oa4};
const cm_composite_item_descriptor c2("c2", 1, sizeof(struct m2), aggrList2, sizeof(aggrList2)/sizeof(aggrList2[0]));


// xxx todo: delete the file cfg.bin before starting test, else this test is not independent
TEST(initNoFile, config_manager)
{
    FILE * fp;
    config_manager cm(&c1);
    unsigned char expectedTlv [20] =
    /* T   L     T    L    V        T    L    V    */
    { 1,0, 16,0, 1,0, 4,0, 0,0,0,0, 2,0, 4,0, 0,0,0,0};
    unsigned char actualTlv [20];
    

    cm.init(NULL, NULL);

    char * commandWord[] = {"save"};
    cm.do_cmd(1, commandWord);

    if ((fp = fopen("cfg.bin", "rb")) == NULL)
    {
        FAIL("Couldn't open cfg.bin");
    }

    fread(actualTlv, sizeof(actualTlv), 1, fp);

    CHECK(memcmp(expectedTlv, actualTlv, sizeof(expectedTlv)) == 0);
    fclose(fp);    
}



