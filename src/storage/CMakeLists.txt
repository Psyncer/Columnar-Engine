add_library(storage
    batch.cpp
    column.cpp
    conversion_batch.cpp
    schema.cpp
)

target_include_directories(storage PUBLIC
    ${PROJECT_SOURCE_DIR}
)

target_compile_features(storage PRIVATE
    cxx_std_23
)
