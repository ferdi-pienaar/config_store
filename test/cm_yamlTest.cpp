// Unit test using open-source unit test framework
//
// xxx valid for little-endian systems only
//

#include "gtest/gtest.h"
#include "config_manager_yaml.h"  // Unit under test
#include <string.h> // strncmp
#include "nvram_spy.h"

#include <iostream>
using namespace std;

static uint8_t clientRam[1024];

namespace {
class YamTest : public testing::Test {
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
TEST_F(YamTest, DISABLED_writeSimple)
{
    uint8_t mem[] = {1,2,3,4};
    //                    T     L              V
    uint8_t expected[] = {55,0, sizeof(mem),0, 1,2,3,4};

    yaml->writeSimple("thing", sizeof(mem), mem, NULL);

    EXPECT_TRUE(nvram_spy_match(expected, sizeof(expected)));
}


//
TEST_F(YamTest, DISABLED_writeComposite)
{
    uint8_t s1[] = {1,2,3,4};
    uint8_t s2[] = {11,22,33,44};
    //                    T     L     T     L     V         T      L     V
    uint8_t expected[] = {55,0, 16,0, 44,0, 4,0,  1,2,3,4,  44,0,  4,0,  11,22,33,44};

    yaml->startWriteComposite("compo");
    yaml->writeSimple("inside", sizeof(s1), s1, NULL);
    yaml->writeSimple("next", sizeof(s2), s2, NULL);
    yaml->endWriteComposite();

    //cout << memcmp(expected, nvMem, sizeof(expected));

    EXPECT_TRUE(nvram_spy_match(expected, sizeof(expected)));
}

//
TEST_F(YamTest, DISABLED_loadSimple)
{
    //                   T        L    V
    uint8_t    nvSet[] = {0xab,0, 2,0, 55,0};
    uint8_t    expected[] = {55,0};
    cm_item_len_t length = sizeof(expected);

    // xxx set client RAM to bitpattern and verify only the expected section is modified

    nvram_spy_set(nvSet, sizeof(nvSet));

    yaml->startLoadSimple("simp");
    yaml->endLoadSimple(&length, clientRam);

    EXPECT_TRUE(memcmp(expected, clientRam, sizeof(expected)) == 0);
}

} // namespace

