///
//

#include "cfg_mgr_json_ctxt.h"
#include <assert.h>

using namespace std;

namespace cfg_mgr
{

void ContextStack::init()
{
    m_index = 0;
    m_stack[m_index].m_type = OBJECT;
    m_stack[m_index].m_isFirstMember = true;
}

void ContextStack::push(ValueType t)
{
    m_index++;
    assert(m_index < STACK_DEPTH);
    m_stack[m_index].m_type = t;
    m_stack[m_index].m_isFirstMember = true;
}

void ContextStack::pop()
{
    assert(m_index > 0);
    m_index--;
}

}
