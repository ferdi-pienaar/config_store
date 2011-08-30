// The fake interface includes the basic interface, and additional methods. 
#ifndef NVRAM_FAKE_H
#define NVRAM_FAKE_H

#include "nvram.h"

void nvram_fake_clear();
uint8_t * nvram_fake_getPtr();
void      nvram_fake_set(uint8_t * d, unsigned len);

#endif // NVRAM_H

