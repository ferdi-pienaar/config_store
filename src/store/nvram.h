//
// Interface to non-volatile memory.
// The interface represents a file-like abstraction, where reading and writing
// are done from the current offset, and advance the offset by the number of bytes
// read or written.
//
#pragma once

#include <stdint.h> // uint8_t, etc
#include <stdio.h> // FILE
#include "nvram_itf.h"

namespace cfg_mgr
{

class Nvram : public Nvram_itf
{
public:
    Nvram(const char * fname): filename(fname), m_fp(nullptr) {}
    ~Nvram() {}
    bool initForWrite() override;
    bool initForRead() override;
    void accessComplete() override;
    bool write(const uint8_t * d, unsigned int len) override;
    bool read(uint8_t * d, unsigned int len) override;
    bool setOffset(unsigned int offset) override;
    unsigned int getOffset() override;
    bool adjustOffset(int i) override;

private:
    const char * filename = nullptr;
    FILE * m_fp;
};
}
