cmake_minimum_required(VERSION 3.16)
project(cfg_mgr
    VERSION 1.0
    LANGUAGES CXX)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_library(cfg_mgr_tlv)

target_include_directories(cfg_mgr_tlv
    PUBLIC
        ${CMAKE_SOURCE_DIR}/export
        ${CMAKE_SOURCE_DIR}/export/util
    PRIVATE
        ${CMAKE_SOURCE_DIR}/src
        ${CMAKE_SOURCE_DIR}/util
)

add_library(cfg_mgr_json)

target_include_directories(cfg_mgr_json
    PUBLIC
        ${CMAKE_SOURCE_DIR}/export
        ${CMAKE_SOURCE_DIR}/export/util
    PRIVATE
        ${CMAKE_SOURCE_DIR}/src
        ${CMAKE_SOURCE_DIR}/util
)

# ============================================================
# CORE SOURCES
# ============================================================
set(CFG_MGR_CORE_SRC
    src/cfg_mgr_implement.cpp
    src/cfg_mgr.cpp
    src/cfg_mgr_prt_hexstr.cpp
    src/cfg_mgr_printf.cpp
    src/cfg_mgr_simple_descriptor.cpp
    src/cfg_mgr_composite_descriptor.cpp
    src/cfg_mgr_aggregate.cpp
    src/cfg_mgr_contained_aggregate.cpp
    src/cfg_mgr_owned_aggregate.cpp
    src/cfg_mgr_cmd_stack.cpp
    src/cfg_mgr_cmd_ctxt.cpp

    src/store/cfg_mgr_store.cpp
    src/store/nvram.cpp

    # util sources (part of library)
    util/cfg_mgr_set_int.cpp
    util/cfg_mgr_set_str.cpp
    util/cfg_mgr_prt_int.cpp
    util/cfg_mgr_prt_str.cpp
    util/cfg_mgr_setdef_null.cpp
    util/cfg_mgr_strtok.cpp
)

set(CFG_MGR_TLV_STORE_SRC
    src/store/tlv/cfg_mgr_tlv_store.cpp
    src/store/tlv/cfg_mgr_tlv_writer.cpp
    src/store/tlv/cfg_mgr_tlv_loader.cpp
)

set(CFG_MGR_JSON_STORE_SRC
    src/store/json/cfg_mgr_json_store.cpp
    src/store/json/cfg_mgr_json_writer.cpp
    src/store/json/cfg_mgr_json_loader.cpp
    src/store/json/cfg_mgr_json_ctxt.cpp
)

target_sources(cfg_mgr_tlv PRIVATE ${CFG_MGR_CORE_SRC} ${CFG_MGR_TLV_STORE_SRC})

target_sources(cfg_mgr_json PRIVATE ${CFG_MGR_CORE_SRC} ${CFG_MGR_JSON_STORE_SRC})

add_subdirectory(example)
add_subdirectory(code_gen_example)

# -----------------------------
# Tests
# -----------------------------
enable_testing()
add_subdirectory(test)
