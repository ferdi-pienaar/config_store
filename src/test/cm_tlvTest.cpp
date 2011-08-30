// Unit test using open-source unit test framework
//

#include "CppUTest/TestHarness.h"
#include "CppUTest/CommandLineTestRunner.h"
#include "config_manager_tlv.h"  // Unit under test
#include <string.h> // strncmp
#include "nvram_fake.h"

#include <iostream>
using namespace std;

static uint8_t clientRam[1024];

int main(int argc, char** argv)
{
    return RUN_ALL_TESTS(argc, argv);
}


TEST_GROUP(tlv)
{
    Tlv tlv;

    void setup()
    {
        nvram_fake_clear();
    }
    
    void teardown()
    {
        //clean up steps are executed after each TEST
    }
};


// xxx valid for little-endian systems only
TEST(tlv, writeSimple)
{
    cm_item_id_t id = 55;

    uint8_t mem[] = {1,2,3,4};
    //                    T     L              V
    uint8_t expected[] = {55,0, sizeof(mem),0, 1,2,3,4};

    tlv.writeSimple(id, sizeof(mem), mem);

    CHECK(memcmp(expected, nvram_fake_getPtr(), sizeof(expected)) == 0);
}


// xxx valid for little-endian systems only
TEST(tlv, writeComposite)
{
    cm_item_id_t cId = 55;
    cm_item_id_t sId = 44;

    uint8_t s1[] = {1,2,3,4};
    uint8_t s2[] = {11,22,33,44};
    //                    T     L     T     L     V         T      L     V
    uint8_t expected[] = {55,0, 16,0, 44,0, 4,0,  1,2,3,4,  44,0,  4,0,  11,22,33,44};

    tlv.startWriteComposite(cId);
    tlv.writeSimple(sId, sizeof(s1), s1);
    tlv.writeSimple(sId, sizeof(s2), s2);
    tlv.endWriteComposite();

    //cout << memcmp(expected, nvMem, sizeof(expected));

    CHECK(memcmp(expected, nvram_fake_getPtr(), sizeof(expected)) == 0);
}


// xxx valid for little-endian systems only
TEST(tlv, writeNestedComposite)
{
    cm_item_id_t id = 0xab; // All items are at different levels, so they can share an ID

    uint8_t s1[] = {1,2,3,4,5,6};
    //                    T       L     T       L      T       L     V
    uint8_t expected[] = {0xab,0, 14,0, 0xab,0, 10,0,  0xab,0, 6,0,  1,2,3,4,5,6};
    
    tlv.startWriteComposite(id);
    tlv.startWriteComposite(id);
    tlv.writeSimple(id, sizeof(s1), s1);
    tlv.endWriteComposite();
    tlv.endWriteComposite();

    CHECK(memcmp(expected, nvram_fake_getPtr(), sizeof(expected)) == 0);
}


// xxx valid for little-endian systems only
TEST(tlv, writeNestedCompositeAndSimple)
{
    uint8_t s1[] = {1,2,3,4,5,6};
    uint8_t s2[] = {10,11};
    //                    T       L     T       L      T       L     V            T       L    V
    uint8_t expected[] = {0xab,0, 20,0, 0xab,0, 10,0,  0xab,0, 6,0,  1,2,3,4,5,6, 0xbc,0, 2,0, 10,11};
    
    tlv.startWriteComposite(0xab);
    tlv.startWriteComposite(0xab);
    tlv.writeSimple(0xab, sizeof(s1), s1);
    tlv.endWriteComposite();
    tlv.writeSimple(0xbc, sizeof(s2), s2);
    tlv.endWriteComposite();

    CHECK(memcmp(expected, nvram_fake_getPtr(), sizeof(expected)) == 0);
}


// xxx valid for little-endian systems only
TEST(tlv, loadSimple)
{
    //                   T        L    V
    uint8_t    nvSet[] = {0xab,0, 2,0, 55,0};
    uint8_t    expected[] = {55,0};
    cm_item_len_t length = sizeof(expected);


    // xxx set client RAM to bitpattern and verify only the expected section is modified
    

    // xxx Create a function for setting NVRAM via a backdoor.
    // What kind of test double is this?  Spy? Fake?
    nvram_fake_set(nvSet, sizeof(nvSet));

    cm_item_id_t id;
    tlv.getType(&id);
    LONGS_EQUAL(0xab, id);

    unsigned complete = 0;
    tlv.loadSimple(clientRam, &length, &complete);

    CHECK(memcmp(expected, clientRam, sizeof(expected)) == 0);
}


// xxx valid for little-endian systems only
TEST(tlv, loadComposite)
{
    //                    T       L     T    L    V     T    L    V
    uint8_t    nvSet[] = {0xab,0, 12,0, 4,0, 2,0, 55,0, 4,0, 2,0, 66,0};
    uint8_t    expected[] = {55,0, 66,0};
    cm_item_id_t id;
    cm_item_len_t length;
    unsigned complete = 0;


    // xxx set client RAM to bitpattern and verify only the expected section is modified
    nvram_fake_set(nvSet, sizeof(nvSet));

    tlv.getType(&id);
    LONGS_EQUAL(0xab, id);

    tlv.loadComposite();

    tlv.getType(&id);
    LONGS_EQUAL(4, id);

    length = 2;
    tlv.loadSimple(clientRam, &length, &complete);
    LONGS_EQUAL(2, length);
    LONGS_EQUAL(0, complete);

    tlv.getType(&id);
    LONGS_EQUAL(4, id);

    length = 2;
    tlv.loadSimple(clientRam + length, &length, &complete);
    LONGS_EQUAL(1, complete); 

    CHECK(memcmp(expected, clientRam, sizeof(expected)) == 0);
}


// xxx valid for little-endian systems only
// xxx incomplete
TEST(tlv, loadNestedComposite)
{
    //                    T       L     T    L    V    T    L        V
    uint8_t    nvSet[] = {0xab,0, 14,0, 0xab,0, 10,0,  0xab,0, 6,0,  1,2,3,4,5,6};
    uint8_t    expected[] = {1,2,3,4,5,6};
    cm_item_id_t id;
    cm_item_len_t length = sizeof(expected);
    unsigned complete = 0;


    // xxx set client RAM to bitpattern and verify only the expected section is modified
    
    nvram_fake_set(nvSet, sizeof(nvSet));

    tlv.getType(&id);
    LONGS_EQUAL(0xab, id);

    tlv.loadComposite();

    tlv.getType(&id);
    LONGS_EQUAL(0xab, id);

    tlv.loadComposite();

    tlv.getType(&id);
    LONGS_EQUAL(0xab, id);

    tlv.loadSimple(clientRam, &length, &complete);
    LONGS_EQUAL(2, complete); // this completes 2 containers

    CHECK(memcmp(expected, clientRam, sizeof(expected)) == 0);
}


// xxx valid for little-endian systems only
TEST(tlv, loadNestedCompositeAndSimple)
{
    //                 T       L     T       L      T       L     V            T       L    V
    uint8_t nvSet[] = {0xab,0, 20,0, 0xab,0, 10,0,  0xab,0, 6,0,  1,2,3,4,5,6, 0xbc,0, 2,0, 10,11};
    uint8_t expected[] = {1,2,3,4,5,6,10,11};
    cm_item_id_t id;
    cm_item_len_t length;
    unsigned complete = 0;

    nvram_fake_set(nvSet, sizeof(nvSet));
    
    tlv.getType(&id);
    LONGS_EQUAL(0xab, id);

    tlv.loadComposite();

    tlv.getType(&id);
    LONGS_EQUAL(0xab, id);

    tlv.loadComposite();

    tlv.getType(&id);
    LONGS_EQUAL(0xab, id);

    length = 6;
    tlv.loadSimple(clientRam, &length, &complete);
    LONGS_EQUAL(1, complete); // complete inner container

    tlv.getType(&id);
    LONGS_EQUAL(0xbc, id);

    length = 2;
    complete = 0;
    tlv.loadSimple(clientRam + 6, &length, &complete);
    LONGS_EQUAL(1, complete); // complete top-level container

    CHECK(memcmp(expected, clientRam, sizeof(expected)) == 0);
}


// xxx valid for little-endian systems only
TEST(tlv, loadNestedCompositeOf2Simples)
{
    //                 T       L     T       L      T       L     V            T       L    V
    uint8_t nvSet[] = {0xab,0, 20,0, 0xab,0, 16,0,  0xab,0, 6,0,  1,2,3,4,5,6, 0xbc,0, 2,0, 10,11};
    uint8_t expected[] = {1,2,3,4,5,6,10,11};
    cm_item_id_t id;
    cm_item_len_t length;
    unsigned complete = 0;

    nvram_fake_set(nvSet, sizeof(nvSet));
    
    tlv.getType(&id);
    LONGS_EQUAL(0xab, id);

    tlv.loadComposite();

    tlv.getType(&id);
    LONGS_EQUAL(0xab, id);

    tlv.loadComposite();

    tlv.getType(&id);
    LONGS_EQUAL(0xab, id);

    length = 6;
    tlv.loadSimple(clientRam, &length, &complete);
    LONGS_EQUAL(0, complete); // not the end of its container


    tlv.getType(&id);
    LONGS_EQUAL(0xbc, id);

    length = 2;
    tlv.loadSimple(clientRam + 6, &length, &complete);
    LONGS_EQUAL(2, complete); // complete both containers

    CHECK(memcmp(expected, clientRam, sizeof(expected)) == 0);
}

