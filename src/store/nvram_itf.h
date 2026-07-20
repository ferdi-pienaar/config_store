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

class Nvram_itf
{
public:
    virtual bool initForWrite() = 0;
    virtual bool initForRead() = 0;
    virtual void accessComplete() = 0;
    virtual bool write(const uint8_t * d, unsigned int len) = 0;
    virtual bool read(uint8_t * d, unsigned int len) = 0;
    virtual bool setOffset(unsigned int offset) = 0;
    virtual unsigned int getOffset() = 0;
    virtual bool adjustOffset(int i) = 0;
};
}
