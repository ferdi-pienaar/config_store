# Possible improvement:
# Do I want this to discover the name of a yaml and use that?
# The code-generation script tries to load an ID YAML file, with a name derived from that of the main
# YAML config file. I could assign that responsibility to this file instead.
# Generated code should go in separate dir, for clarity.

# Code generation by custom tool.
# Input YAML files.
set(CFG_YAML ${CMAKE_CURRENT_SOURCE_DIR}/cfg.yaml)

# Prefix comes from the YAML filename without extension.
get_filename_component(CFG_NAME ${CFG_YAML} NAME_WE)

# Generated files. In future, create separate directory for generated code, like this:
set(GENERATED_DIR ${CMAKE_CURRENT_BINARY_DIR}/generated)
set(GENERATED_DIR ${CMAKE_CURRENT_SOURCE_DIR})
set(CFG_CPP ${GENERATED_DIR}/${CFG_NAME}.cpp)
set(CFG_H   ${GENERATED_DIR}/${CFG_NAME}.h)

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
    DEPENDS
        ${CFG_YAML}
        ${CMAKE_CURRENT_SOURCE_DIR}/../code_gen/yaml_code_gen.py
    COMMENT "Generating ${CFG_NAME}.cpp and ${CFG_NAME}.h from YAML"
)

foreach(BACKEND tlv json)
    add_executable(cfg_mgr_code_gen_example_${BACKEND}
        test_cfg1.cpp
        my_cfg_fn1.cpp
        ${CFG_CPP}
    )

    target_include_directories(cfg_mgr_code_gen_example_${BACKEND}
        PRIVATE
            ${CMAKE_CURRENT_SOURCE_DIR}
            ${GENERATED_DIR}
    )

    target_link_libraries(cfg_mgr_code_gen_example_${BACKEND}
        PRIVATE
            cfg_mgr_${BACKEND}
    )
endforeach()

#if(ENABLE_COVERAGE)
#    enable_coverage(event_recorder_example)
#endif()
