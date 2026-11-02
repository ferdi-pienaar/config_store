set(CONFIG_STORE_CODE_GEN ${PROJECT_SOURCE_DIR}/code_gen/yaml_code_gen.py)
set(SCHEMA_FILE ${PROJECT_SOURCE_DIR}/code_gen/cfg_schema.yaml)
set(ID_SCHEMA_FILE ${PROJECT_SOURCE_DIR}/code_gen/cfg_id_schema.yaml)
set(DEPENDENCY_H ${CMAKE_CURRENT_SOURCE_DIR}/cfg_depend.h)

function(add_e2e_test TEST_CONFIG BACKEND)
    set(GENERATED_DIR ${CMAKE_CURRENT_BINARY_DIR}/generated/${TEST_CONFIG}/${BACKEND})
    set(CFG_CPP ${GENERATED_DIR}/cfg.cpp)
    set(CFG_H   ${GENERATED_DIR}/cfg.h)
    set(CFG_YAML ${CMAKE_CURRENT_SOURCE_DIR}/${TEST_CONFIG}/cfg.yaml)

    # Command creates subdirectory for generated code and generates C++ code.
    add_custom_command(
        OUTPUT
            ${CFG_CPP}
            ${CFG_H}
        COMMAND
            ${CMAKE_COMMAND} -E make_directory ${GENERATED_DIR}
        COMMAND
            ${CONFIG_STORE_CODE_GEN}
            ${CFG_YAML}
            ${BACKEND}
            ${CFG_H}
            ${CFG_CPP}
            ${DEPENDENCY_H}
            ${SCHEMA_FILE}
            ${ID_SCHEMA_FILE}
        DEPENDS
            ${CFG_YAML}
            ${CONFIG_STORE_CODE_GEN}
        COMMENT "Generating ${CFG_H} and ${CFG_CPP} from YAML"
    )

    set(TEST_EXECUTABLE  test_e2e_${TEST_CONFIG}_${BACKEND})

    add_executable(${TEST_EXECUTABLE} test_main.cpp ${CFG_CPP})

    target_include_directories(${TEST_EXECUTABLE} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR} ${GENERATED_DIR})

    # Link to the library that implements the back-end.
    target_link_libraries(${TEST_EXECUTABLE} PRIVATE config_store_${BACKEND})

    if(ENABLE_COVERAGE)
        enable_coverage(${TEST_EXECUTABLE})
    endif()

    add_test(NAME ${TEST_EXECUTABLE}
        # The script takes 2 parameters: the name of the executable to run and the NVRAM file.
        COMMAND ${Python3_EXECUTABLE}
            ${CMAKE_CURRENT_SOURCE_DIR}/${TEST_CONFIG}/test.py
            ${CMAKE_CURRENT_BINARY_DIR}/${TEST_EXECUTABLE}
            cfg.${BACKEND}
    )
endfunction()

add_e2e_test(key_unsigned tlv)
add_e2e_test(key_unsigned json)
