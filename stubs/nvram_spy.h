// The spy interface includes the basic interface, and additional methods.
#ifndef NVRAM_SPY_H
#define NVRAM_SPY_H

#include "store/nvram.h"

class Nvram_spy : public cfg_mgr::Nvram
{
public:
    Nvram_spy();
    virtual ~Nvram_spy() {}
    virtual bool initForWrite();
    virtual bool initForRead();
    virtual void accessComplete();
    virtual bool write(const uint8_t * d, unsigned int len);
    virtual bool read(uint8_t * d, unsigned int len);
    virtual bool setOffset(unsigned int offset);
    virtual unsigned int getOffset();
    virtual bool adjustOffset(int i);
    // spy methods follow.
    void init();
    bool match(uint8_t * expected, unsigned len);
    void set(uint8_t * d, unsigned len);

private:
    static const unsigned int m_memSize = 1024;
    uint8_t m_nvMem[m_memSize];
    unsigned m_bytesWritten; // max number of bytes written, i.e. the file size
    unsigned m_offset; // position in file for writing or reading.
};



#endif // NVRAM_SPY_H

