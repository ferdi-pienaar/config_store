// Unit test using open-source unit test framework
//
// xxx valid for little-endian systems only
//

#include "gtest/gtest.h"
#include "config_manager_tlv.h"  // Unit under test
#include <string.h> // strncmp
#include "nvram_spy.h"

#include <iostream>
using namespace std;

static uint8_t clientRam[1024];

namespace {
class TlvTest : public testing::Test {
protected:
    Nvram nvram;
    Tlv * tlv;

    virtual void SetUp()
    {
        tlv = new Tlv(&nvram);
        nvram_spy_init();
    }

    virtual void TearDown()
    {
        //clean up steps are executed after each TEST
        delete tlv;
    }
};


//
TEST_F(TlvTest, writeSimple)
{
    cm_item_id_t id = 55;

    uint8_t mem[] = {1,2,3,4};
    //                    T     L              V
    uint8_t expected[] = {55,0, sizeof(mem),0, 1,2,3,4};

    tlv->writeSimple(id, sizeof(mem), mem);

    EXPECT_TRUE(nvram_spy_match(expected, sizeof(expected)));
}


//
TEST_F(TlvTest, writeComposite)
{
    cm_item_id_t cId = 55;
    cm_item_id_t sId = 44;

    uint8_t s1[] = {1,2,3,4};
    uint8_t s2[] = {11,22,33,44};
    //                    T     L     T     L     V         T      L     V
    uint8_t expected[] = {55,0, 16,0, 44,0, 4,0,  1,2,3,4,  44,0,  4,0,  11,22,33,44};

    tlv->startWriteComposite(cId);
    tlv->writeSimple(sId, sizeof(s1), s1);
    tlv->writeSimple(sId, sizeof(s2), s2);
    tlv->endWriteComposite();

    //cout << memcmp(expected, nvMem, sizeof(expected));

    EXPECT_TRUE(nvram_spy_match(expected, sizeof(expected)));
}


//
TEST_F(TlvTest, writeNestedComposite)
{
    cm_item_id_t id = 0xab; // All items are at different levels, so they can share an ID

    uint8_t s1[] = {1,2,3,4,5,6};
    //                    T       L     T       L      T       L     V
    uint8_t expected[] = {0xab,0, 14,0, 0xab,0, 10,0,  0xab,0, 6,0,  1,2,3,4,5,6};

    tlv->startWriteComposite(id);
    tlv->startWriteComposite(id);
    tlv->writeSimple(id, sizeof(s1), s1);
    tlv->endWriteComposite();
    tlv->endWriteComposite();

    EXPECT_TRUE(nvram_spy_match(expected, sizeof(expected)));
}


//
TEST_F(TlvTest, writeNestedCompositeAndSimple)
{
    uint8_t s1[] = {1,2,3,4,5,6};
    uint8_t s2[] = {10,11};
    //                    T       L     T       L      T       L     V            T       L    V
    uint8_t expected[] = {0xab,0, 20,0, 0xab,0, 10,0,  0xab,0, 6,0,  1,2,3,4,5,6, 0xbc,0, 2,0, 10,11};

    tlv->startWriteComposite(0xab);
    tlv->startWriteComposite(0xab);
    tlv->writeSimple(0xab, sizeof(s1), s1);
    tlv->endWriteComposite();
    tlv->writeSimple(0xbc, sizeof(s2), s2);
    tlv->endWriteComposite();

    EXPECT_TRUE(nvram_spy_match(expected, sizeof(expected)));
}


//
TEST_F(TlvTest, loadSimple)
{
    //                   T        L    V
    uint8_t    nvSet[] = {0xab,0, 2,0, 55,0};
    uint8_t    expected[] = {55,0};
    cm_item_len_t length = sizeof(expected);

    // xxx set client RAM to bitpattern and verify only the expected section is modified

    nvram_spy_set(nvSet, sizeof(nvSet));

    tlv->loadSimple(0xab, &length, clientRam);

    EXPECT_TRUE(memcmp(expected, clientRam, sizeof(expected)) == 0);
}

TEST_F(TlvTest, invalidEndComposite)
{
    EXPECT_EQ(CM_INCOHERENT_DATA, tlv->endLoadComposite());
}

//
TEST_F(TlvTest, loadComposite)
{
    //                    T       L     T    L    V     T    L    V
    uint8_t    nvSet[] = {0xab,0, 12,0, 4,0, 2,0, 55,0, 4,0, 2,0, 66,0};
    uint8_t    expected[] = {55,0, 66,0};
    cm_item_len_t length;

    // xxx set client RAM to bitpattern and verify only the expected section is modified
    nvram_spy_set(nvSet, sizeof(nvSet));

    tlv->startLoadComposite(0xab);

    length = 2;
    tlv->loadSimple(4, &length, clientRam);
    EXPECT_EQ(2, length);

    tlv->loadSimple(4, &length, clientRam + length);
    EXPECT_EQ(2, length);
    EXPECT_EQ(CM_SUCCESS, tlv->endLoadComposite());
    EXPECT_TRUE(memcmp(expected, clientRam, sizeof(expected)) == 0);
}

// Load only the 2nd component in a composite.
TEST_F(TlvTest, partialLoadComposite)
{
    //                    T       L     T       L    V     T       L    V
    uint8_t    nvSet[] = {0xab,0, 12,0, 0x99,0, 2,0, 55,0, 0xDD,0, 2,0, 66,0};
    uint8_t    expected[] = {66,0};
    cm_item_len_t length;

    // xxx set client RAM to bitpattern and verify only the expected section is modified
    nvram_spy_set(nvSet, sizeof(nvSet));

    tlv->startLoadComposite(0xab);

    length = 2;
    tlv->loadSimple(0xDD, &length, clientRam);
    EXPECT_EQ(2, length);
    EXPECT_EQ(CM_SUCCESS, tlv->endLoadComposite());
    EXPECT_TRUE(memcmp(expected, clientRam, sizeof(expected)) == 0);
}

// During composite load, client first tries to load component that's not in TLV.
// Subsequent loads of components that are present are OK.
TEST_F(TlvTest, findFailLoadComposite)
{
    //                    T       L     T    L    V     T    L    V
    uint8_t    nvSet[] = {0xab,0, 12,0, 4,0, 2,0, 55,0, 4,0, 2,0, 66,0};
    uint8_t    expected[] = {55,0, 66,0};
    cm_item_len_t length;

    // xxx set client RAM to bitpattern and verify only the expected section is modified
    nvram_spy_set(nvSet, sizeof(nvSet));

    tlv->startLoadComposite(0xab);
    
    length = 2;
    t_cm_result ret = tlv->loadSimple(0xdeff, &length, clientRam);
    EXPECT_EQ(ret, CM_NOT_FOUND);

    length = 2;
    tlv->loadSimple(4, &length, clientRam);
    EXPECT_EQ(2, length);

    tlv->loadSimple(4, &length, clientRam + length);
    EXPECT_EQ(2, length);
    EXPECT_EQ(CM_SUCCESS, tlv->endLoadComposite());
    EXPECT_TRUE(memcmp(expected, clientRam, sizeof(expected)) == 0);
}

//
// xxx incomplete
TEST_F(TlvTest, loadNestedComposite)
{
    //                    T       L     T    L    V    T    L        V
    uint8_t    nvSet[] = {0xab,0, 14,0, 0xab,0, 10,0,  0xab,0, 6,0,  1,2,3,4,5,6};
    uint8_t    expected[] = {1,2,3,4,5,6};
    cm_item_len_t length = sizeof(expected);


    // xxx set client RAM to bitpattern and verify only the expected section is modified

    nvram_spy_set(nvSet, sizeof(nvSet));

    tlv->startLoadComposite(0xab);

    tlv->startLoadComposite(0xab);

    tlv->loadSimple(0xab, &length, clientRam);
    EXPECT_EQ(CM_SUCCESS, tlv->endLoadComposite());
    EXPECT_EQ(CM_SUCCESS, tlv->endLoadComposite());
    EXPECT_TRUE(memcmp(expected, clientRam, sizeof(expected)) == 0);
}


//
TEST_F(TlvTest, loadNestedCompositeAndSimple)
{
    //                 T       L     T       L      T       L     V            T       L    V
    uint8_t nvSet[] = {0xab,0, 20,0, 0xab,0, 10,0,  0xab,0, 6,0,  1,2,3,4,5,6, 0xbc,0, 2,0, 10,11};
    uint8_t expected[] = {1,2,3,4,5,6,10,11};
    cm_item_len_t length;

    nvram_spy_set(nvSet, sizeof(nvSet));

    tlv->startLoadComposite(0xab);

    tlv->startLoadComposite(0xab);

    length = 6;
    tlv->loadSimple(0xab, &length, clientRam);

    length = 2;
    tlv->loadSimple(0xbc, &length, clientRam + 6);
    EXPECT_EQ(CM_SUCCESS, tlv->endLoadComposite());
    EXPECT_EQ(CM_SUCCESS, tlv->endLoadComposite());
    EXPECT_TRUE(memcmp(expected, clientRam, sizeof(expected)) == 0);
}

//
TEST_F(TlvTest, loadNestedCompositeOf2Simples)
{
    //                 T       L     T       L      T       L     V            T       L    V
    uint8_t nvSet[] = {0xab,0, 20,0, 0xab,0, 16,0,  0xab,0, 6,0,  1,2,3,4,5,6, 0xbc,0, 2,0, 10,11};
    uint8_t expected[] = {1,2,3,4,5,6,10,11};
    cm_item_len_t length;

    nvram_spy_set(nvSet, sizeof(nvSet));

    tlv->startLoadComposite(0xab);

    tlv->startLoadComposite(0xab);

    length = 6;
    tlv->loadSimple(0xab, &length, clientRam);

    length = 2;
    tlv->loadSimple(0xbc, &length, clientRam + 6);
    EXPECT_EQ(CM_SUCCESS, tlv->endLoadComposite());
    EXPECT_EQ(CM_SUCCESS, tlv->endLoadComposite());
    EXPECT_TRUE(memcmp(expected, clientRam, sizeof(expected)) == 0);
}

//
TEST_F(TlvTest, load2Composites)
{
    //                    T       L    T       L    V     T       L    T       L    V
    uint8_t    nvSet[] = {0xab,0, 6,0, 0x31,0, 2,0, 55,0, 0xda,0, 6,0, 0xbc,0, 2,0, 88,0};
    uint8_t expected[] = {55,00, 88,0};
    cm_item_len_t length;

    nvram_spy_set(nvSet, sizeof(nvSet));

    tlv->startLoadComposite(0xab);
    length = 2;
    tlv->loadSimple(0x31, &length, clientRam);
    EXPECT_EQ(CM_SUCCESS, tlv->endLoadComposite());

    tlv->startLoadComposite(0xda);
    tlv->loadSimple(0xbc, &length, clientRam + length);
    EXPECT_EQ(CM_SUCCESS, tlv->endLoadComposite());
    EXPECT_TRUE(memcmp(expected, clientRam, sizeof(expected)) == 0);
}


TEST_F(TlvTest, loadTruncatedSimple)
{
    //                 T       L     V ...
    uint8_t nvSet[] = {0xab,0, 4,0,  1};
    cm_item_len_t length;

    nvram_spy_set(nvSet, sizeof(nvSet));

    length = 4;
    t_cm_result res = tlv->loadSimple(0xab, &length, clientRam);
    EXPECT_EQ(CM_READ_FAIL, res);
}
} // namespace

