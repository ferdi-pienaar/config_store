
#ifndef CFG_MGR_STRTOK_H
#define CFG_MGR_STRTOK_H

#include <iostream>

namespace cfg_mgr
{

class Strtok
{
public:
    Strtok(char * str) : m_str(str) {}
    char * operator()(const char * word_delimiters,
                      const char * block_start = nullptr,
                      const char * block_end = nullptr);

private:
    // Type of token. In one string, we can find both types.
    enum TokenType
    {
        WORD, // Token separated from others by word delimiters, e.g. spaces.
        BLOCK // Token from start marker to end marker, e.g. open-quote to close-quote.
    };
    char * get_start(const char * word_delimiters, const char * block_start, TokenType * t);
    bool get_end(const char * word_delimiters, const char * block_end, TokenType t);

    char * m_str; // Pointer to the next character in the string that we're analyzing.
};

}

#endif // CFG_MGR_STRTOK_H

