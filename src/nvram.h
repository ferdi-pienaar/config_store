// Interface to non-volatile memory.
#ifndef NVRAM_H
#define NVRAM_H

#include <stdint.h> // uint8_t, etc
#include <iostream> // FILE  xxx is this really the file to include for that definition?
 

class Nvram
{
public:
    Nvram(): offset(0), fp(NULL) {}
    bool initWrite();
    bool initRead();
    void accessComplete();
    bool write(const uint8_t * d, unsigned int len);
    bool read(uint8_t * d, unsigned int len);
    void setOffset(unsigned int o);
    unsigned int getOffset();
    void adjustOffset(int i);

private:
    unsigned int offset; // read/write offset relative to base address [bytes]

    FILE * fp;  // open file for binary read

};

#endif // NVRAM_H

