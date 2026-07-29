# Possible improvement:
# Do I want this to discover the name of a yaml and use that?
# The code-generation script tries to load an ID YAML file, with a name derived from that of the main
# YAML config file. I could assign that responsibility to this file instead.
# Generated code should go in separate dir, for clarity.

# Code generation by custom tool.
# Input YAML files.
set(CFG_YAML ${CMAKE_CURRENT_SOURCE_DIR}/cfg.yaml)
set(CFG_ID_YAML ${CMAKE_CURRENT_SOURCE_DIR}/cfg_id.yaml)

# Generated files. In future, create separate directory for generated code, like this:
set(GENERATED_DIR ${CMAKE_CURRENT_BINARY_DIR}/generated)
set(GENERATED_DIR ${CMAKE_CURRENT_SOURCE_DIR})
set(CFG_CPP ${GENERATED_DIR}/cfg.cpp)
set(CFG_H   ${GENERATED_DIR}/cfg.h)
set(DEPENDENCY_H   ${CMAKE_CURRENT_SOURCE_DIR}/cfg_depend.h)
set(SCHEMA_FILE ${CMAKE_CURRENT_SOURCE_DIR}/../code_gen/cfg_schema.yaml)
set(ID_SCHEMA_FILE ${CMAKE_CURRENT_SOURCE_DIR}/../code_gen/cfg_id_schema.yaml)

find_package(Python3 REQUIRED COMPONENTS Interpreter)

# Command creates subdirectory and generates C++ code.
add_custom_command(
    OUTPUT
        ${CFG_CPP}
        ${CFG_H}
    COMMAND
        # Later when this generates code in the build dir, this will be useful.
        ${CMAKE_COMMAND} -E make_directory ${GENERATED_DIR}
    COMMAND
        ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/../code_gen/yaml_code_gen.py
        ${CFG_YAML}
        ${CFG_ID_YAML}
        ${CFG_H}
        ${CFG_CPP}
        ${DEPENDENCY_H}
        ${SCHEMA_FILE}
        ${ID_SCHEMA_FILE}
    DEPENDS
        ${CFG_YAML}
        ${CMAKE_CURRENT_SOURCE_DIR}/../code_gen/yaml_code_gen.py
    COMMENT "Generating ${CFG_H} and ${CFG_CPP} from YAML"
)

foreach(BACKEND tlv json)
    add_executable(config_store_code_gen_example_${BACKEND}
        test_cfg1.cpp
        my_cfg_fn1.cpp
        ${CFG_CPP}
    )

    target_include_directories(config_store_code_gen_example_${BACKEND}
        PRIVATE
            ${CMAKE_CURRENT_SOURCE_DIR}
            ${GENERATED_DIR}
    )

    target_link_libraries(config_store_code_gen_example_${BACKEND}
        PRIVATE
            config_store_${BACKEND}
    )
endforeach()

#if(ENABLE_COVERAGE)
#    enable_coverage(config_store_code_gen_example_tlv)
#    enable_coverage(config_store_code_gen_example_json)
#endif()
