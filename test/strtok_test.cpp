// Unit test using open-source unit test framework
//

#include "gtest/gtest.h"
#include "cfg_mgr_strtok.h"

namespace {

class Words : public testing::Test
{
protected:
    void SetUp()
    {
        strncpy(words, "my word\n", sizeof(words));
    }
    char words[100];
    const char * word_delimiters = " \n";
};

TEST_F(Words, getTwo)
{
    cfg_mgr::Strtok t(words);

    EXPECT_STREQ("my", t(word_delimiters));
    EXPECT_STREQ("word", t(word_delimiters));
    EXPECT_EQ(nullptr, t(word_delimiters));
}

class Quote : public testing::Test
{
protected:
    void SetUp()
    {
        strncpy(words, "Just say: \"no sir\" to them.\n", sizeof(words));
    }
    char words[100];
    const char * word_delimiters = " :.\n";
    const char * block_delimiter = "\"";
};

TEST_F(Quote, get)
{
    cfg_mgr::Strtok t(words);

    EXPECT_STREQ("Just", t(word_delimiters, block_delimiter));
    EXPECT_STREQ("say", t(word_delimiters, block_delimiter));
    EXPECT_STREQ("\"no sir\"", t(word_delimiters, block_delimiter));
    EXPECT_STREQ("to", t(word_delimiters, block_delimiter));
    EXPECT_STREQ("them", t(word_delimiters, block_delimiter));
    EXPECT_EQ(nullptr, t(word_delimiters, block_delimiter));
}

class ManyQuotes : public testing::Test
{
protected:
    void SetUp()
    {
        strncpy(words, "Say \"no sir\" \"blast\"\n", sizeof(words));
    }
    char words[100];
    const char * word_delimiters = " \n";
    const char * block_delimiter = "\"";
};

TEST_F(ManyQuotes, get)
{
    cfg_mgr::Strtok t(words);

    EXPECT_STREQ("Say", t(word_delimiters, block_delimiter));
    EXPECT_STREQ("\"no sir\"", t(word_delimiters, block_delimiter));
    EXPECT_STREQ("\"blast\"", t(word_delimiters, block_delimiter));
    EXPECT_EQ(nullptr, t(word_delimiters, block_delimiter));
}

class Braces : public testing::Test
{
protected:
    void SetUp()
    {
        strncpy(words, "outside1 {in1 in2} outside2\n", sizeof(words));
    }
    char words[100];
    const char * word_delimiters = " \n";
    const char * block_open = "{";
    const char * block_close = "}";
};

TEST_F(Braces, get)
{
    cfg_mgr::Strtok t(words);

    EXPECT_STREQ("outside1", t(word_delimiters, block_open, block_close));
    EXPECT_STREQ("{in1 in2}", t(word_delimiters, block_open, block_close));
    EXPECT_STREQ("outside2", t(word_delimiters, block_open, block_close));
    EXPECT_EQ(nullptr, t(word_delimiters, block_open, block_close));
}

class SpecialBlock : public testing::Test
{
protected:
    void SetUp()
    {
        strncpy(words, " \n out1 BEGINin1 in2BEND out2 \n ", sizeof(words));
    }
    char words[100];
    const char * word_delimiters = " \n";
    const char * block_open = "BEGIN";
    const char * block_close = "BEND";
};

TEST_F(SpecialBlock, get)
{
    cfg_mgr::Strtok t(words);

    EXPECT_STREQ("out1", t(word_delimiters, block_open, block_close));
    EXPECT_STREQ("BEGINin1 in2BEND", t(word_delimiters, block_open, block_close));
    EXPECT_STREQ("out2", t(word_delimiters, block_open, block_close));
    EXPECT_EQ(nullptr, t(word_delimiters, block_open, block_close));
}

} // namespace
