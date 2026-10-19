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
