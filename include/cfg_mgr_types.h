// Common types needed in all files
// xxx This differs from config_manager.h in that... It's not for export?
#pragma once

#include <stdint.h> // uint8_t, etc

namespace cfg_mgr
{

class Store;

using PRINTF_FN_TYPE = int(*)(const char* fmt, ...);

// Number of bytes in an item; used in NVRAM
// Because it determines the longest possible length of any item in NVRAM,
// it's also big enough to be used for the length of items in RAM
// (which are shorter, as the exclude the Id and Length fields saved to NVRAM).
typedef uint16_t item_len_t;

// Identifier ID, unique within its context, used to identify it in NVRFAM
typedef uint16_t item_id_t;

//
enum class Result
{
    CM_SUCCESS,
    CM_READ_FAIL,
    CM_INCOHERENT_DATA,
    CM_NOT_FOUND,
    CM_FAIL
};

static inline const char * ResultString(Result r)
{
    if (r == Result::CM_SUCCESS) return "CM_SUCCESS";
    if (r == Result::CM_READ_FAIL) return "CM_READ_FAIL";
    if (r == Result::CM_INCOHERENT_DATA) return "CM_INCOHERENT_DATA";
    if (r == Result::CM_NOT_FOUND) return "CM_NOT_FOUND";
    if (r == Result::CM_FAIL) return "CM_FAIL";
    return "Invalid result value!";
}

// services that manager provides to the descriptors that it manages.
struct Mgr_service
{
    PRINTF_FN_TYPE m_print;
    Store * m_store;
};

}
