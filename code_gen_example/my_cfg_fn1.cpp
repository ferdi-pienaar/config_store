// Functions implemented by the application programmer, to extend
// the features provided by the config_manager.
//

#include <iostream>
#include <stdlib.h> // strtod
#include <cstring> // strlen
#include <climits> // SHRT_MIN, SHRT_MAX
#include <math.h> // round
#include "cfg_mgr.h"
#include "my_cfg_fn1.h"
#include "cfg_mgr_printf.h"
#include <sstream>

using namespace std;
using namespace cfg_mgr;

// convert temperature to internal integer representation
static long int d2internal(double t)
{
    return round((t - 50) * 250);
}

// convert internal short int to temperature
// The range is 2^16/250 = 262.144 degrees.
static float internal2t(short int i)
{
    return i/250.0 + 50;
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


//
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
        cm_printf("Not a valid temperature: %s.\n", c_string);
        return false;
    }

    long int internal_representation = d2internal(t);
    if (internal_representation > SHRT_MAX)
    {
        cm_printf("Limiting %f to max %.3f.\n", t, internal2t(SHRT_MAX));
        internal_representation = SHRT_MAX;
    }
    else if (internal_representation < SHRT_MIN)
    {
        cm_printf("Limiting %f to min %.3f.\n", t, internal2t(SHRT_MIN));
        internal_representation = SHRT_MIN;
    }
    *((short *)pItem) = (short)internal_representation;
    return true;
}
