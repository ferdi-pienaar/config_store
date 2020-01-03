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
        memset(clientRam, 0, sizeof(clientRam));
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
TEST_F(JsonTest, loadStringWithEscapedQuote)
{
    uint8_t    nvSet[] = {"\n"
                          " \"thing\": \"prop \\\"special\\\"\""
                         };
    uint8_t    expected[] = {"prop \\\"special\\\""};
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

// After trying to load something that's not found in JSON, load another successfully.
TEST_F(JsonTest, loadNotFound)
{
    uint8_t    nvSet[] = {"\"compo\": {\n"
                          " \"thing1\": \"property 13\"\n"
                          "}"
                         };
    uint8_t    expected[] = {"property 13"};
    item_len_t length = sizeof(expected);

    // xxx set client RAM to bitpattern and verify only the expected section is modified

    nvram->set(nvSet, sizeof(nvSet));
    EXPECT_EQ(CM_SUCCESS, json->startLoadComposite("compo"));
    EXPECT_EQ(CM_NOT_FOUND, json->startLoadSimple("bogus"));
    EXPECT_EQ(CM_SUCCESS, json->startLoadSimple("thing1"));
    EXPECT_EQ(CM_SUCCESS, json->endLoadSimple(&length, clientRam, cm_set_str));
    EXPECT_EQ(CM_SUCCESS, json->endLoadComposite());

    EXPECT_TRUE(memcmp(expected, clientRam, sizeof(expected)) == 0);
}

// Load component thing2, although there's an unknown component (thing1) in JSON.
TEST_F(JsonTest, loadSkipUnknown)
{
    uint8_t    nvSet[] = {"\"compo\": {\n"
                          " \"thing1\": \"property 13\",\n"
                          " \"thing2\": \"property 15\"\n"
                          "}"
                         };
    uint8_t    expected[] = {"property 15"};
    item_len_t length = sizeof(expected);

    // xxx set client RAM to bitpattern and verify only the expected section is modified

    nvram->set(nvSet, sizeof(nvSet));
    EXPECT_EQ(CM_SUCCESS, json->startLoadComposite("compo"));
    EXPECT_EQ(CM_SUCCESS, json->startLoadSimple("thing2"));
    EXPECT_EQ(CM_SUCCESS, json->endLoadSimple(&length, clientRam, cm_set_str));
    EXPECT_EQ(CM_SUCCESS, json->endLoadComposite());

    EXPECT_TRUE(memcmp(expected, clientRam, sizeof(expected)) == 0);
}

// xxx describe this failing test, which makes subsequent tests crash or fail.
TEST_F(JsonTest, DISABLED_loadSkipUnknown2)
{
    uint8_t    nvSet[] = {"\"compo\": {\n"
                          " \"thing1\": \"property 13\",\n"
                          " \"thing2\": \"property 15\"\n"
                          "}"
                         };
    uint8_t    expected[] = {"property 15"};
    item_len_t length = sizeof(expected);

    // xxx set client RAM to bitpattern and verify only the expected section is modified

    nvram->set(nvSet, sizeof(nvSet));

    EXPECT_EQ(CM_SUCCESS, json->startLoadSimple("thing2"));
    json->endLoadSimple(&length, clientRam, cm_set_str);

    EXPECT_TRUE(memcmp(expected, clientRam, sizeof(expected)) == 0);
}

TEST_F(JsonTest, loadComposite)
{
    uint8_t    nvSet[] = {"\"compo\": {\n"
                          " \"thing1\": \"property 13\",\n"
                          " \"thing2\": \"property 15\"\n"
                          "}"
                         };
    item_len_t length = 16;
    char expected[32] = {0};
    strncpy(expected, "property 13", length);
    strncpy(expected+length, "property 15", length);


    // xxx set client RAM to bitpattern and verify only the expected section is modified

    nvram->set(nvSet, sizeof(nvSet));

    EXPECT_EQ(CM_SUCCESS, json->startLoadComposite("compo"));
    EXPECT_EQ(CM_SUCCESS, json->startLoadSimple("thing1"));
    json->endLoadSimple(&length, clientRam, cm_set_str);
    EXPECT_EQ(CM_SUCCESS, json->startLoadSimple("thing2"));
    json->endLoadSimple(&length, clientRam + length, cm_set_str);
    json->endLoadComposite();

    EXPECT_TRUE(memcmp(&expected, clientRam, sizeof(expected)) == 0);
}

TEST_F(JsonTest, loadEmbeddedComposite)
{
    uint8_t    nvSet[] = {"\"compo\": {\n"
                          " \"compoInside\": {\n"
                          "  \"thing1\": \"property 13\",\n"
                          "  \"thing2\": \"property 15\"\n"
                          " }\n"
                          "}"
                         };
    item_len_t length = 16;
    char expected[32] = {0};
    strncpy(expected, "property 13", length);
    strncpy(expected+length, "property 15", length);


    // xxx set client RAM to bitpattern and verify only the expected section is modified

    nvram->set(nvSet, sizeof(nvSet));

    EXPECT_EQ(CM_SUCCESS, json->startLoadComposite("compo"));
    EXPECT_EQ(CM_SUCCESS, json->startLoadComposite("compoInside"));
    EXPECT_EQ(CM_SUCCESS, json->startLoadSimple("thing1"));
    EXPECT_EQ(CM_SUCCESS, json->endLoadSimple(&length, clientRam, cm_set_str));
    EXPECT_EQ(CM_SUCCESS, json->startLoadSimple("thing2"));
    EXPECT_EQ(CM_SUCCESS, json->endLoadSimple(&length, clientRam + length, cm_set_str));
    EXPECT_EQ(CM_SUCCESS, json->endLoadComposite());
    EXPECT_EQ(CM_SUCCESS, json->endLoadComposite());

    EXPECT_TRUE(memcmp(&expected, clientRam, sizeof(expected)) == 0);
}

TEST_F(JsonTest, loadArray)
{
    uint8_t    nvSet[] = {"\"array\": [\n"
                          " \"value 1\",\n"
                          " \"value 2\"\n"
                          "]"
                         };
    item_len_t length = 16;
    char expected[32] = {0};
    strncpy(expected, "value 1", length);
    strncpy(expected+length, "value 2", length);

    // xxx set client RAM to bitpattern and verify only the expected section is modified

    nvram->set(nvSet, sizeof(nvSet));

    EXPECT_EQ(CM_SUCCESS, json->startLoadArray("array"));
    EXPECT_EQ(CM_SUCCESS, json->startLoadSimple("array"));
    json->endLoadSimple(&length, clientRam, cm_set_str);
    EXPECT_EQ(CM_SUCCESS, json->startLoadSimple("array"));
    json->endLoadSimple(&length, clientRam + length, cm_set_str);
    json->endLoadArray();

    EXPECT_TRUE(memcmp(&expected, clientRam, sizeof(expected)) == 0);
}

// Try to load array elements beyond what's present in NVRAM -- a normal case,
// since client doesn't know how many entries there are.
// Check that next element, after array, is loaded OK, too.
TEST_F(JsonTest, loadArrayExtra)
{
    uint8_t    nvSet[] = {"\"compo\": {\n"
                          " \"array1\": [\n"
                          "  \"avalue 1\"\n"
                          " ],"
                          "\"simplename2\": \"simpleval 2\"\n"
                          "}"
                         };
    item_len_t length = 16;
    char expected[32] = {0};
    strncpy(expected, "avalue 1", length);
    strncpy(expected+length, "simpleval 2", length);

    // xxx set client RAM to bitpattern and verify only the expected section is modified

    nvram->set(nvSet, sizeof(nvSet));

    EXPECT_EQ(CM_SUCCESS, json->startLoadComposite("compo"));
    EXPECT_EQ(CM_SUCCESS, json->startLoadArray("array1"));
    EXPECT_EQ(CM_SUCCESS, json->startLoadSimple("array1"));
    EXPECT_EQ(CM_SUCCESS, json->endLoadSimple(&length, clientRam, cm_set_str));
    // Next array entry is not found.
    EXPECT_EQ(CM_NOT_FOUND, json->startLoadSimple("array1"));
    // But load end array is OK.
    EXPECT_EQ(CM_SUCCESS, json->endLoadArray());
    EXPECT_EQ(CM_SUCCESS, json->startLoadSimple("simplename2"));
    EXPECT_EQ(CM_SUCCESS, json->endLoadSimple(&length, clientRam + length, cm_set_str));

    EXPECT_TRUE(memcmp(&expected, clientRam, sizeof(expected)) == 0);
}

TEST_F(JsonTest, loadArrayOfComposite)
{
    uint8_t    nvSet[] = {"\"compo\": [\n"
                          " {\n"
                          "  \"thing1\": \"property 13\",\n"
                          "  \"thing2\": \"property 15\"\n"
                          " },\n"
                          " {\n"
                          "  \"thing1\": \"property 20\",\n"
                          "  \"thing2\": \"property 22\"\n"
                          " }\n"
                          "]"
                         };
    item_len_t length = 16;
    char expected[64] = {0};
    strncpy(expected, "property 13", length);
    strncpy(expected+length, "property 15", length);
    strncpy(expected+2*length, "property 20", length);
    strncpy(expected+3*length, "property 22", length);

    // xxx set client RAM to bitpattern and verify only the expected section is modified

    nvram->set(nvSet, sizeof(nvSet));

    EXPECT_EQ(CM_SUCCESS, json->startLoadArray("compo"));
    EXPECT_EQ(CM_SUCCESS, json->startLoadComposite("compo"));
    EXPECT_EQ(CM_SUCCESS, json->startLoadSimple("thing1"));
    EXPECT_EQ(CM_SUCCESS, json->endLoadSimple(&length, clientRam, cm_set_str));
    EXPECT_EQ(CM_SUCCESS, json->startLoadSimple("thing2"));
    EXPECT_EQ(CM_SUCCESS, json->endLoadSimple(&length, clientRam + length, cm_set_str));
    EXPECT_EQ(CM_SUCCESS, json->endLoadComposite());
    EXPECT_EQ(CM_SUCCESS, json->startLoadComposite("compo"));
    EXPECT_EQ(CM_SUCCESS, json->startLoadSimple("thing1"));
    EXPECT_EQ(CM_SUCCESS, json->endLoadSimple(&length, clientRam + 2 * length, cm_set_str));
    EXPECT_EQ(CM_SUCCESS, json->startLoadSimple("thing2"));
    EXPECT_EQ(CM_SUCCESS, json->endLoadSimple(&length, clientRam + 3 * length, cm_set_str));
    EXPECT_EQ(CM_SUCCESS, json->endLoadComposite());
    EXPECT_EQ(CM_SUCCESS, json->endLoadArray());

    EXPECT_TRUE(memcmp(&expected, clientRam, sizeof(expected)) == 0);
}

} // namespace

