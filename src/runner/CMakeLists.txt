add_executable(runner
    query_builder.cpp
    runner.cpp
)

target_include_directories(runner PUBLIC
    ${PROJECT_SOURCE_DIR}
)

target_link_libraries(runner PUBLIC 
    execution
    io
    storage
)

target_compile_features(runner PRIVATE
    cxx_std_23
)
