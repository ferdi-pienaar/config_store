
#ifndef CFG_MAN_STRTOK_H
#define CFG_MAN_STRTOK_H

#include <iostream>

namespace cfg_mgr
{

class Strtok
{
public:
    Strtok(char * str) : m_str(str) {}
    char * operator()(const char * word_delimiter, const char * block_delimiter = nullptr);

private:
    enum State
    {
        PREFIX,
        WORD,
        BLOCK
    };
    bool is_in_set(char c, const char * set);

    char * m_str;
};

}

#endif // CFG_MAN_STRTOK_H

