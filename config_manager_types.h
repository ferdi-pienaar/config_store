// Common types needed in all files
#ifndef CFG_MAN_TYPES_H
#define CFG_MAN_TYPES_H

// Number of bytes in an item; used in NVRAM
// Because it determines the longest possible length of any item in NVRAM,
// it's also big enough to be used for the length of items in RAM
// (which are shorter, as the exclude the Id and Length fields saved to NVRAM).
typedef uint16_t cm_item_len_t;

// Identifier ID, unique within its context, used to identify it in NVRFAM
typedef uint16_t cm_item_id_t;

#endif // CFG_MAN_TYPES_H

