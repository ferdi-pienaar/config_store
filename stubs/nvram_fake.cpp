// NVRAM implementation using a block of memory for "persistent" storage.
//

#include "nvram.h"
#include <string.h> // memset, strcmp, memcpy

#define CFG_FILE_NAME "cfg.bin"

static uint8_t nvMem[1024];

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
    memcpy(d, nvMem + offset, len);
    offset += len;
    return true;
}


void nvram_fake_clear()
{
    memset(nvMem, 0, sizeof(nvMem));
}


uint8_t * nvram_fake_getPtr()
{
    return nvMem;
}


void  nvram_fake_set(uint8_t * d, unsigned len)
{
    memcpy(nvMem, d, len);
}

