// Metadata specific to JSON build for cfg_mgr_common_test.cpp.

#include "cfg_mgr_common_test.h"
#include "cfg_mgr_simple_descriptor.h"       // Unit under test
#include "cfg_mgr_composite_descriptor.h"       // Unit under test
#include "cfg_mgr_contained_aggregate.h"       // Unit under test
#include "cfg_mgr_owned_aggregate.h"       // Unit under test
#include "item_identifier.h"
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

namespace owned_follower
{

// owned with max count 1 and no counter
const Simple_metadata s1_d = {{"leader", PERSISTENT, sizeof(int)}, cm_set_int, cm_setdef_null, cm_prt_int};
Simple_descriptor s1(&s1_d);
const Owned_aggregate_data a1_d = {{&s1, 1, offsetof(t_dev, leader)}};
Owned_aggregate a1(&a1_d);

const Simple_metadata s2_d = {{"owned_cnt", NOT_PERSISTENT, sizeof(unsigned)}, nullptr, nullptr, cm_prt_int};
Simple_descriptor s2(&s2_d);
const Aggregate_data a2_d = {&s2, 1, offsetof(t_dev, owned_cnt)};
Contained_aggregate a2(&a2_d);

// owned with max count MAX_NUM_OWNED and counter
const Simple_metadata s3_d = {{"owned", PERSISTENT, sizeof(int)}, cm_set_int, cm_setdef_null, cm_prt_int};
Simple_descriptor s3(&s3_d);
const Owned_aggregate_data a3_d = {{&s3, MAX_NUM_OWNED, offsetof(t_dev, owned)}, &a2};
Owned_aggregate a3(&a3_d);

// owned with max count 1 and no counter
const Simple_metadata s4_d = {{"follower", PERSISTENT, sizeof(int)}, cm_set_int, cm_setdef_null, cm_prt_int};
Simple_descriptor s4(&s4_d);
const Owned_aggregate_data a4_d = {{&s4, 1, offsetof(t_dev, follower)}};
Owned_aggregate a4(&a4_d);

Aggregate * const aggrList1[] = {&a1, &a2, &a3, &a4};
const Composite_metadata c1_d = {{"dev", PERSISTENT, sizeof(t_dev)}, aggrList1, sizeof(aggrList1)/sizeof(aggrList1[0])};
Composite_descriptor c1(&c1_d);

} // namespace owned_follower

namespace leader_owned_1_follower
{

// owned with max count 1 and no counter
const Simple_metadata s1_d = {{"leader", PERSISTENT, sizeof(int)}, cm_set_int, cm_setdef_null, cm_prt_int};
Simple_descriptor s1(&s1_d);
const Owned_aggregate_data a1_d = {{&s1, 1, offsetof(t_dev, leader)}};
Owned_aggregate a1(&a1_d);

// owned with max count 1 and no counter
const Simple_metadata s2_d = {{"owned", PERSISTENT, sizeof(int)}, cm_set_int, cm_setdef_null, cm_prt_int};
Simple_descriptor s2(&s2_d);
const Owned_aggregate_data a2_d = {{&s2, 1, offsetof(t_dev, owned)}};
Owned_aggregate a2(&a2_d);

// owned with max count 1 and no counter
const Simple_metadata s3_d = {{"follower", PERSISTENT, sizeof(int)}, cm_set_int, cm_setdef_null, cm_prt_int};
Simple_descriptor s3(&s3_d);
const Owned_aggregate_data a3_d = {{&s3, 1, offsetof(t_dev, follower)}};
Owned_aggregate a3(&a3_d);

Aggregate * const aggrList1[] = {&a1, &a2, &a3};
const Composite_metadata c1_d = {{"dev", PERSISTENT, sizeof(t_dev)}, aggrList1, sizeof(aggrList1)/sizeof(aggrList1[0])};
Composite_descriptor c1(&c1_d);

} // namespace leader_owned_1_follower
