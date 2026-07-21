set(SRC
    test_cfg_mgr.cpp
    my_cfg.cpp
    my_cfg_fn.cpp
)

add_executable(config_store_example_tlv ${SRC})
target_link_libraries(config_store_example_tlv PRIVATE config_store_tlv)

add_executable(config_store_example_json ${SRC})
target_link_libraries(config_store_example_json PRIVATE config_store_json)

#if(ENABLE_COVERAGE)
#    enable_coverage(config_store_example_tlv)
#    enable_coverage(config_store_example_json)
#endif()
