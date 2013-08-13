// NVRAM implementation using a file for persistent storage.
//

#include "nvram.h"

#define CFG_FILE_NAME "cfg.bin"

bool Nvram::initForWrite()
{        
    offset = 0;

    fp = fopen(CFG_FILE_NAME, "wb");  // open file for binary write

    if (fp == NULL)
    {
        return false;
    }
    return true;
}


bool Nvram::initForRead()
{
    offset = 0;

    fp = fopen(CFG_FILE_NAME, "rb");  // open file for binary read

    if (fp == NULL)
    {
        return false;
    }
    return true;
}


void Nvram::accessComplete()
{
    fclose(fp);
}


void Nvram::setOffset(unsigned int o)
{
    fseek(fp, o, SEEK_SET);
    offset = o;
}


unsigned int Nvram::getOffset()
{
    // xxx can we eliminate offset by getting this from fp?
    return offset;
}


void Nvram::adjustOffset(int i)
{
    fseek(fp, i, SEEK_CUR);
    offset += i;
}


//
bool Nvram::write(const uint8_t * d, unsigned int len)
{
    fwrite(d, 1, len, fp);
    
    offset += len;
    return true;
}


bool Nvram::read(uint8_t * d, unsigned int len)
{
    if (fread(d, len, 1, fp) != 1)
    {
        return false;
    }
    
    offset += len;
    return true;
}


