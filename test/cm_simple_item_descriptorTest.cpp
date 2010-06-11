// Unit test using open-source unit test framework
// These tests use the cm_simple_item_descriptor's public interface
// to test it.  This includes redirecting to a file output that it sends to
// stdout, so that it can be read from the file and compared to the
// expected output.
// We also read/write the items themselves. 
//

#include "TestHarness.h"
#include "config_manager.h"  // Unit under test
#include "config_manager_util.h"     // Extensions to unit under test (generic "set" functions)

#include <string>
using namespace std;


int main()
{
	TestResult tr;
	TestRegistry::runAllTests(tr);
	return 0;
}


TEST(getLen, cm_simple_item_descriptor)
{
    cm_simple_item_descriptor d("d01", 1 , 55, NULL, NULL, NULL);
    CHECK(d.getLen() == 55);
}


TEST(print, cm_simple_item_descriptor)
{
    string prefix = "";
    unsigned mem = 7;
    char outstring[64];
    
    cm_simple_item_descriptor d("d01", 1 , sizeof(mem), NULL, NULL, NULL);

    // Redirect STDOUT to a file, so the test can examine what UUT writes there
    if (freopen("testout.txt", "w", stdout) == NULL)
    {
        cout << "redirecting stdout failed" << endl;
    }
    d.print((unsigned char *)&mem, prefix);

    // I tried using /dev/stdout instead of /dev/console -- that didn't work (no output to console was produced)
    freopen("/dev/console", "w", stdout);

    // xxx Reading from a file and comparing the contents could be re-implemented as an assert method
    FILE * resf = fopen("testout.txt", "r");
    fgets(outstring, sizeof(outstring), resf);
    CHECK(strncmp(outstring, "= 07000000\n", sizeof(outstring)) == 0);
}


TEST(set, cm_simple_item_descriptor)
{
    int mem = 0;

    cm_simple_item_descriptor d("d01", 1 , sizeof(mem), cm_set_int, NULL, NULL);
    
    d.set((unsigned char *)&mem, "4");
    CHECK(mem == 4);    
}


