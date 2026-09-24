# Code generation by custom tool.
set(CONFIG_STORE_CODE_GEN ${PROJECT_SOURCE_DIR}/code_gen/yaml_code_gen.py)
set(CFG_YAML ${CMAKE_CURRENT_SOURCE_DIR}/config/cfg.yaml)
set(CFG_ID_YAML ${CMAKE_CURRENT_SOURCE_DIR}/config/cfg_id.yaml)
set(GENERATED_DIR ${CMAKE_CURRENT_BINARY_DIR}/generated)
set(CFG_CPP ${GENERATED_DIR}/cfg.cpp)
set(CFG_H   ${GENERATED_DIR}/cfg.h)
set(DEPENDENCY_H   ${CMAKE_CURRENT_SOURCE_DIR}/cfg_depend.h)
set(SCHEMA_FILE ${CMAKE_CURRENT_SOURCE_DIR}/../code_gen/cfg_schema.yaml)
set(ID_SCHEMA_FILE ${CMAKE_CURRENT_SOURCE_DIR}/../code_gen/cfg_id_schema.yaml)

# Command creates subdirectory and generates C++ code.
add_custom_command(
    OUTPUT
        ${CFG_CPP}
        ${CFG_H}
        ${CFG_ID_YAML}
    COMMAND
        ${CMAKE_COMMAND} -E make_directory ${GENERATED_DIR}
    COMMAND
        ${CONFIG_STORE_CODE_GEN}
        ${CFG_YAML}
        ${CFG_ID_YAML}
        ${CFG_H}
        ${CFG_CPP}
        ${DEPENDENCY_H}
        ${SCHEMA_FILE}
        ${ID_SCHEMA_FILE}
    DEPENDS
        ${CFG_YAML}
        ${CFG_ID_YAML}
        ${CONFIG_STORE_CODE_GEN}
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
