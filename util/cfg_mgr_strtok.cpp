// Tokenization helper class.
// Unlike strtok, it's a class, and each instance keeps its own state,
// so you could have instances running concurrently.
//
#include "cfg_mgr_strtok.h"
#include <cstring> // strchr, strncmp

namespace cfg_mgr
{

// Split string into tokens, similar to strtok, but optionally defines tokens
// that start and end with special delimiters (e.g. quoted blocks).
// @return a string consisting of next word, or a string including opening and closing block marks.
//  Note that blocks must be separated by whitespace, to have a place to write null terminator.
char * Strtok::operator() (const char * word_delimiters,
                           const char * block_start,
                           const char * block_end)
{
    if ((block_start != nullptr) && (block_end == nullptr))
    {
        // If caller gives block_start but not block_end, block_start is also block_end.
        block_end = block_start;
    }
    char * start = nullptr;
    for ( ; *m_str != 0; m_str++)
    {
        TokenType type;
        if (!start)
        {
            start = get_start(word_delimiters, block_start, &type);
        }
        if (start && get_end(word_delimiters, block_end, type))
        {
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

// Return pointer to start of token if we're at start of a token.
// @param word_delimiters - in, set of characters between words
// @param block_start - in, string
// @param type - out, type of token starting.
// @return ptr to token's start, or nullptr if m_str points to a word delimiter.
// @pre start not found yet.
// @post if return non-nullptr, m_str points to location to start search for
//       token's end.
char * Strtok::get_start(const char * word_delimiters, const char * block_start, TokenType * type)
{
    if (strchr(word_delimiters, *m_str))
    {
        // Still in prefix.
        return nullptr;
    }
    // After prefix (if any), start token of type WORD or BLOCK.
    if ((block_start != nullptr) && (strncmp(block_start, m_str, strlen(block_start)) == 0))
    {
        *type = BLOCK;
        char * start = m_str;
        // Start looking for block close AFTER block open.
        m_str += strlen(block_start);
        return start;
    }
    else
    {
        *type = WORD;
        return m_str;
    }
}

// Return true if we've reached the end of a token.
// @param word_delimiters - in, set of characters between words
// @param block_end - in, string
// @param type - in, type of token we're in.
// @pre start already found.
// @post if return true, m_str is set to end of token.
bool Strtok::get_end(const char * word_delimiters, const char * block_end, TokenType type)
{
    if ((type == BLOCK) && (strncmp(block_end, m_str, strlen(block_end)) == 0))
    {
        // Token ends AFTER block end.
        m_str += strlen(block_end);
        return true;
    }
    else if ((type == WORD) && strchr(word_delimiters, *m_str))
    {
        // Token ends at word-end delimiter.
        return true;
    }
    return false;
}

}

