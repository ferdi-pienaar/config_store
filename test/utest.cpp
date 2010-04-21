// Unit test using open-source unit test framework

#include "../../CppUnitLite/TestHarness.h"
#include "../config_manager.h"              // Unit under test
#include <string>


int main()
{
	TestResult tr;
	TestRegistry::runAllTests(tr);
	return 0;
}


TEST(getLen, cm_simple_item_descriptor)
{
    cm_simple_item_descriptor * d = new cm_simple_item_descriptor("d01",
                                                                  1, // ID
                                                                  55, // length 
                                                                  NULL,
                                                                  NULL,
                                                                  NULL);
    CHECK(d->getLen() == 55);
}


TEST(print, cm_simple_item_descriptor)
{
    string prefix = "";
    unsigned mem = 7;
    char outstring[64];
    
    cm_simple_item_descriptor * d = new cm_simple_item_descriptor("d01",
                                                                  1, // ID
                                                                  sizeof(mem), // length 
                                                                  NULL,
                                                                  NULL,
                                                                  NULL);

    // Redirect STDOUT to a file, so the test can examine it
    freopen("testres.txt", "w", stdout);
    d->print((unsigned char *)&mem, prefix);

    // I don't know why it works when I redirect to /dev/stderr, but not when it's /dev/stdout.
    // Maybe it's because stderr is NOT buffered.
    freopen("/dev/stderr", "w", stdout);


    // xxx Reading from a file and comparing the contents could be re-implemented as an assert method
    FILE * resf = fopen("testres.txt", "r");
    fgets(outstring, sizeof(outstring), resf);
    CHECK(strncmp(outstring, "07000000\n", sizeof(outstring)) == 0);
}


