// Unit test using open-source unit test framework
//
// xxx valid for little-endian systems only
//

#include "gtest/gtest.h"
#include "config_manager_yaml.h"  // Unit under test
#include "config_manager_set_int.h" // load and save functions re-used
#include <string.h> // strncmp
#include "nvram_spy.h"

#include <iostream>
using namespace std;
using namespace cfg_mgr;

static uint8_t clientRam[1024];

namespace {
class YamlTest : public testing::Test {
protected:
    Nvram nvram;
    Yaml * yaml;

    virtual void SetUp()
    {
        yaml = new Yaml(&nvram);
        nvram_spy_init();
    }

    virtual void TearDown()
    {
        //clean up steps are executed after each TEST
        delete yaml;
    }
};


//
TEST_F(YamlTest, DISABLED_writeSimple)
{
    uint8_t mem[] = {1,2,3,4};
    uint8_t expected[] = {55,0, sizeof(mem),0, 1,2,3,4};

    yaml->writeSimple("thing", sizeof(mem), mem, NULL);

    EXPECT_TRUE(nvram_spy_match(expected, sizeof(expected)));
}


//
TEST_F(YamlTest, DISABLED_writeComposite)
{
    uint8_t s1[] = {1,2,3,4};
    uint8_t s2[] = {11,22,33,44};
    uint8_t expected[] = {55,0, 16,0, 44,0, 4,0,  1,2,3,4,  44,0,  4,0,  11,22,33,44};

    yaml->startWriteComposite("compo");
    yaml->writeSimple("inside", sizeof(s1), s1, NULL);
    yaml->writeSimple("next", sizeof(s2), s2, NULL);
    yaml->endWriteComposite();

    //cout << memcmp(expected, nvMem, sizeof(expected));

    EXPECT_TRUE(nvram_spy_match(expected, sizeof(expected)));
}

//
TEST_F(YamlTest, DISABLED_loadSimple)
{
    uint8_t    nvSet[] = {"\"simp\": 55\n"};
    uint8_t    expected[] = {55,0};
    item_len_t length = sizeof(expected);

    // xxx set client RAM to bitpattern and verify only the expected section is modified

    nvram_spy_set(nvSet, sizeof(nvSet));

    EXPECT_EQ(CM_SUCCESS, yaml->startLoadSimple("simp"));
    yaml->endLoadSimple(&length, clientRam, cm_set_int);

    EXPECT_TRUE(memcmp(expected, clientRam, sizeof(expected)) == 0);
}

} // namespace

