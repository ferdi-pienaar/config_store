// Unit test using open-source unit test framework

#include "../../CppUnitLite/TestHarness.h"
#include "../config_manager.h"              // Unit under test


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

