add_library(converter
    csv_reader.cpp
    columnar_writer.cpp
    conversion_batch.cpp
    csv_writer.cpp
    parsing.cpp
)

target_include_directories(converter
    PUBLIC ${PROJECT_SOURCE_DIR}/include
    PUBLIC ${PROJECT_SOURCE_DIR}/include/converter
    PUBLIC ${PROJECT_SOURCE_DIR}/tools
)

target_link_libraries(converter
    PUBLIC columnar_engine
)

target_compile_features(converter PUBLIC cxx_std_23)
