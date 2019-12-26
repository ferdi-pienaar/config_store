
#ifndef CFG_MAN_STRTOK_H
#define CFG_MAN_STRTOK_H

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
    enum TokenType
    {
        WORD,
        BLOCK
    };
    char * get_start(const char * word_delimiters, const char * block_start, TokenType * t);
    bool get_end(const char * word_delimiters, const char * block_end, TokenType t);

    char * m_str;
};

}

#endif // CFG_MAN_STRTOK_H

