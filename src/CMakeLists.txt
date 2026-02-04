add_library(columnar_engine
    schema.cpp
    csv_reader.cpp
    columnar_writer.cpp
    batch.cpp
    columnar_reader.cpp
    csv_writer.cpp
)

target_include_directories(columnar_engine
    PUBLIC ${PROJECT_SOURCE_DIR}/include
    PUBLIC ${PROJECT_SOURCE_DIR}/tools
)

target_compile_features(columnar_engine PUBLIC cxx_std_23)