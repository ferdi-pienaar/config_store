// Tokenization helper class.
// Unlike strtok, it's a class, and each instance keeps its own state,
// so you could have instances running concurrently.
//
#include "config_manager_strtok.h"

namespace cfg_mgr
{

// Split string into tokens, similar to strtok, but optionally defines blocks
// that start and end with same (set of) chars as tokens (e.g. quoted blocks).
// If not in block, return a string consisting of next word.
// If in block, return a string including opening and closing block marks (e.g. quotes).
//  This means that blocks must be separated by whitespace, to have a place to write null terminator.
char * Strtok::operator() (const char * word_delimiter, const char * block_delimiter)
{
    State st = PREFIX;
    char * start;
    for ( ; *m_str != 0; m_str++)
    {
        if ((st == PREFIX) && (!is_in_set(*m_str, word_delimiter)))
        {
            // After prefix (if any), start token of type WORD or BLOCK.
            start = m_str;
            if (is_in_set(*m_str, block_delimiter))
            {
                st = BLOCK;
                // In block, start looking for block close AFTER block open.
                m_str++;
            }
            else
            {
                st = WORD;
            }
        }

        if ((st == BLOCK) && (is_in_set(*m_str, block_delimiter)))
        {
            // Token ends AFTER the closing mark of block.
            m_str++;
            break;
        }
        else if ((st == WORD) && (is_in_set(*m_str, word_delimiter)))
        {
            // Token ends at word closing delimiter.
            break;
        }
    }
    if (*m_str == 0)
    {
        // No token found: done.
        return nullptr;
    }
    // Terminate the output string at token's end.
    *m_str = 0;
    // Next time, start search from first char after this token's end.
    m_str++;
    return start;
}


bool Strtok::is_in_set(char c, const char * set)
{
    if (set == nullptr)
    {
        return false;
    }
    for (const char * d = set; *d != 0; d++)
    {
        if (c == *d)
        {
            return true;
        }
    }
    return false;
}

}

