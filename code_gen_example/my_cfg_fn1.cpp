// Functions implemented by the application programmer, to extend
// the features provided by the config_manager.
// xxx client should use same print function that they pass to
// to cfg_mgr to do printing here; for now, it is just printf.

#include <iostream>
#include <stdlib.h> // strtod
#include <cstring> // strlen
#include <climits> // SHRT_MIN, SHRT_MAX
#include <math.h> // round
#include "cfg_mgr_types.h"
#include "my_cfg_fn1.h"
#include <sstream>
#include <assert.h>

using namespace std;
using namespace cfg_mgr;

constexpr double TEMP_MIN = -50;
constexpr double TEMP_MAX = 160;
constexpr double SCALE =  (SHRT_MAX - SHRT_MIN) / (TEMP_MAX - TEMP_MIN);

// Convert temperature to internal integer representation.
// TEMP_MAX is represented as 32767.
// TEMP_MIN is represented as -32768.
static long int d2internal(double t)
{
    return round((t - TEMP_MIN) * SCALE + SHRT_MIN);
}

// Convert internal short int representation to temperature.
static double internal2t(short int i)
{
    return ((i - SHRT_MIN) / SCALE + TEMP_MIN);
}

// Use short to represent temperatures
void setdef_temp(uint8_t *pItem, item_len_t len)
{
    // Sanity check
    assert(len == sizeof(short));

    *((short *)pItem) = (short)d2internal(37.11);
}

//
// Use short to represent temperatures
string prt_temp(const uint8_t *pItem, item_len_t len)
{
    // Sanity check
    assert(len == sizeof(short));

    stringstream ss;
    ss << internal2t(*((short *)pItem));
    return ss.str();
}


// Use short to represent temperatures.
// Using a double as an intermediate step makes it easier,
// but maybe has some corner cases?
bool set_temp(uint8_t *pItem, item_len_t len, string val)
{
    const char * c_string = val.c_str();
    char * pEnd; // pointer to char after chars accepted by strtod
    double t = strtod(c_string, &pEnd);
    if (pEnd != c_string + strlen(c_string))
    {
        // Input string could not be fully converted to double
        printf("Not a valid temperature: %s.\n", c_string);
        return false;
    }

    long int internal_representation = d2internal(t);
    if (internal_representation > SHRT_MAX)
    {
        printf("Limiting %f to max %.3f.\n", t, TEMP_MAX);
        internal_representation = SHRT_MAX;
    }
    else if (internal_representation < SHRT_MIN)
    {
        printf("Limiting %f to min %.3f.\n", t, TEMP_MIN);
        internal_representation = SHRT_MIN;
    }
    *((short *)pItem) = (short)internal_representation;
    return true;
}
