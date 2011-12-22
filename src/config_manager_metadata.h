//
// Item metadata, i.e. information about the configurable items that are kept in RAM.
// We place metadata in a structures separate from the descriptor classes,
// to make it easy to ensure this constant data is ROMable (the rules for ensuring
// a class object is ROMable, are very restrictive).
// This comes at the cost of having an additional level of indirection.

#ifndef CFG_MAN_METADATA_H
#define CFG_MAN_METADATA_H

#include <stdint.h> // uint8_t, etc
#include <string>
#include "config_manager_types.h"


// Function pointers -- types registered by user when descriptor is created
typedef bool (*CM_SET_FPTR)(uint8_t *pItem, cm_item_len_t len, std::string val);
typedef void (*CM_SETDEF_FPTR)(uint8_t *pItem, cm_item_len_t len);
typedef void (*CM_PRT_FPTR)(FILE * f, const uint8_t *pItem, cm_item_len_t len);


struct cm_common_metadata
{
    const char * const  name;       ///< name by which item is addressed on CLI    
    const cm_item_id_t  id;         ///< ID (unique within the context of the component's composite) of item in NVRAM
    const cm_item_len_t len;        ///< Number of bytes occupied by an item in RAM
    const bool          persistent; ///< Saved to NVRAM?
};

struct cm_simple_metadata
{
    const cm_common_metadata c;

    // Populate the following ptrs when creating a descriptor.
    // The config manager's extensions provide a set of such functions
    // for basic types of configurable items, with 'C' linkage.
    //
    const CM_SET_FPTR    pSet;
    const CM_SETDEF_FPTR pSetDefault;
    const CM_PRT_FPTR    pPrt;
};

class cm_aggregate;

struct cm_composite_metadata
{
    const cm_common_metadata c;

    // Information about the components of the composite
    const cm_aggregate * const * const aggrList;  // Array of pointers to aggregates (pointers, because abstract cm_aggregate can't be instantiated)
    const unsigned short               aggrCount; // Number of aggregates in the list (number of descriptors, not items)

};

class cm_descriptor;

struct cm_aggregate_data
{    
    const cm_descriptor * const pDesc; ///< the component's descriptor
    const unsigned short  maxCount;    ///< Max number of instances of the item
    const unsigned int    offset;      ///< Offset [bytes] of items, or pointer to items, within the composite item
};


#endif // CFG_MAN_METADATA_H

