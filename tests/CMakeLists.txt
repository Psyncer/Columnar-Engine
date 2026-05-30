add_executable(test_queries
    test_queries.cpp
)

target_link_libraries(test_queries
    PRIVATE converter
    PRIVATE columnar_engine
)

target_compile_features(test_queries PRIVATE cxx_std_23)