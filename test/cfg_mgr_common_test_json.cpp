// Metadata specific to JSON build for cfg_mgr_common_test.cpp.

#include "cfg_mgr_common_test.h"
#include "cfg_mgr_simple_descriptor.h"       // Unit under test
#include "cfg_mgr_composite_descriptor.h"       // Unit under test
#include "cfg_mgr_contained_aggregate.h"       // Unit under test
#include "cfg_mgr_owned_aggregate.h"       // Unit under test
#include "cfg_mgr_set_int.h"  // Extensions to unit under test (generic "set" functions)
#include "cfg_mgr_prt_int.h"  // Extensions to unit under test (generic "print" functions)
#include "cfg_mgr_set_str.h"  // Extensions to unit under test (generic "set" functions)
#include "cfg_mgr_prt_str.h"  // Extensions to unit under test (generic "print" functions)
#include "cfg_mgr_setdef_null.h" // generic setdef function

using namespace cfg_mgr;

// test set 1 metadata
const Simple_metadata s1_d = {{"name1", NOT_PERSISTENT, sizeof(char) * STRING_ARRAY_LEN}, cm_set_str, cm_setdef_null, nullptr};
Simple_descriptor s1(&s1_d);
const Aggregate_data ca1_d = {&s1, 1, offsetof(struct m, m1)};
Contained_aggregate ca1(&ca1_d);

const Simple_metadata s2_d = {{"name2", PERSISTENT, sizeof(short int)}, cm_set_int, setdef_t1, cm_prt_int};
Simple_descriptor s2(&s2_d);
const Aggregate_data ca2_d = {&s2, ARRAY_LEN, offsetof(struct m, m2)};
Contained_aggregate ca2(&ca2_d);

Aggregate * const aggrList1[] = {&ca1, &ca2};
const Composite_metadata c1_d = {{"c1", PERSISTENT, sizeof(struct m)}, aggrList1, sizeof(aggrList1)/sizeof(aggrList1[0])};
Composite_descriptor c1(&c1_d);
