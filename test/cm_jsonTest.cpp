// Unit test using open-source unit test framework
//
//

#include "gtest/gtest.h"
#include "cfg_mgr_json.h"  // Unit under test
#include "cfg_mgr_prt_int.h" // load and save functions re-used
#include "cfg_mgr_prt_str.h" // load and save functions re-used
#include "cfg_mgr_set_int.h" // load and save functions re-used
#include "cfg_mgr_set_str.h" // load and save functions re-used
#include "nvram_spy.h"
#include <iostream>

using namespace std;
using namespace cfg_mgr;

static uint8_t clientRam[1024];

namespace {
class JsonTest : public testing::Test {
protected:
    Nvram_spy * nvram;
    Json * json;

    virtual void SetUp()
    {
        nvram = new Nvram_spy;
        json = new Json(nvram);
    }

    virtual void TearDown()
    {
        //clean up steps are executed after each TEST
        delete json;
        delete nvram;
    }
};

TEST_F(JsonTest, writeSimple)
{
    uint32_t mem = 67305985; // 0x04030201
    string expected = "\n"
                      " \"thing\": 67305985";

    json->writeSimple("thing", sizeof(mem), (uint8_t *)&mem, cm_prt_int);

    EXPECT_TRUE(nvram->match((uint8_t *)expected.c_str(), expected.length()));
}

TEST_F(JsonTest, writeSimpleString)
{
    const char mem[] = "property 13";
    string expected = "\n"
                      " \"thing\": \"property 13\"";

    json->writeSimple("thing", sizeof(mem), (uint8_t *)&mem, cm_prt_str);

    EXPECT_TRUE(nvram->match((uint8_t *)expected.c_str(), expected.length()));
}

TEST_F(JsonTest, writeComposite)
{
    uint32_t s1 = 67305985; // 0x04030201
    uint32_t s2 = 16843009; // 0x01010101
    string expected = "\n"
                      " \"compo\": {\n"
                      "  \"inside\": 67305985,\n"
                      "  \"next\": 16843009\n"
                      " }";

    json->startWriteComposite("compo");
    json->writeSimple("inside", sizeof(s1), (uint8_t *)&s1, cm_prt_int);
    json->writeSimple("next", sizeof(s2), (uint8_t *)&s2, cm_prt_int);
    json->endWriteComposite();

    EXPECT_TRUE(nvram->match((uint8_t *)expected.c_str(), expected.length()));
}

TEST_F(JsonTest, writeEmbeddedComposite)
{
    uint32_t s1 = 67305985; // 0x04030201
    string expected = "\n"
                      " \"compo\": {\n"
                      "  \"compo2\": {\n"
                      "   \"inside\": 67305985\n"
                      "  }\n"
                      " }";

    json->startWriteComposite("compo");
    json->startWriteComposite("compo2");
    json->writeSimple("inside", sizeof(s1), (uint8_t *)&s1, cm_prt_int);
    json->endWriteComposite();
    json->endWriteComposite();

    EXPECT_TRUE(nvram->match((uint8_t *)expected.c_str(), expected.length()));
}


TEST_F(JsonTest, writeArrayOfSimple)
{
    uint32_t s1 = 67305985; // 0x04030201
    uint32_t s2 = 16843009; // 0x01010101
    string expected = "\n"
                      " \"values\": [\n"
                      "  67305985,\n"
                      "  16843009\n"
                      " ]";

    json->startWriteArray("values");
    json->writeSimple("inside", sizeof(s1), (uint8_t *)&s1, cm_prt_int);
    json->writeSimple("next", sizeof(s2), (uint8_t *)&s2, cm_prt_int);
    json->endWriteArray();

    EXPECT_TRUE(nvram->match((uint8_t *)expected.c_str(), expected.length()));
}

TEST_F(JsonTest, writeArrayOfComposite)
{
    const char v1[] = "property 1";
    const char v2[] = "property 2";
    string expected = "\n"
                      " \"array\": [\n"
                      "  {\n"
                      "   \"v1\": \"property 1\"\n"
                      "  },\n"
                      "  {\n"
                      "   \"v2\": \"property 2\"\n"
                      "  }\n"
                      " ]";

    json->startWriteArray("array");
    json->startWriteComposite("composite1");
    json->writeSimple("v1", sizeof(v1), (uint8_t *)v1, cm_prt_str);
    json->endWriteComposite();
    json->startWriteComposite("composite2");
    json->writeSimple("v2", sizeof(v2), (uint8_t *)v2, cm_prt_str);
    json->endWriteComposite();
    json->endWriteArray();
    EXPECT_TRUE(nvram->match((uint8_t *)expected.c_str(), expected.length()));
}

//
TEST_F(JsonTest, loadString)
{
    uint8_t    nvSet[] = {"\n"
                          " \"thing\": \"property 13\""
                         };
    uint8_t    expected[] = {"property 13"};
    item_len_t length = sizeof(expected);

    // xxx set client RAM to bitpattern and verify only the expected section is modified

    nvram->set(nvSet, sizeof(nvSet));

    EXPECT_EQ(CM_SUCCESS, json->startLoadSimple("thing"));
    json->endLoadSimple(&length, clientRam, cm_set_str);

    EXPECT_TRUE(memcmp(expected, clientRam, sizeof(expected)) == 0);
}

//
TEST_F(JsonTest, loadNonString)
{
    uint8_t    nvSet[] = {"\n"
                          " \"thing\": 1002003"
                         };
    uint32_t    expected = 1002003;
    item_len_t length = sizeof(expected);

    // xxx set client RAM to bitpattern and verify only the expected section is modified

    nvram->set(nvSet, sizeof(nvSet));

    EXPECT_EQ(CM_SUCCESS, json->startLoadSimple("thing"));
    json->endLoadSimple(&length, clientRam, cm_set_int);

    EXPECT_TRUE(memcmp(&expected, clientRam, sizeof(expected)) == 0);
}

} // namespace

