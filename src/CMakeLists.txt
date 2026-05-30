add_library(columnar_engine
    schema.cpp
    columnar_reader.cpp
    operator.cpp
    batch.cpp
    expression.cpp
)

find_package(PkgConfig REQUIRED)
pkg_check_modules(RE2 REQUIRED re2)
target_include_directories(columnar_engine PUBLIC ${RE2_INCLUDE_DIRS})
target_link_libraries(columnar_engine PUBLIC ${RE2_LIBRARIES})

find_package(absl REQUIRED)
target_link_libraries(columnar_engine PUBLIC absl::flat_hash_map absl::flat_hash_set)

target_include_directories(columnar_engine
    PUBLIC ${PROJECT_SOURCE_DIR}/include
    PUBLIC ${PROJECT_SOURCE_DIR}/include/converter
    PUBLIC ${PROJECT_SOURCE_DIR}/tools
)

add_subdirectory(converter)

target_compile_features(columnar_engine PUBLIC cxx_std_23)
