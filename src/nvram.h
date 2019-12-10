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
    Nvram(): offset(0), fp(nullptr) {}
    bool initForWrite();
    bool initForRead();
    void accessComplete();
    bool write(const uint8_t * d, unsigned int len);
    bool read(uint8_t * d, unsigned int len);
    void setOffset(unsigned int o);
    unsigned int getOffset();
    void adjustOffset(int i);

private:
    unsigned int offset; // read/write offset relative to base address [bytes]

    FILE * fp;

};
}
#endif // NVRAM_H

