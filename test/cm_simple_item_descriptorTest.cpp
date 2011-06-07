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
    cm_simple_metadata d_d = {{"d01", 1 , 55}, NULL, NULL, NULL};

    cm_simple_descriptor d(&d_d, false);
    CHECK(d.getLen() == 55);
}


TEST(cm_simple_descriptor, print)
{
    string prefix = "";
    unsigned mem = 7;
    char outstring[64];
    cm_simple_metadata d_d = {{"d01", 1 , sizeof(mem)}, NULL, NULL, NULL};

    cm_simple_descriptor d(&d_d, false);

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
    CHECK(strncmp(outstring, "= 07000000\n", sizeof(outstring)) == 0);
}


TEST(cm_simple_descriptor, set)
{
    int mem = 0;
    cm_simple_metadata d_d = {{"d01", 1 , sizeof(mem)}, cm_set_int, NULL, NULL};

    cm_simple_descriptor d(&d_d, false);
    
    d.set((uint8_t *)&mem, "4");
    CHECK(mem == 4);    
}


// Check setdef() calls the function installed in metadata
TEST(cm_simple_descriptor, setdefFunc)
{
    int mem = 8;
    cm_simple_metadata d_d = {{"d01", 1 , sizeof(mem)}, NULL, setint11, NULL};

    cm_simple_descriptor d(&d_d, false);
    
    d.setdef((uint8_t *)&mem);
    CHECK(mem == 11);    
}


// Check setdef() sets data to 0 if there's no setdef function installed in metadata
TEST(cm_simple_descriptor, setdef)
{
    int mem = 8;
    cm_simple_metadata d_d = {{"d01", 1 , sizeof(mem)}, NULL, NULL, NULL};

    cm_simple_descriptor d(&d_d, false);
    
    d.setdef((uint8_t *)&mem);
    CHECK(mem == 0);    
}


