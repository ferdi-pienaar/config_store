#pragma once

#include "cfg_mgr_json_common.h"

namespace cfg_mgr
{

// Stack used in writing and loading JSON.
class ContextStack
{
public:
    ContextStack()
    {
        init();
    }

    void init();
    void push(ValueType t);
    void pop();

    ValueType getType()
    {
        return m_stack[m_index].m_type;
    }

    void setIsFirstMember(bool first)
    {
        m_stack[m_index].m_isFirstMember = first;
    }

    bool isFirstMember()
    {
        return m_stack[m_index].m_isFirstMember;
    }

    unsigned getIndex()
    {
        return m_index;
    }

private:
    static const unsigned int STACK_DEPTH = 16;

    // Context maintained for reading/writing a Value that is an Object or Array.
    struct Context
    {
        Context(): m_type(OBJECT), m_isFirstMember(true) {}

        ValueType m_type;
        bool m_isFirstMember; // Are we writing/loading the first member of a list or object?
    };

    Context m_stack[STACK_DEPTH];
    unsigned m_index;
};
}
