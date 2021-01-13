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
    if (block_start && !block_end)
    {
        // Caller gave block_start but not block_end: use block_start for both.
        block_end = block_start;
    }
    State state = seek_token(word_delimiters, block_start, block_end);
    if (state == FAIL)
    {
        return nullptr;
    }
    // Terminate the output string at token's end.
    *(m_str - 1) = 0;
    return m_start;
}

// Search the remainder of the string for the next token.
// @return FOUND if a token was found. m_start points to the token
//          and m_str points to the character after the token.
//         FAIL if string terminator reached without a token.
Strtok::State Strtok::seek_token(const char * word_delimiters,
                                 const char * block_start,
                                 const char * block_end)
{
    State state = PREFIX;
    for ( ; state < FOUND; m_str++)
    {
        TokenType type;
        if (state == PREFIX)
        {
            state = seek_start(word_delimiters, block_start, &type);
        }
        else if (state == TOKEN)
        {
            state = seek_end(word_delimiters, block_end, type);
        }
    }
    return state;
}

// Set pointer to start of token if we're at start of a token.
// @param word_delimiters - in, set of characters between words
// @param block_start - in, string
// @param type - out, type of token starting.
// @return state.
// @pre state == SEEK.
// @return PREFIX if still in prefix consisting of word-delimiters
//         TOKEN (m_start is set and m_str advanced past block start if type is BLOCK)
//         FAIL if string terminator reached without finding token start.
Strtok::State Strtok::seek_start(const char * word_delimiters,
                                 const char * block_start,
                                 TokenType * type)
{
    if (*m_str == 0)
    {
        return FAIL;
    }
    if (strchr(word_delimiters, *m_str))
    {
        return PREFIX;
    }
    // After prefix (if any), start token of type WORD or BLOCK.
    m_start = m_str;
    *type = WORD; // the default type is WORD.
    if (block_start && (strncmp(block_start, m_str, strlen(block_start)) == 0))
    {
        *type = BLOCK;
        // Start looking for block close AFTER block open.
        m_str += strlen(block_start);
    }
    return TOKEN;
}

// Look for end of a token or the end of the string.
// @param word_delimiters - in, set of characters between words
// @param block_end - in, string
// @param type - in, type of token we're in.
// @pre state == TOKEN.
// @return TOKEN (still in token)
//         FOUND or
//         FAIL if string terminator reached before end of block-type token.
Strtok::State Strtok::seek_end(const char * word_delimiters,
                               const char * block_end,
                               TokenType type)
{
    if (type == BLOCK)
    {
        return seek_block_end(block_end);
    }
    // Word ends at string terminator or word delimiter.
    if ((*m_str == 0) || strchr(word_delimiters, *m_str))
    {
        return FOUND;
    }
    return TOKEN;
}

Strtok::State Strtok::seek_block_end(const char * block_end)
{
    if (*m_str == 0)
    {
        // Reached string terminator without finding block end.
        return FAIL;
    }
    unsigned block_end_len = strlen(block_end);
    if (strncmp(block_end, m_str, block_end_len) == 0)
    {
        // Token ends AFTER block end -- string may terminate too.
        m_str += block_end_len;
        return FOUND;
    }
    return TOKEN;
}

}
