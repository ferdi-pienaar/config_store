// The spy interface includes the basic interface, and additional methods.
#ifndef NVRAM_SPY_H
#define NVRAM_SPY_H

#include "nvram.h"

void nvram_spy_init();
bool nvram_spy_match(uint8_t * expected, unsigned len);
void nvram_spy_set(uint8_t * d, unsigned len);

#endif // NVRAM_SPY_H

