#pragma once
#include "cfg_mgr_composite_descriptor.h"

static constexpr unsigned STRING_ARRAY_LEN = 8;
static constexpr unsigned ARRAY_LEN = 2;

struct m
{
    char m1[STRING_ARRAY_LEN]; // String is represented as an array, with metadata: size=len, and 1 instance in aggregate data.
    short int m2[ARRAY_LEN];
};

void setdef_t1(uint8_t *pItem, cfg_mgr::item_len_t len);

extern cfg_mgr::Composite_descriptor c1;

namespace owned_follower
{
// These structs allow us to test these cases in NVRAM:
// leader (may be absent) followed by array (may be absent) followed by follower (may be absent).
// owned_cnt is not persistent, so not saved to NVRAM.
static constexpr unsigned MAX_NUM_OWNED = 2;

struct t_dev
{
    int *leader;
    unsigned owned_cnt;
    int *owned;
    int *follower;
};

extern cfg_mgr::Composite_descriptor c1;

} // namespace owned_follower

namespace leader_owned_1_follower
{
// These structs allow us to test these cases in NVRAM:
// leader (may be absent) followed by object (may be absent) followed by follower (may be absent).
// Differs from the above case in that owned has max 1 instance, so is saved as object, not array.
struct t_dev
{
    int *leader;
    int *owned;
    int *follower;
};

extern cfg_mgr::Composite_descriptor c1;

} // namespace leader_owned_1_follower
