// NVRAM implementation using a file for persistent storage.
//

#include "nvram.h"

#define CFG_FILE_NAME "cfg.bin"

namespace cfg_mgr
{

bool Nvram::initForWrite()
{
    m_offset = 0;

    m_fp = fopen(CFG_FILE_NAME, "wb");  // open file for binary write

    if (m_fp == nullptr)
    {
        return false;
    }
    return true;
}


bool Nvram::initForRead()
{
    m_offset = 0;

    m_fp = fopen(CFG_FILE_NAME, "rb");  // open file for binary read

    if (m_fp == nullptr)
    {
        return false;
    }
    return true;
}


void Nvram::accessComplete()
{
    fclose(m_fp);
}


void Nvram::setOffset(unsigned int o)
{
    fseek(m_fp, o, SEEK_SET);
    m_offset = o;
}


unsigned int Nvram::getOffset()
{
    // xxx can we eliminate offset by getting this from fp?
    return m_offset;
}


void Nvram::adjustOffset(int i)
{
    fseek(m_fp, i, SEEK_CUR);
    m_offset += i;
}


//
bool Nvram::write(const uint8_t * d, unsigned int len)
{
    fwrite(d, 1, len, m_fp);

    m_offset += len;
    return true;
}


bool Nvram::read(uint8_t * d, unsigned int len)
{
    if (fread(d, len, 1, m_fp) != 1)
    {
        return false;
    }

    m_offset += len;
    return true;
}

}
