//
// Item metadata, i.e. information about the configurable items that are kept in RAM.
// We place metadata in structures separate from the descriptor classes that own them,
// to make it easy to ensure this constant data is ROMable (the rules for ensuring
// a class object is ROMable, are very restrictive).
// This comes at the cost of having an additional level of indirection.

#pragma once

#include <stdint.h> // uint8_t, etc
#include <string>
#include "cfg_mgr_types.h"
#include "metadata_common.h" // Common to Simple_metadata and Composite_metadata.

namespace cfg_mgr
{

// Function pointers -- types registered by user when descriptor is created
// Convert string into item of len bytes.
typedef bool (*CM_SET_FPTR)(uint8_t *pItem, item_len_t len, std::string val, PRINTF_FN_TYPE);
// Set item of len bytes to a default value defined in the function.
typedef void (*CM_SETDEF_FPTR)(uint8_t *pItem, item_len_t len);
// Convert item of len bytes into a string.
typedef std::string (*CM_PRT_FPTR)(const uint8_t *pItem, item_len_t len);

// class Simple_descriptor has a pointer to an object of this type.
struct Simple_metadata
{
    const Common_metadata c;

    // Populate the following ptrs when creating a descriptor.
    // The config manager's utils provide a set of such functions
    // for basic types of configurable items, with 'C' linkage.
    //
    const CM_SET_FPTR    pSet;
    const CM_SETDEF_FPTR pSetDefault;
    const CM_PRT_FPTR    pPrt;
};

class Aggregate;

// class Composite_descriptor has a pointer to an object of this type.
struct Composite_metadata
{
    Common_metadata c;

    // Information about the components of the composite
    Aggregate * const * aggrList; // Array of pointers to aggregates (pointers, because abstract Aggregate can't be instantiated)
    unsigned short aggrCount; // Number of aggregates in the list (number of descriptors, not items)
};

class Descriptor;

// class Aggregate has a pointer to an object of this type.
struct Aggregate_data
{
    Descriptor * const pDesc; ///< the component's descriptor
    const unsigned short maxCount; ///< Max number of instances of the item
    const unsigned int offset; ///< Offset [bytes] of items, or pointer to items, within the composite item
};

class Contained_aggregate;

struct Owned_aggregate_data
{
    Aggregate_data a;
    Contained_aggregate * m_counterAggr; // the counter for this owned component
};

}
