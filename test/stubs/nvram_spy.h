// The spy interface includes the basic interface, and additional methods.
#pragma once

#include "store/nvram_itf.h"

class Nvram_spy : public cfg_mgr::Nvram_itf
{
public:
    Nvram_spy();
    virtual ~Nvram_spy() {}
    virtual bool initForWrite() override;
    virtual bool initForRead() override;
    virtual void accessComplete() override;
    virtual bool write(const uint8_t * d, unsigned int len) override;
    virtual bool read(uint8_t * d, unsigned int len) override;
    virtual bool setOffset(unsigned int offset) override;
    virtual unsigned int getOffset() override;
    virtual bool adjustOffset(int i) override;
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
