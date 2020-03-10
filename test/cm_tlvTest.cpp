// Unit test using open-source unit test framework
//
// xxx valid for little-endian systems only
//

#include "gtest/gtest.h"
#include "cfg_mgr_tlv.h"  // Unit under test
#include <cstring> // memcmp
#include "nvram_spy.h"

#include <iostream>
using namespace std;
using namespace cfg_mgr;

static uint8_t clientRam[1024];

namespace {
class TlvTest : public testing::Test {
protected:
    Nvram_spy * nvram;
    Tlv * tlv;

    virtual void SetUp()
    {
        nvram = new Nvram_spy;
        tlv = new Tlv(nvram);
    }

    virtual void TearDown()
    {
        //clean up steps are executed after each TEST
        delete tlv;
        delete nvram;
    }
};


//
TEST_F(TlvTest, writeSimple)
{
    item_id_t id = 55;

    uint8_t mem[] = {1,2,3,4};
    //                    T     L              V
    uint8_t expected[] = {55,0, sizeof(mem),0, 1,2,3,4};

    tlv->writeSimple(id, sizeof(mem), mem);

    EXPECT_TRUE(nvram->match(expected, sizeof(expected)));
}


//
TEST_F(TlvTest, writeComposite)
{
    item_id_t cId = 55;
    item_id_t sId = 44;

    uint8_t s1[] = {1,2,3,4};
    uint8_t s2[] = {11,22,33,44};
    //                    T     L     T     L     V         T      L     V
    uint8_t expected[] = {55,0, 16,0, 44,0, 4,0,  1,2,3,4,  44,0,  4,0,  11,22,33,44};

    tlv->startWriteComposite(cId);
    tlv->writeSimple(sId, sizeof(s1), s1);
    tlv->writeSimple(sId, sizeof(s2), s2);
    tlv->endWriteComposite();

    //cout << memcmp(expected, nvMem, sizeof(expected));

    EXPECT_TRUE(nvram->match(expected, sizeof(expected)));
}


//
TEST_F(TlvTest, writeNestedComposite)
{
    item_id_t id = 0xab; // All items are at different levels, so they can share an ID

    uint8_t s1[] = {1,2,3,4,5,6};
    //                    T       L     T       L      T       L     V
    uint8_t expected[] = {0xab,0, 14,0, 0xab,0, 10,0,  0xab,0, 6,0,  1,2,3,4,5,6};

    tlv->startWriteComposite(id);
    tlv->startWriteComposite(id);
    tlv->writeSimple(id, sizeof(s1), s1);
    tlv->endWriteComposite();
    tlv->endWriteComposite();

    EXPECT_TRUE(nvram->match(expected, sizeof(expected)));
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

    EXPECT_TRUE(nvram->match(expected, sizeof(expected)));
}


//
TEST_F(TlvTest, loadSimple)
{
    //                   T        L    V
    uint8_t    nvSet[] = {0xab,0, 2,0, 55,0};
    uint8_t    expected[] = {55,0};
    item_len_t length = sizeof(expected);

    // xxx set client RAM to bitpattern and verify only the expected section is modified

    nvram->set(nvSet, sizeof(nvSet));

    tlv->startLoadSimple(0xab);
    tlv->endLoadSimple(&length, clientRam);

    EXPECT_TRUE(memcmp(expected, clientRam, sizeof(expected)) == 0);
}

TEST_F(TlvTest, invalidEndLoadComposite)
{
    EXPECT_EQ(CM_INCOHERENT_DATA, tlv->endLoadComposite());
}

// Client tries to load a composite when NVRAM is empty.
// Nothing should be written to clientRam.
TEST_F(TlvTest, loadEmptyComposite)
{
    nvram->set(NULL, 0);

    EXPECT_EQ(CM_READ_FAIL, tlv->startLoadComposite(0));
}

//
TEST_F(TlvTest, loadComposite)
{
    //                    T       L     T    L    V     T    L    V
    uint8_t    nvSet[] = {0xab,0, 12,0, 4,0, 2,0, 55,0, 4,0, 2,0, 66,0};
    uint8_t    expected[] = {55,0, 66,0};
    item_len_t length;

    // xxx set client RAM to bitpattern and verify only the expected section is modified
    nvram->set(nvSet, sizeof(nvSet));

    tlv->startLoadComposite(0xab);

    tlv->startLoadSimple(4);
    length = 2;
    tlv->endLoadSimple(&length, clientRam);
    EXPECT_EQ(2, length);

    tlv->startLoadSimple(4);
    tlv->endLoadSimple(&length, clientRam + length);
    EXPECT_EQ(2, length);
    EXPECT_EQ(CM_SUCCESS, tlv->endLoadComposite());
    EXPECT_TRUE(memcmp(expected, clientRam, sizeof(expected)) == 0);
}

// xxx disabled because feature not implemented yet.
TEST_F(TlvTest, DISABLED_loadCompositeOutOfOrder)
{
    //                    T       L     T    L    V     T    L    V
    uint8_t    nvSet[] = {0xab,0, 12,0, 8,0, 2,0, 55,0, 9,0, 2,0, 66,0};
    uint8_t    expected[] = {66,0, 55,0};
    item_len_t length;

    // xxx set client RAM to bitpattern and verify only the expected section is modified
    nvram->set(nvSet, sizeof(nvSet));

    tlv->startLoadComposite(0xab);

    // Load T=9 first, which is 2nd in NVRAM, then load T=8, which is 1st in NVRAM.
    tlv->startLoadSimple(9);
    length = 2;
    tlv->endLoadSimple(&length, clientRam);
    EXPECT_EQ(2, length);

    tlv->startLoadSimple(8);
    tlv->endLoadSimple(&length, clientRam + length);
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
    item_len_t length;

    // xxx set client RAM to bitpattern and verify only the expected section is modified
    nvram->set(nvSet, sizeof(nvSet));

    tlv->startLoadComposite(0xab);

    length = 2;
    tlv->startLoadSimple(0xDD);
    tlv->endLoadSimple(&length, clientRam);
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
    item_len_t length;

    // xxx set client RAM to bitpattern and verify only the expected section is modified
    nvram->set(nvSet, sizeof(nvSet));

    tlv->startLoadComposite(0xab);

    length = 2;
    result_t ret = tlv->startLoadSimple(0xdeff);
    EXPECT_EQ(ret, CM_NOT_FOUND);

    length = 2;
    tlv->startLoadSimple(4);
    tlv->endLoadSimple(&length, clientRam);
    EXPECT_EQ(2, length);

    tlv->startLoadSimple(4);
    tlv->endLoadSimple(&length, clientRam + length);
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
    item_len_t length = sizeof(expected);


    // xxx set client RAM to bitpattern and verify only the expected section is modified

    nvram->set(nvSet, sizeof(nvSet));

    tlv->startLoadComposite(0xab);

    tlv->startLoadComposite(0xab);

    tlv->startLoadSimple(0xab);
    tlv->endLoadSimple(&length, clientRam);
    EXPECT_EQ(CM_SUCCESS, tlv->endLoadComposite());
    EXPECT_EQ(CM_SUCCESS, tlv->endLoadComposite());
    EXPECT_TRUE(memcmp(expected, clientRam, sizeof(expected)) == 0);
}


//
TEST_F(TlvTest, loadNestedCompositeAndSimple)
{
    // The outer composite includes all, the inner contains the first simple only.
    //                 T       L     T       L      T       L     V            T       L    V
    uint8_t nvSet[] = {0xab,0, 20,0, 0xab,0, 10,0,  0xab,0, 6,0,  1,2,3,4,5,6, 0xbc,0, 2,0, 10,11};
    uint8_t expected[] = {1,2,3,4,5,6,10,11};
    item_len_t length;

    nvram->set(nvSet, sizeof(nvSet));

    tlv->startLoadComposite(0xab);
    tlv->startLoadComposite(0xab);
    length = 6;
    tlv->startLoadSimple(0xab);
    tlv->endLoadSimple(&length, clientRam);
    EXPECT_EQ(CM_SUCCESS, tlv->endLoadComposite());

    length = 2;
    tlv->startLoadSimple(0xbc);
    tlv->endLoadSimple(&length, clientRam + 6);
    EXPECT_EQ(CM_SUCCESS, tlv->endLoadComposite());

    EXPECT_TRUE(memcmp(expected, clientRam, sizeof(expected)) == 0);
}

// After an inner composite that's partly loaded, client can load the component that follows.
TEST_F(TlvTest, loadCompositeWithUnwantedComponentAndSimple)
{
    // The outer composite includes all, the inner contains the first simple, which is not loaded by client.
    //                 T       L     T       L      T       L     V            T       L    V
    uint8_t nvSet[] = {0xab,0, 20,0, 0xab,0, 10,0,  0xab,0, 6,0,  1,2,3,4,5,6, 0xbc,0, 2,0, 10,11};
    uint8_t expected[] = {10,11};
    item_len_t length;

    nvram->set(nvSet, sizeof(nvSet));

    tlv->startLoadComposite(0xab);
    tlv->startLoadComposite(0xab);
    length = 6;
    EXPECT_EQ(CM_NOT_FOUND, tlv->startLoadSimple(0xd00d));
    // not found, so client doesn't call endLoadSimple.
    EXPECT_EQ(CM_SUCCESS, tlv->endLoadComposite());

    length = 2;
    tlv->startLoadSimple(0xbc);
    tlv->endLoadSimple(&length, clientRam);
    EXPECT_EQ(CM_SUCCESS, tlv->endLoadComposite());

    EXPECT_TRUE(memcmp(expected, clientRam, sizeof(expected)) == 0);
}

//
TEST_F(TlvTest, loadNestedCompositeOf2Simples)
{
    //                 T       L     T       L      T       L     V            T       L    V
    uint8_t nvSet[] = {0xab,0, 20,0, 0xab,0, 16,0,  0xab,0, 6,0,  1,2,3,4,5,6, 0xbc,0, 2,0, 10,11};
    uint8_t expected[] = {1,2,3,4,5,6,10,11};
    item_len_t length;

    nvram->set(nvSet, sizeof(nvSet));

    tlv->startLoadComposite(0xab);

    tlv->startLoadComposite(0xab);

    length = 6;
    tlv->startLoadSimple(0xab);
    tlv->endLoadSimple(&length, clientRam);

    length = 2;
    tlv->startLoadSimple(0xbc);
    tlv->endLoadSimple(&length, clientRam + 6);
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
    item_len_t length;

    nvram->set(nvSet, sizeof(nvSet));

    tlv->startLoadComposite(0xab);
    length = 2;
    tlv->startLoadSimple(0x31);
    tlv->endLoadSimple(&length, clientRam);
    EXPECT_EQ(CM_SUCCESS, tlv->endLoadComposite());

    tlv->startLoadComposite(0xda);
    tlv->startLoadSimple(0xbc);
    tlv->endLoadSimple(&length, clientRam + length);
    EXPECT_EQ(CM_SUCCESS, tlv->endLoadComposite());
    EXPECT_TRUE(memcmp(expected, clientRam, sizeof(expected)) == 0);
}


TEST_F(TlvTest, loadTruncatedSimple)
{
    //                 T       L     V ...
    uint8_t nvSet[] = {0xab,0, 4,0,  1};
    item_len_t length;

    nvram->set(nvSet, sizeof(nvSet));

    length = 4;
    tlv->startLoadSimple(0xab);
    result_t res = tlv->endLoadSimple(&length, clientRam);
    EXPECT_EQ(CM_READ_FAIL, res);
}
} // namespace

