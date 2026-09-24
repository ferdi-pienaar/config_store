cmake_minimum_required(VERSION 3.16)
project(config_store
    VERSION 1.0
    LANGUAGES CXX)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_library(config_store_tlv)

target_include_directories(config_store_tlv
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include>
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include/util>
        $<INSTALL_INTERFACE:include/config_store>
    PRIVATE
        ${CMAKE_SOURCE_DIR}/src
)

add_library(config_store_json)

target_include_directories(config_store_json
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include>
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include/util>
        $<INSTALL_INTERFACE:include/config_store>
    PRIVATE
        ${CMAKE_SOURCE_DIR}/src
)

# ============================================================
# CORE SOURCES
# ============================================================
set(CFG_MGR_CORE_SRC
    src/cfg_mgr_implement.cpp
    src/cfg_mgr.cpp
    src/cfg_mgr_prt_hexstr.cpp
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

target_sources(config_store_tlv PRIVATE ${CFG_MGR_CORE_SRC} ${CFG_MGR_TLV_STORE_SRC})
target_sources(config_store_json PRIVATE ${CFG_MGR_CORE_SRC} ${CFG_MGR_JSON_STORE_SRC})

# Install the executable code-generation script.
install(PROGRAMS code_gen/yaml_code_gen.py
        DESTINATION bin
        RENAME config_store_code_gen)
# Install files used by the code-generation script to validate client YAML files.
install(FILES
        code_gen/cfg_schema.yaml
        code_gen/cfg_id_schema.yaml
        DESTINATION share/config_store)
install(DIRECTORY include/ DESTINATION include/config_store)
# Install the library and create package 'export set' descriptor.
install(TARGETS
        config_store_tlv
        config_store_json
        EXPORT config_storeTargets)
# Write package 'export set' to a file. It allows clients to import the package.
install(EXPORT config_storeTargets
        FILE config_storeTargets.cmake
        NAMESPACE config_store::
        DESTINATION lib/cmake/config_store)
# Write package configuration file; it allows clients to find the package descriptor file.
install(FILES cmake/config_storeConfig.cmake
        DESTINATION lib/cmake/config_store)

add_subdirectory(example)
add_subdirectory(code_gen_example)

# -----------------------------
# Tests
# -----------------------------
enable_testing()
add_subdirectory(test)
