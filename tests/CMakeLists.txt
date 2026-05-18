add_executable(test_queries
    test_queries.cpp
)

target_link_libraries(test_queries
    PRIVATE converter
)