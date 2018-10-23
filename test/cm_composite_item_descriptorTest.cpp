// Unit test using open-source unit test framework
// These tests use the cm_composite_descriptor's public interface
// to test it, and a spy to verify what the CUT prints to its console.
// We also read/write the items themselves.
//

#include "gtest/gtest.h"
#include "config_manager.h"       // Unit under test
#include "config_manager_util.h"  // Extensions to unit under test (generic "set" functions)
#include "config_manager_printf_spy.h"

#include <string>
#include <stdlib.h> // malloc
#include <string.h> // strncmp, etc


using namespace std;

namespace {

////////////////////////////////////////////////////////////////////////////////
// xxx TBD: Write this test set, OWNED with a non-printing counter.
// The counter is still available to the application, but the user doesn't
// want to see it.

////////////////////////////////////////////////////////////////////////////////
// xxx TBD: Write this test set, OWNED with a counter not adjacent to
// the owned element, but AFTER it.


TEST(cm_composite_descriptor, getLen)
{
    cm_composite_metadata d_d  = {{"c1", 1, 55, true}, NULL, 0};
    cm_composite_descriptor d(&d_d);

    EXPECT_EQ(55, d.getLen());
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
const cm_simple_metadata s1_d = {{"name1", 1, sizeof(int), true}, NULL, NULL, NULL};
const cm_simple_descriptor s1(&s1_d);
const cm_aggregate_data ca1_d = {&s1, 1, offsetof(struct m, m1)};
const cm_contained_aggregate ca1(&ca1_d);

const cm_simple_metadata s2_d = {{"name2", 2, sizeof(int), true}, NULL, NULL, NULL};
const cm_simple_descriptor s2(&s2_d);
const cm_aggregate_data ca2_d = {&s2, 1, offsetof(struct m, m2)};
const cm_contained_aggregate ca2(&ca2_d);

const cm_aggregate * const aggrList1[] = {&ca1, &ca2};
const cm_composite_metadata c1_d = {{"c1", 1, sizeof(struct m), true}, aggrList1, sizeof(aggrList1)/sizeof(aggrList1[0])};
const cm_composite_descriptor c1(&c1_d);

class CompositeContained : public testing::Test 
{
protected:
    struct m mem;
    
    virtual void SetUp()
    {
        memset(&mem, 0, sizeof(mem));
        cm_printf_spy_init();
    }
    
    virtual void TearDown()
    {
        //clean up steps are executed after each TEST
    }
};


TEST_F(CompositeContained, print)
{
    string prefix = "";
    const char * expected =
        "name1 = 03000000\n"
        "name2 = 05000000\n";

    mem.m1 = 3;
    mem.m2 = 5;

    c1.print((uint8_t *)&mem, prefix, false);
    EXPECT_STREQ(expected, cm_printf_spy_get());
}


static unsigned setdef_spy_calls = 0;

// test set 2 user-defined functions: a spy
void setdef_spy(uint8_t *pItem, cm_item_len_t len)
{
    (void)pItem;
    (void)len;

    setdef_spy_calls++;
}

////////////////////////////////////////////////////////////////////////////////
// test set 2, OWNED with a visible (printing) counter
// test set 2 data structure
static const unsigned MAX_NUMBER_OWNED_SET2 = 10;
struct m2
{
    unsigned cnt;
    int *    owned;
};

// test set 2 metadata
const cm_simple_metadata s3_d = {{"count", 1, sizeof(int), true}, NULL, NULL, NULL};
const cm_simple_descriptor s3(&s3_d);
const cm_aggregate_data ca3_d = {&s3, 1, offsetof(struct m2, cnt)};
const cm_contained_aggregate ca3(&ca3_d);

const cm_simple_metadata s4_d = {{"owned", 2, sizeof(int), true}, cm_set_int, setdef_spy, NULL};
const cm_simple_descriptor s4(&s4_d);
const cm_aggregate_data oa4_d = {&s4, MAX_NUMBER_OWNED_SET2, offsetof(struct m2, owned)};
const cm_owned_aggregate oa4(&oa4_d, &ca3);

const cm_aggregate * const aggrList2[] = {&ca3, &oa4};
const cm_composite_metadata c2_d = {{"c2", 1, sizeof(struct m2), true}, aggrList2, sizeof(aggrList2)/sizeof(aggrList2[0])};
const cm_composite_descriptor c2(&c2_d);

class CompositeOwned : public testing::Test 
{
protected:
    struct m2 mem;
    
    virtual void SetUp()
    {
        memset(&mem, 0, sizeof(mem));
        setdef_spy_calls = 0;
        cm_printf_spy_init();
    }
    
    virtual void TearDown()
    {
        //clean up steps are executed after each TEST
    }
};


// Owned component in metadata, but not allocated
TEST_F(CompositeOwned, printNull)
{
    string prefix = "";
    const char * expected =
        "count = 00000000\n";

    c2.print((uint8_t *)&mem, prefix, false);
    EXPECT_STREQ(expected, cm_printf_spy_get());
}


// Owned component in metadata, correctly allocated
TEST_F(CompositeOwned, printData)
{
    const unsigned NUM_OWNED = 2;

    string prefix = "";
    const char * expected =
        "count = 02000000\n"
        "owned 0 = 07000000\n"
        "owned 1 = 08000000\n";
    int owned[NUM_OWNED] = {7, 8};
    mem.cnt = NUM_OWNED;
    mem.owned = owned;
    
    c2.print((uint8_t *)&mem, prefix, false);
    EXPECT_STREQ(expected, cm_printf_spy_get());
}


TEST_F(CompositeOwned, addFirst)
{
    char * commandWord[] = {(char *)"add", (char *)"owned"};
    command_stack cmd(2, commandWord);

    c2.handleCmd(&cmd, (uint8_t *)&mem);

    // Counter is incremented and ptr to owned item is no longer NULL
    EXPECT_EQ(1, mem.cnt);
    EXPECT_TRUE(mem.owned != NULL);

    // Item initialized to "default default"
    EXPECT_EQ(0, mem.owned[0]);
}


TEST_F(CompositeOwned, addAnother)
{
    char * commandWord[] = {(char *)"add", (char *)"owned"};
    command_stack cmd1(2, commandWord);
    command_stack cmd2(2, commandWord);

    c2.handleCmd(&cmd1, (uint8_t *)&mem);
    c2.handleCmd(&cmd2, (uint8_t *)&mem);

    // Counter is incremented and ptr to owned item is no longer NULL
    EXPECT_EQ(2, mem.cnt);
    EXPECT_TRUE(mem.owned != NULL);

    // Items initialized to "default default"
    EXPECT_EQ(0, mem.owned[0]);
    EXPECT_EQ(0, mem.owned[1]);
}


TEST_F(CompositeOwned, delNull)
{
    char * commandWord[] = {(char *)"del", (char *)"owned", (char *)"1"};
    command_stack cmd(3, commandWord);

    c2.handleCmd(&cmd, (uint8_t *)&mem);

    // The command does nothing since there's nothing to delete; verify count remains unchanged
    EXPECT_EQ(0, mem.cnt);
}


// Delete the 2nd of two owned items; the first is unchanged
TEST_F(CompositeOwned, delEnd)
{
    char * commandWord[] = {(char *)"del", (char *)"owned", (char *)"1"};
    command_stack cmd(3, commandWord);
    const unsigned NUM_OWNED = 2;

    // We have to malloc, not use automatic variables, since the del operation calls free() for owned memory
    mem.owned = (int *)malloc(NUM_OWNED * sizeof(int));
    mem.owned[0] = 7;
    mem.cnt = NUM_OWNED;

    c2.handleCmd(&cmd, (uint8_t *)&mem);

    // Counter is decremented
    EXPECT_EQ(NUM_OWNED - 1, mem.cnt);
    // But the 0th item is unaffected by the deletion of the 1th item
    EXPECT_EQ(7, mem.owned[0]);
}


// Delete the 1st of two owned items; the 2nd moves down
TEST_F(CompositeOwned, delFirst)
{
    char * commandWord[] = {(char *)"del", (char *)"owned", (char *)"0"};
    command_stack cmd(3, commandWord);
    const unsigned NUM_OWNED = 2;

    // We have to malloc, not use automatic variables, since the del operation calls free() for owned memory
    mem.owned = (int *)malloc(NUM_OWNED * sizeof(int));
    mem.owned[0] = 7;
    mem.owned[1] = 8;
    mem.cnt = NUM_OWNED;

    c2.handleCmd(&cmd, (uint8_t *)&mem);

    // Counter is decremented
    EXPECT_EQ(NUM_OWNED - 1, mem.cnt);

    // And the 1th item has become the 0th item
    EXPECT_EQ(8, mem.owned[0]);
}


TEST_F(CompositeOwned, delSingle)
{
    char * commandWord[] = {(char *)"del", (char *)"owned", (char *)"0"};
    command_stack cmd(3, commandWord);
    const unsigned NUM_OWNED = 1;

    // We have to malloc, not use automatic variables, since the del operation calls free() for owned memory
    mem.owned = (int *)malloc(NUM_OWNED * sizeof(int));
    mem.cnt = NUM_OWNED;

    c2.handleCmd(&cmd, (uint8_t *)&mem);

    // Counter is decremented to 0
    EXPECT_EQ(0, mem.cnt);

    // And the pointer to owned is set to NULL after owned memory freed
    EXPECT_EQ(NULL, mem.owned);
}


// Allocate memory as side-effect of set command
TEST_F(CompositeOwned, implicitAdd)
{
    char * commandWord[] = {(char *)"owned", (char *)"0", (char *)"=", (char *)"42"};
    command_stack cmd(4, commandWord);
    
    c2.handleCmd(&cmd, (uint8_t *)&mem);

    // Counter is incremented and ptr to owned item is no longer NULL
    EXPECT_EQ(1, mem.cnt);
    EXPECT_TRUE(mem.owned != NULL);
    EXPECT_EQ(42, mem.owned[0]);
}
 

// Do not (permanently) allocate memory as side-effect of executing invalid command
TEST_F(CompositeOwned, implicitAddFail)
{
    char * commandWord[] = {(char *)"owned", (char *)"0", (char *)"blabla"};
    command_stack cmd(3, commandWord);
    
    c2.handleCmd(&cmd, (uint8_t *)&mem);

    // Counter is not incremented and ptr to owned item is NULL
    EXPECT_EQ(0, mem.cnt);
    EXPECT_EQ(NULL, mem.owned);
}


// Setting to default frees items
TEST_F(CompositeOwned, setdef)
{
    const unsigned NUM_OWNED = 2;

    // We have to malloc, not use automatic variables, since setdef operation results in free() for owned memory
    mem.owned = (int *)malloc(NUM_OWNED * sizeof(int));
    mem.owned[0] = 7;
    mem.cnt = NUM_OWNED;
    
    c2.setDefault((uint8_t *)&mem);

    EXPECT_EQ(0, mem.cnt);
    EXPECT_EQ(NULL, mem.owned);
    EXPECT_EQ(NUM_OWNED, setdef_spy_calls); // verify setdef of components is called
}


////////////////////////////////////////////////////////////////////////////////
// test set 4, OWNED with no counter => cntr for cm_owned_aggregate() is NULL
// (When the max number of owned items is 1, no counter is needed, since
// the pointer to the owned data is either NULL or not.)
// test set 4 data structure
static const unsigned MAX_NUMBER_OWNED_SET4 = 1;
struct m4
{
    int * owned;
};

// test set 4 metadata
const cm_simple_metadata s7_d = {{"owned", 1, sizeof(int), true}, NULL, NULL, NULL};
const cm_simple_descriptor s7(&s7_d);
const cm_aggregate_data oa7_d = {&s7, MAX_NUMBER_OWNED_SET4, offsetof(struct m4, owned)};
const cm_owned_aggregate oa7(&oa7_d, NULL); // NULL => no counter

const cm_aggregate * const aggrList4[] = {&oa7};
const cm_composite_metadata c4_d = {{"c4", 1, sizeof(struct m4), true}, aggrList4, sizeof(aggrList4)/sizeof(aggrList4[0])};
const cm_composite_descriptor c4(&c4_d);

class OwnedWithoutCounter : public testing::Test 
{
protected:
    struct m4 mem;
    
    virtual void SetUp()
    {
        memset(&mem, 0, sizeof(mem));
    }
    
    virtual void TearDown()
    {
        //clean up steps are executed after each TEST
    }
};


// Add the sole, uncounted OWNed item
TEST_F(OwnedWithoutCounter, addOnly)
{
    char * commandWord[] = {(char *)"add", (char *)"owned"};
    command_stack cmd(2, commandWord);

    c4.handleCmd(&cmd, (uint8_t *)&mem);

    // Pointer updated
    EXPECT_TRUE(mem.owned != NULL);

    // Item initialized to "default default"
    EXPECT_EQ(0, *(mem.owned));
}


// Delete the sole, uncounted OWNed item
TEST_F(OwnedWithoutCounter, delOnly)
{
    char * commandWord[] = {(char *)"del", (char *)"owned"};
    command_stack cmd(2, commandWord);
    
    // We have to malloc, not use automatic variables, since the del operation calls free() for owned memory
    mem.owned = (int *)malloc(MAX_NUMBER_OWNED_SET4 * sizeof(int));

    c4.handleCmd(&cmd, (uint8_t *)&mem);

    // And the pointer to owned is set to NULL after owned memory freed
    EXPECT_EQ(NULL, mem.owned);
}
} // namespace
