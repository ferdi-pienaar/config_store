
#ifndef CFG_MGR_STRTOK_H
#define CFG_MGR_STRTOK_H

#include <iostream>

namespace cfg_mgr
{

class Strtok
{
public:
    Strtok(char * str) : m_str(str), m_start(nullptr) {}
    char * operator()(const char * word_delimiters,
                      const char * block_start = nullptr,
                      const char * block_end = nullptr);

private:
    // Type of token. In one string, we can find both types.
    enum TokenType
    {
        WORD, // Token separated from others by word delimiters, e.g. spaces.
        BLOCK // Token from start-marker to end-marker, e.g. open-quote to close-quote.
    };

    // The state of the in-progress token search.
    enum State
    {
        PREFIX, // In prefix consisting of word-delimiters before token
        TOKEN, // Start of a token found but not end
        FOUND, // End of a token found
        FAIL // String terminated but no token found.
    };
    State seek_token(const char * word_delimiters,
                     const char * block_start,
                     const char * block_end);
    State seek_start(const char * word_delimiters,
                     const char * block_start,
                     TokenType * t);
    State seek_end(const char * word_delimiters,
                   const char * block_end,
                   TokenType t);
    State seek_block_end(const char * block_end);

    char * m_str; // Pointer to the next character in the string that we're analyzing.
    char * m_start; // Pointer to start of token, if any.
};

}

#endif // CFG_MGR_STRTOK_H
