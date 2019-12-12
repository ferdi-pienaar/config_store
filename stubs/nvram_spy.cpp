// NVRAM implementation using a block of memory for "persistent" storage.
//

#include "nvram_spy.h"
#include <string.h> // memset, strcmp, memcpy
#include <iostream>
#include <iomanip> // setw

using namespace std;

#define CFG_FILE_NAME "cfg.bin"

static const unsigned int memSize = 1024;
static uint8_t nvMem[memSize];
static unsigned bytesWritten = 0; // max number of bytes written, i.e. the file size

static void hexdump(const uint8_t * b, size_t len);

bool cfg_mgr::Nvram::initForWrite()
{
    m_offset = 0;
    bytesWritten = 0;
    return true;
}


bool cfg_mgr::Nvram::initForRead()
{
    m_offset = 0;
    return true;
}


void cfg_mgr::Nvram::accessComplete()
{

}


void cfg_mgr::Nvram::setOffset(unsigned int o)
{
    m_offset = o;
}


unsigned int cfg_mgr::Nvram::getOffset()
{
    return m_offset;
}


void cfg_mgr::Nvram::adjustOffset(int i)
{
    m_offset += i;
}


//
bool cfg_mgr::Nvram::write(const uint8_t * d, unsigned int len)
{
    memcpy(nvMem + m_offset, d, len);
    m_offset += len;
    if (m_offset > bytesWritten)
    {
        bytesWritten = m_offset;
    }
    return true;
}


bool cfg_mgr::Nvram::read(uint8_t * d, unsigned int len)
{
    if (m_offset + len > bytesWritten)
    {
        // Fail: attempt to read bytes that haven't been set
        return false;
    }

    memcpy(d, nvMem + m_offset, len);
    m_offset += len;

    return true;
}

////////////////////////////////////////////////////////////////////////////////
//
// Methods outside of NVRAM's interface, for test purposes
//
////////////////////////////////////////////////////////////////////////////////

// xxx can I obsolete this?
void nvram_spy_init()
{
    // Set pattern in nvMem
    memset(nvMem, 0xfa, memSize);
    bytesWritten = 0;
}


// Compare expected contents and length of NVRAM to what has been written to it.
bool nvram_spy_match(uint8_t * expected, unsigned len)
{
    bool ret = true;
    int first_diff_offset = -1;
    if (memcmp(expected, nvMem, len) != 0)
    {
        ret = false;
        for (size_t i = 0; i < len; i++)
        {
            if (expected[i] != nvMem[i])
            {
                first_diff_offset = i;
                break;
            }
        }
    }

    // The amount written is what's expected
    if (len != bytesWritten)
    {
        ret = false;
    }
    if (!ret)
    {
        cout << "actual and expected differ" << endl;
        hexdump(nvMem, bytesWritten);
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
void nvram_spy_set(uint8_t * d, unsigned len)
{
    memcpy(nvMem, d, len);
    bytesWritten = len;
}

static void hexdump(const uint8_t * b, size_t len)
{
    for (size_t i = 0; i < len; i++)
    {
        cout << setfill('0') << setw(2) << hex << (int)b[i] << " ";
    }
}
