// NVRAM implementation using a block of memory for "persistent" storage.
//

#include "nvram_spy.h"
#include <cstring> // memset, strcmp, memcpy
#include <iostream>
#include <iomanip> // setw

using namespace std;

#define CFG_FILE_NAME "cfg.bin"



static void hexdump(const uint8_t * b, size_t len);

Nvram_spy::Nvram_spy() : m_bytesWritten(0), m_offset(0)
{
    memset(m_nvMem, 0, m_memSize);
}

bool Nvram_spy::initForWrite()
{
    m_offset = 0;
    m_bytesWritten = 0;
    return true;
}


bool Nvram_spy::initForRead()
{
    m_offset = 0;
    return true;
}


void Nvram_spy::accessComplete()
{

}


bool Nvram_spy::setOffset(unsigned int offset)
{
    if (m_offset > m_bytesWritten)
    {
        return false;
    }
    m_offset = offset;
    return true;
}


unsigned int Nvram_spy::getOffset()
{
    return m_offset;
}


bool Nvram_spy::adjustOffset(int i)
{
    m_offset += i;
    return true;
}


//
bool Nvram_spy::write(const uint8_t * d, unsigned int len)
{
    memcpy(m_nvMem + m_offset, d, len);
    m_offset += len;
    if (m_offset > m_bytesWritten)
    {
        m_bytesWritten = m_offset;
    }
    return true;
}


bool Nvram_spy::read(uint8_t * d, unsigned int len)
{
    if (m_offset + len > m_bytesWritten)
    {
        // Fail: attempt to read bytes that haven't been set
        return false;
    }

    memcpy(d, m_nvMem + m_offset, len);
    m_offset += len;
    return true;
}

////////////////////////////////////////////////////////////////////////////////
//
// Methods outside of NVRAM's interface, for test purposes
//
////////////////////////////////////////////////////////////////////////////////

// xxx can I obsolete this?
void Nvram_spy::init()
{
    // Set pattern in nvMem
    memset(m_nvMem, 0xfa, m_memSize);
    m_bytesWritten = 0;
}


// Compare expected contents and length of NVRAM to what has been written to it.
bool Nvram_spy::match(uint8_t * expected, unsigned len)
{
    bool ret = true;
    int first_diff_offset = -1;
    if (memcmp(expected, m_nvMem, len) != 0)
    {
        ret = false;
        for (size_t i = 0; i < len; i++)
        {
            if (expected[i] != m_nvMem[i])
            {
                first_diff_offset = i;
                break;
            }
        }
    }

    // The amount written is what's expected
    if (len != m_bytesWritten)
    {
        ret = false;
    }
    if (!ret)
    {
        cout << "actual and expected differ" << endl;
        hexdump(m_nvMem, m_bytesWritten);
        cout << endl;
        hexdump(expected, len);
        cout << endl;
        if (first_diff_offset >= 0)
        {
            for (int i = 0; i < first_diff_offset; i++)
            {
                cout << "   ";
            }
            cout << "^" << endl;
        }
    }
    return ret;
}

// Set the contents of NVRAM. This allows a test to start
// with values already present, as if saved in a file -- it allows us
// to simulate the non-volatility of NVRAM.
// xxx really need this? -- how does it differ from write?
void Nvram_spy::set(uint8_t * d, unsigned len)
{
    memcpy(m_nvMem, d, len);
    m_bytesWritten = len;
}

static void hexdump(const uint8_t * b, size_t len)
{
    for (size_t i = 0; i < len; i++)
    {
        cout << setfill('0') << setw(2) << hex << (int)b[i] << " ";
    }
}
