set(SRC
    test_cfg_mgr.cpp
    my_cfg.cpp
    my_cfg_fn.cpp
)

add_executable(cfg_mgr_example_tlv ${SRC})
target_link_libraries(cfg_mgr_example_tlv PRIVATE cfg_mgr_tlv)

add_executable(cfg_mgr_example_json ${SRC})
target_link_libraries(cfg_mgr_example_json PRIVATE cfg_mgr_json)

#if(ENABLE_COVERAGE)
#    enable_coverage(event_recorder_example)
#endif()
