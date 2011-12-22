// Unit test using open-source unit test framework
// These tests use the cm_simple_descriptor's public interface
// to test it.  This includes redirecting stdout to a file,
// so that it can be read from the file and compared to the
// expected output.
// We also read/write the items themselves. 
//

#include "CppUTest/TestHarness.h"
#include "CppUTest/CommandLineTestRunner.h"
#include "config_manager.h"  // Unit under test
#include "config_manager_util.h"     // Extensions to unit under test (generic "set" functions)
#include <string.h> // strncmp

#include <string>
using namespace std;


// setdef function used in tests
void setint11(uint8_t *pItem, cm_item_len_t len)
{
    // Sanity check
    assert(len == sizeof(int));

    *((int *)pItem) = 11;
}    


int main(int argc, char** argv)
{
    return RUN_ALL_TESTS(argc, argv);
}


TEST_GROUP(cm_simple_descriptor)
{
    //Define data accessible to test group members here.
    void setup()
    {
        //initialization steps are executed before each TEST
    }
    
    void teardown()
    {
        //clean up steps are executed after each TEST
    }
};


TEST(cm_simple_descriptor, getLen)
{
    cm_simple_metadata d_d = {{"d01", 1 , 55, true}, NULL, NULL, NULL};

    cm_simple_descriptor d(&d_d);
    LONGS_EQUAL(55, d.getLen());
}


TEST(cm_simple_descriptor, print)
{
    string prefix = "";
    unsigned mem = 7;
    char outstring[64];
    cm_simple_metadata d_d = {{"d01", 1 , sizeof(mem), true}, NULL, NULL, NULL};

    cm_simple_descriptor d(&d_d);

    // Redirect STDOUT to a file, so the test can examine what UUT writes there
    if (freopen("testout.txt", "w", stdout) == NULL)
    {
        cout << "redirecting stdout failed" << endl;
    }
    d.print((uint8_t *)&mem, prefix);

    // I tried using /dev/stdout instead of /dev/console -- that didn't work (no output to console was produced)
    freopen("/dev/console", "w", stdout);

    // xxx Reading from a file and comparing the contents could be re-implemented as an assert method
    FILE * resf = fopen("testout.txt", "r");
    fgets(outstring, sizeof(outstring), resf);
    STRCMP_EQUAL("= 07000000\n", outstring);
}


TEST(cm_simple_descriptor, set)
{
    int mem = 0;
    cm_simple_metadata d_d = {{"d01", 1 , sizeof(mem), true}, cm_set_int, NULL, NULL};

    cm_simple_descriptor d(&d_d);
    
    d.set((uint8_t *)&mem, "4");
    LONGS_EQUAL(4, mem);
}


// Check setdef() calls the function installed in metadata
TEST(cm_simple_descriptor, setdefFunc)
{
    int mem = 8;
    cm_simple_metadata d_d = {{"d01", 1 , sizeof(mem), true}, NULL, setint11, NULL};

    cm_simple_descriptor d(&d_d);
    
    d.setDefault((uint8_t *)&mem);
    LONGS_EQUAL(11, mem);
}


// Check setdef() does nothing if there's no setdef function installed in metadata
TEST(cm_simple_descriptor, setdef)
{
    int mem = 8;
    cm_simple_metadata d_d = {{"d01", 1 , sizeof(mem), true}, NULL, NULL, NULL};

    cm_simple_descriptor d(&d_d);
    
    d.setDefault((uint8_t *)&mem);
    LONGS_EQUAL(8, mem);
}


