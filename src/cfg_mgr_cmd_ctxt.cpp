#include "cfg_mgr_cmd_ctxt.h"

using namespace std;

namespace cfg_mgr
{

// Add a token to the context string
void Cmd_context::add(string w)
{
    m_string += w + " ";
}


// Add an unsigned integer to the context string
void Cmd_context::add(unsigned idx)
{
    char indexbuf[6]; // xxx big enough to avoid truncation in all cases?

    snprintf(indexbuf, sizeof(indexbuf), "%d ", idx);
    m_string += indexbuf;
}

}
