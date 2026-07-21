cmake_minimum_required(VERSION 3.16)

# -------------------------------------------------
# GTest (installed via Ubuntu package)
# -------------------------------------------------
find_package(GTest REQUIRED)
include(GoogleTest)

enable_testing()

# -------------------------------------------------
# Test support library (stubs + util)
# -------------------------------------------------
add_library(config_store_test_support STATIC)

target_include_directories(config_store_test_support
    PUBLIC
        ${CMAKE_SOURCE_DIR}/export
        ${CMAKE_SOURCE_DIR}/export/util
        ${CMAKE_SOURCE_DIR}/src
        ${CMAKE_CURRENT_SOURCE_DIR}/stubs
)

target_sources(config_store_test_support PRIVATE
    stubs/nvram_spy.cpp
    stubs/cfg_mgr_printf_spy.cpp

    ../util/cfg_mgr_set_int.cpp
    ../util/cfg_mgr_set_str.cpp
    ../util/cfg_mgr_prt_int.cpp
    ../util/cfg_mgr_prt_str.cpp
    ../util/cfg_mgr_setdef_null.cpp
    ../util/cfg_mgr_strtok.cpp
)

set(COMMON_TEST_SOURCES
    cfg_mgr_common_test.cpp
)

set(TLV_TEST_SOURCES
    cm_tlvTest.cpp
    cm_simple_item_descriptorTest.cpp
    cm_composite_item_descriptorTest.cpp
    cfg_mgr_test.cpp
    strtok_test.cpp
)

set(JSON_TEST_SOURCES
    cm_jsonTest.cpp
)

# -------------------------------------------------
# TLV tests
# -------------------------------------------------
add_executable(config_store_test_tlv ${COMMON_TEST_SOURCES} ${TLV_TEST_SOURCES})

target_link_libraries(config_store_test_tlv PRIVATE
    config_store_tlv
    config_store_test_support
    GTest::gtest_main
    pthread
)

gtest_discover_tests(config_store_test_tlv)

# -------------------------------------------------
# JSON tests
# -------------------------------------------------
add_executable(config_store_test_json ${COMMON_TEST_SOURCES} ${JSON_TEST_SOURCES})

target_link_libraries(config_store_test_json PRIVATE
    config_store_json
    config_store_test_support
    GTest::gtest_main
    pthread
)

gtest_discover_tests(config_store_test_json)
