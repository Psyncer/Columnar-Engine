add_executable(test_full_cycle
    test_full_cycle.cpp
)

target_link_libraries(test_full_cycle
    PRIVATE converter
)