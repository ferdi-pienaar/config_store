// NVRAM implementation using a block of memory for "persistent" storage.
//

#include "nvram.h"
#include <string.h> // memset, strcmp, memcpy

#define CFG_FILE_NAME "cfg.bin"

static const unsigned int memSize = 1024;
static uint8_t nvMem[memSize];
static uint8_t nvMemClean[memSize];
static unsigned bytesSet = 0;

bool Nvram::initWrite()
{        
    offset = 0;
    return true;
}


bool Nvram::initRead()
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
    return true;
}


bool Nvram::read(uint8_t * d, unsigned int len)
{
    if (offset + len > bytesSet)
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
void nvram_spy_init()
{
    // Set pattern in nvMem, and init nvMemClean with same pattern for later comparison
    memset(nvMemClean, 0xfa, memSize);
    memcpy(nvMem, nvMemClean, memSize);
}


bool nvram_spy_match(uint8_t * expected, unsigned len)
{
    if (memcmp(expected, nvMem, len) != 0)
    {
        return false;
    }

    // Verify the remainder of nvMem is unaffected by the test
    if (memcmp(nvMem + len, nvMemClean + len, memSize - len) != 0)
    {
        return false;
    }
    return true;
}


void  nvram_spy_set(uint8_t * d, unsigned len)
{
    memcpy(nvMem, d, len);

    bytesSet = len;
}

