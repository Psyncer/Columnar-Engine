add_library(io
    columnar_reader.cpp
    columnar_writer.cpp
    csv_reader.cpp
    csv_writer.cpp
    parsing.cpp
)

target_include_directories(io PUBLIC
    ${PROJECT_SOURCE_DIR}
)

target_link_libraries(io PUBLIC 
    storage
)

target_compile_features(io PRIVATE
    cxx_std_23
)
