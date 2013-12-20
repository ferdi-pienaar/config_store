// NVRAM implementation using a block of memory for "persistent" storage.
//

#include "nvram_spy.h"
#include <string.h> // memset, strcmp, memcpy

#define CFG_FILE_NAME "cfg.bin"

static const unsigned int memSize = 1024;
static uint8_t nvMem[memSize];
static unsigned bytesWritten = 0; // max number of bytes written, i.e. the file size

bool Nvram::initForWrite()
{        
    offset = 0;
    bytesWritten = 0;
    return true;
}


bool Nvram::initForRead()
{
    offset = 0;
    return true;
}


void Nvram::accessComplete()
{

}


void Nvram::setOffset(unsigned int o)
{
    offset = o;
}


unsigned int Nvram::getOffset()
{
    return offset;
}


void Nvram::adjustOffset(int i)
{
    offset += i;
}


//
bool Nvram::write(const uint8_t * d, unsigned int len)
{
    memcpy(nvMem + offset, d, len);
    offset += len;
    if (offset > bytesWritten)
    {
        bytesWritten = offset;
    }
    return true;
}


bool Nvram::read(uint8_t * d, unsigned int len)
{
    if (offset + len > bytesWritten)
    {
        // Fail: attempt to read bytes that haven't been set
        return false;
    }

    memcpy(d, nvMem + offset, len);
    offset += len;

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
    if (memcmp(expected, nvMem, len) != 0)
    {
        return false;
    }

    // The amount written is what's expected
    if (len != bytesWritten)
    {
        return false;
    }
    return true;
}

// Set the contents of NVRAM. This allows a test to start
// with values already present, as if saved in a file -- it allows us
// to simulate the non-volatility of NVRAM.
void nvram_spy_set(uint8_t * d, unsigned len)
{
    memcpy(nvMem, d, len);
    bytesWritten = len;
}

