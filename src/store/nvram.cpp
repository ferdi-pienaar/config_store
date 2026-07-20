// NVRAM implementation using a file for persistent storage.
//

#include "nvram.h"

namespace cfg_mgr
{

bool Nvram::initForWrite()
{
    m_fp = fopen(filename, "wb");  // open file for binary write
    return m_fp != nullptr;
}


bool Nvram::initForRead()
{
    m_fp = fopen(filename, "rb");  // open file for binary read
    return m_fp != nullptr;
}


void Nvram::accessComplete()
{
    fclose(m_fp);
}


bool Nvram::setOffset(unsigned int offset)
{
    return fseek(m_fp, offset, SEEK_SET) == 0;
}


unsigned int Nvram::getOffset()
{
    return ftell(m_fp);
}


bool Nvram::adjustOffset(int i)
{
    return fseek(m_fp, i, SEEK_CUR) == 0;
}


//
bool Nvram::write(const uint8_t * d, unsigned int len)
{
    size_t wlen = fwrite(d, 1, len, m_fp);
    return wlen == len;
}


bool Nvram::read(uint8_t * d, unsigned int len)
{
    return fread(d, len, 1, m_fp) == 1;
}

}
