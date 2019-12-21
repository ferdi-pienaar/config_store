//
// Interface to non-volatile memory.
// The interface represents a file-like abstraction, where reading and writing
// are done from the current offset, and advance the offset by the number of bytes
// read or written.
//
#ifndef NVRAM_H
#define NVRAM_H

#include <stdint.h> // uint8_t, etc
#include <stdio.h> // FILE

namespace cfg_mgr
{

class Nvram
{
public:
    Nvram(): m_fp(nullptr) {}
    bool initForWrite();
    bool initForRead();
    void accessComplete();
    bool write(const uint8_t * d, unsigned int len);
    bool read(uint8_t * d, unsigned int len);
    bool setOffset(unsigned int offset);
    unsigned int getOffset();
    bool adjustOffset(int i);

private:
    FILE * m_fp;
};
}
#endif // NVRAM_H

