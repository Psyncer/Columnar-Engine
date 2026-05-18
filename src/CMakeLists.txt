add_library(columnar_engine
    schema.cpp
    columnar_reader.cpp
    operator.cpp
    batch.cpp
    expression.cpp
)

target_include_directories(columnar_engine
    PUBLIC ${PROJECT_SOURCE_DIR}/include
    PUBLIC ${PROJECT_SOURCE_DIR}/include/converter
    PUBLIC ${PROJECT_SOURCE_DIR}/tools
)

add_subdirectory(converter)

target_compile_features(columnar_engine PUBLIC cxx_std_23)
