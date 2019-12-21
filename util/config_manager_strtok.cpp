// Tokenization assistant.
//
#include "config_manager_strtok.h"

namespace cfg_mgr
{
// Split string into tokens, similar to strtok.
// If not in quote, return a string consisting of next word.
// If quoted block, return a string including opening and closing quotes.
//  This means that quoted blocks must be separated by whitespace.
char * cm_strtok (char * str)
{
    static char * lstr = nullptr;
    if (str != nullptr)
    {
        lstr = str;
    }

    char * start = lstr;
    bool in_quote = false;
    for (; *lstr != 0; lstr++)
    {
        if (!in_quote)
        {
            if ((*lstr == ' ') || (*lstr == '\t') || (*lstr == '\n'))
            {
                // End of word.
                break;
            }
            else if (*lstr == '\"')
            {
                // Start of quote: now look for its end.
                in_quote = true;
            }
        }
        else
        {
            // In quote: look for closing quote.
            if (*lstr == '\"')
            {
                // Token ends AFTER the closing quote.
                lstr++;
                break;
            }
        }
    }
    if (*lstr == 0)
    {
        // No token found: done.
        return nullptr;
    }
    // Terminate the output string at token's end
    *lstr = 0;
    // Next time, start search from first char after this token's end.
    lstr++;
    return start;
}

}

