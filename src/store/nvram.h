//
// Interface to non-volatile memory.
// The interface represents a file-like abstraction, where reading and writing
// are done from the current offset, and advance the offset by the number of bytes
// read or written.
//
#pragma once

#include <stdint.h> // uint8_t, etc
#include <stdio.h> // FILE

namespace cfg_mgr
{

class Nvram
{
public:
    Nvram(): m_fp(nullptr) {}
    virtual ~Nvram() {}
    virtual bool initForWrite(const char * filename);
    virtual bool initForRead(const char * filename);
    virtual void accessComplete();
    virtual bool write(const uint8_t * d, unsigned int len);
    virtual bool read(uint8_t * d, unsigned int len);
    virtual bool setOffset(unsigned int offset);
    virtual unsigned int getOffset();
    virtual bool adjustOffset(int i);

private:
    FILE * m_fp;
};
}
