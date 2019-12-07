// Unit test using open-source unit test framework
//
//

#include "gtest/gtest.h"
#include "config_manager_json.h"  // Unit under test
#include "config_manager_util.h" // load and save functions re-used
#include "nvram_spy.h"
#include <iostream>

using namespace std;
using namespace cfg_mgr;

static uint8_t clientRam[1024];

namespace {
class JsonTest : public testing::Test {
protected:
    Nvram nvram;
    Json * json;

    virtual void SetUp()
    {
        json = new Json(&nvram);
        nvram_spy_init();
    }

    virtual void TearDown()
    {
        //clean up steps are executed after each TEST
        delete json;
    }
};

TEST_F(JsonTest, writeSimple)
{
    uint32_t mem = 67305985; // 0x04030201
    string expected = "\n\"thing\": 67305985";

    json->writeSimple("thing", sizeof(mem), (uint8_t *)&mem, cm_prt_int);

    EXPECT_TRUE(nvram_spy_match((uint8_t *)expected.c_str(), expected.length()));
}

TEST_F(JsonTest, writeComposite)
{
    uint32_t s1 = 67305985; // 0x04030201
    uint32_t s2 = 16843009; // 0x01010101
    string expected = "\n\"compo\": {\n \"inside\": 67305985,\n \"next\": 16843009\n}";

    json->startWriteComposite("compo");
    json->writeSimple("inside", sizeof(s1), (uint8_t *)&s1, cm_prt_int);
    json->writeSimple("next", sizeof(s2), (uint8_t *)&s2, cm_prt_int);
    json->endWriteComposite();

    EXPECT_TRUE(nvram_spy_match((uint8_t *)expected.c_str(), expected.length()));
}

TEST_F(JsonTest, writeEmbeddedComposite)
{
    uint32_t s1 = 67305985; // 0x04030201
    string expected = "\n\"compo\": {\n \"compo2\": {\n  \"inside\": 67305985\n }\n}";

    json->startWriteComposite("compo");
    json->startWriteComposite("compo2");
    json->writeSimple("inside", sizeof(s1), (uint8_t *)&s1, cm_prt_int);
    json->endWriteComposite();
    json->endWriteComposite();

    EXPECT_TRUE(nvram_spy_match((uint8_t *)expected.c_str(), expected.length()));
}


TEST_F(JsonTest, writeArray)
{
    uint32_t s1 = 67305985; // 0x04030201
    uint32_t s2 = 16843009; // 0x01010101
    string expected = "\n\"values\": [\n 67305985,\n 16843009\n]";

    json->startWriteArray("values");
    json->writeSimple("inside", sizeof(s1), (uint8_t *)&s1, cm_prt_int);
    json->writeSimple("next", sizeof(s2), (uint8_t *)&s2, cm_prt_int);
    json->endWriteArray();

    EXPECT_TRUE(nvram_spy_match((uint8_t *)expected.c_str(), expected.length()));
}

//
TEST_F(JsonTest, DISABLED_loadSimple)
{
    uint8_t    nvSet[] = {"\"simp\": 55\n"};
    uint8_t    expected[] = {55,0};
    item_len_t length = sizeof(expected);

    // xxx set client RAM to bitpattern and verify only the expected section is modified

    nvram_spy_set(nvSet, sizeof(nvSet));

    EXPECT_EQ(CM_SUCCESS, json->startLoadSimple("simp"));
    json->endLoadSimple(&length, clientRam, cm_set_int);

    EXPECT_TRUE(memcmp(expected, clientRam, sizeof(expected)) == 0);
}

} // namespace

